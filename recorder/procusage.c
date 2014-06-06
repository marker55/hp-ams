#include <stdio.h> 
#include <stdlib.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

#include "ams_rec.h"
#include "recorder.h"

/* routines needed for debug output */
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/output_api.h>

/* required to re-schedule log event */
#include "net-snmp/library/snmp_alarm.h"

void LOG_PROCESSOR_USAGE();

int rec_init();
int doCodeDescriptor(unsigned int, unsigned int, unsigned char, unsigned char,
                     unsigned char, unsigned char, unsigned char, char *);

extern int file_select(const struct dirent *);
extern int proc_select(const struct dirent *);
extern int log_interval;
extern void rec_log_proc_usage(unsigned int, void *);
extern int proc_usage_reinit;
extern void
dump_chunk(const char *debugtoken, const char *title, const u_char * buf,
           int size);

static field fld[] = {
      {REC_SIZE_BYTE,   REC_TYP_NUMERIC, "User Time "},
      {REC_SIZE_BYTE,   REC_TYP_NUMERIC, "Kernel Time "},
};

int cpucount;

void LOG_PROCESSOR_USAGE()
{
    int j = 0;
    int rc;
    int num_descript;
    size_t toc_sz = 0;
    size_t max_toc_sz = (10 * sizeof(descript));
    struct dirent **cpuent;
    descript *toc;
    descript *ttoc;

    /* do something different for VmWare and Solaris */
    cpucount = scandir("/sys/class/cpuid", &cpuent, file_select, versionsort);
    DEBUGMSGTL(("rec:rec_log:rec_log_proc_usage","cpucount = %d\n", cpucount));

    if (cpucount == -1) 
        return;

    if (!rec_init() )
        return;

    /* do Code Descriptor */
    if ((rc = doCodeDescriptor(s_ams_rec_handle, 
                               s_ams_rec_class, 
			       REC_CODE_AMS_PROCESSOR_USAGE,
			       0,
			       REC_SEV_PERIOD,
			       REC_BINARY_DATA,
			       REC_VIS_CUSTOMER,
			       REC_CODE_AMS_PROCESSOR_USAGE_STR)) 
            != RECORDER_OK) {
        DEBUGMSGTL(("rec:rec_log_error", 
		    "Host Name register descriptor failed %d\n", rc));
        return;
    }

    num_descript = sizeof(fld)/sizeof(field) * cpucount;
    DEBUGMSGTL(("rec:rec_log:rec_log_proc_usage","ProcUsage num_descript = %d\n", 			num_descript));

    toc_sz = sizeof(descript) * num_descript;
    toc = malloc(toc_sz);
    DEBUGMSGTL(("rec:log","Initial Processor Usage toc_sz = %ld\n", toc_sz));

    /* now do the field descriptor */
    memset(toc, 0, toc_sz);
    for (j = 0; j < num_descript; j++ ) {
        toc[j].flags = REC_FLAGS_DESCRIPTOR | REC_FLAGS_DESC_FIELD;
        toc[j].classs = s_ams_rec_class;
        toc[j].code = REC_CODE_AMS_PROCESSOR_USAGE;
        toc[j].field = j;
        toc[j].severity_type = REC_SEV_PERIOD | fld[j%2].tp;
        toc[j].format = fld[j%2].sz;
        toc[j].visibility = REC_VIS_CUSTOMER;
        strncpy(toc[j].desc, fld[j%2].nm, strlen(fld[j%2].nm),
                sizeof(toc[j].desc) - strlen(toc[j].desc - 1));
        strcat(toc[j].desc, cpuent[j/2]->d_name);
        DEBUGMSGTL(("rec:log", "toc[i] = %p\n", &toc[j]));
        dump_chunk("rec:rec_log","descriptor", (const u_char *)&toc[j], 96);
        if (j % 2)
            free(cpuent[j/2]);
    }
    dump_chunk("rec:rec_log", "toc", (const u_char *)toc, toc_sz);
    free(cpuent);

    ttoc = toc;
    while (toc_sz > RECORDER_MAX_BLOB_SIZE) {
        DEBUGMSGTL(("rec:log","Processor Usage ttoc = %p\n", ttoc));
        DEBUGMSGTL(("rec:log","Processor Usage toc_sz = %ld\n", toc_sz));
        if ((rc = rec_api4_field(s_ams_rec_handle, 
                                      max_toc_sz,
                                      ttoc)) != RECORDER_OK) {
            DEBUGMSGTL(("rec:rec_log_error","Proc Usage register descriptor failed %d\n", 
                        rc));
            return;
        }
        toc_sz = toc_sz - max_toc_sz;
        ttoc = (void *)ttoc + max_toc_sz;
    }
    if ((rc = rec_api4_field(s_ams_rec_handle, toc_sz, ttoc)) 
                    != RECORDER_OK) {
        DEBUGMSGTL(("rec:rec_log_error","Proc Usage register descriptor failed %d\n", rc));
        return;
    }
    free(toc);

    if (!proc_usage_reinit) 
        switch (log_interval) {
            case 0:
                rec_log_proc_usage(0, &cpucount);
                break;
            case 1:
                snmp_alarm_register(60, SA_REPEAT, &rec_log_proc_usage, &cpucount);
                break;
            case 2:
                snmp_alarm_register(600, SA_REPEAT, &rec_log_proc_usage, &cpucount);
                break;
            case 3:
                snmp_alarm_register(3600, SA_REPEAT, &rec_log_proc_usage, &cpucount);
                break;
        }
    else
        proc_usage_reinit = 0;
    return;
}


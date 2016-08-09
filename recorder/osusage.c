#define _BSD_SOURCE
#include <stdio.h> 
#include <stdlib.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "ams_rec.h"
#include "recorder.h"

/* routines needed for debug output */
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/output_api.h>

/* required to re-schedule log event */
#include "net-snmp/library/snmp_alarm.h"

int rec_init();
int doCodeDescriptor(unsigned int, unsigned int, unsigned char, unsigned char,
                     unsigned char, unsigned char, unsigned char, char *);

extern int log_interval;
extern void rec_log_os_usage(unsigned int, void *);
extern int os_usage_reinit;

static field fld[] = {
      {REC_SIZE_DWORD,   REC_TYP_NUMERIC, "Process Count"},
      {REC_SIZE_DWORD,   REC_TYP_NUMERIC, "Thread Count"},
      {REC_SIZE_DWORD,   REC_TYP_NUMERIC, "Handle Count"},
      {REC_SIZE_DWORD,   REC_TYP_NUMERIC, "Load Avg (1 min)"},
      {REC_SIZE_DWORD,   REC_TYP_NUMERIC, "Load Avg (5 min)"},
      {REC_SIZE_DWORD,   REC_TYP_NUMERIC, "Load Avg (15 min)"},
};

void LOG_OS_USAGE()
{
    int i = 0;
    int rc;
    int num_descript;
    size_t toc_sz = 0;
    RECORDER_API4_RECORD *toc;


    if (!rec_init() )
        return;

   /* do Code Descriptor */
    if ((rc = doCodeDescriptor(s_ams_rec_handle,
                               s_ams_rec_class,
                               REC_CODE_AMS_OS_USAGE,
                               0,
                               REC_SEV_PERIOD,
                               REC_BINARY_DATA,
                               REC_VIS_CUSTOMER,
                               REC_CODE_AMS_OS_USAGE_STR)) != RECORDER_OK) {
        DEBUGMSGTL(("rec:rec_log_error","OS Usage register descriptor failed %d\n", rc));
        return;
    }

    /* build descriptor toc */
    num_descript = sizeof(fld)/sizeof(field);
    DEBUGMSGTL(("rec:log","OS Usage num_descript = %d\n", num_descript));
    if ((toc_sz = sizeof(RECORDER_API4_RECORD) * num_descript)
            > RECORDER_MAX_BLOB_SIZE) {
        DEBUGMSGTL(("rec:rec_log_error","OS Usage Descriptor too large %ld\n", toc_sz));
        return;
    }
    DEBUGMSGTL(("rec:log","OS Usage toc_sz = %ld\n", toc_sz));
    if ((toc = malloc(toc_sz)) == NULL) {
        DEBUGMSGTL(("rec:log","OS Usage Unable to malloc() %ld bytes\n", toc_sz));
        return;
    }

    /* now do the field descriptor */
    memset(toc, 0, toc_sz);
    for ( i = 0; i < num_descript; i++) {
        toc[i].flags = REC_FLAGS_API4 | REC_FLAGS_DESC_FIELD;
        toc[i].classs = s_ams_rec_class;
        toc[i].code = REC_CODE_AMS_OS_USAGE;
        toc[i].field = i;
        toc[i].severity_type = REC_SEV_PERIOD | fld[i].tp;
        toc[i].format = fld[i].sz;
        toc[i].visibility = REC_VIS_CUSTOMER;
        strncpy(toc[i].desc, fld[i].nm, strlen(fld[i].nm));
        DEBUGMSGTL(("rec:log", "toc[i] = %p\n", &toc[i]));
        dump_chunk("rec:rec_log","descriptor",(const u_char *)&toc[i],96);
    }

    dump_chunk("rec:rec_log","toc", (const u_char *) toc,toc_sz);

    if ((rc = rec_api4_field(s_ams_rec_handle, toc_sz, toc))
            != RECORDER_OK) {
        DEBUGMSGTL(("rec:rec_log_error","OS Usage register descriptor failed %d\n", rc));
        return;
    }
    free(toc);

    if (!os_usage_reinit) 
        switch (log_interval) {
            case 0:
                rec_log_os_usage(0, NULL);
                break;
            case 1:
                snmp_alarm_register(60, SA_REPEAT, &rec_log_os_usage, NULL);
                break;
            case 2:
                snmp_alarm_register(600, SA_REPEAT, &rec_log_os_usage, NULL);
                break;
            case 3:
                snmp_alarm_register(3600, SA_REPEAT, &rec_log_os_usage, NULL);
                break;
        }
    else
        os_usage_reinit = 0;
    return;
}


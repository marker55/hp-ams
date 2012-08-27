#define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include "cpqHost/libcpqhost/cpqhost.h"
#include "recorder.h"
#include "data_access/services.h"

#include "net-snmp/net-snmp-config.h"
#include "net-snmp/library/snmp_impl.h"
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/output_api.h>
#include "net-snmp/library/tools.h"

extern int s_class;
extern int s_handle;

int recorder_init();
int doCodeDescriptor(unsigned int, unsigned int, unsigned char, unsigned char,
                     unsigned char, unsigned char, unsigned char, char *);

typedef struct _ServiceData
{
    UINT16   Flags;
} ServiceData;

extern svc_entry **services;

static field fld[]= {
    {SIZE_WORD,   TYP_NUMERIC, "Flags"},
    {BINARY_DATA, TYP_TEXT,    "Name"},
    {BINARY_DATA, TYP_TEXT,    "File"},
    {BINARY_DATA, TYP_TEXT,    "Version"},
    {BINARY_DATA, TYP_TEXT,    "Timestamp"},
    {BINARY_DATA, TYP_TEXT,    "Vendor"},
};

enum ServiceFlags
{
    Service_First            = 0x0001,
    Service_Last             = 0X0002,
    Service_Start            = 0X0004,
    Service_Stop             = 0X0008
};

int getService();
void record_services()
{
    int i, j;

    int rc;
    int num_descript;
    size_t toc_sz = 0;
    size_t blob_sz = 0;
    ServiceData *Data;

    char *blob;
    DESCRIPTOR_RECORD *toc;
    int svccount = 0;


    if (!recorder_init() )
        return;

   /* do Code Descriptor */
    if ((rc = doCodeDescriptor(s_handle, s_class,
                               6, 0, 2, 0, 1,
                               "Service")) != OK) {
        DEBUGMSGTL(("recorder:log_service","Service register descriptor failed %d\n", rc));
        return;
    }

    /* build descriptor toc */
    num_descript = sizeof(fld)/sizeof(field);
    DEBUGMSGTL(("recorder:log","Service num_descript = %d\n", num_descript));
    if ((toc_sz = sizeof(DESCRIPTOR_RECORD) * num_descript)
            > MAX_BLOB_SIZE) {
        DEBUGMSGTL(("recorder:log","Service Descriptor too large %ld\n", toc_sz));
        return;
    }
    DEBUGMSGTL(("recorder:log","Service toc_sz = %ld\n", toc_sz));
    if ((toc = malloc(toc_sz)) == NULL) {
        DEBUGMSGTL(("recorder:log","Service Unable to malloc() %ld bytes\n", toc_sz));
        return;
    }

    /* now do the field descriptor */
    memset(toc, 0, toc_sz);
    for ( i = 0; i < num_descript; i++) {
        toc[i].flags = 0x8c;
        toc[i].classs = s_class;
        toc[i].code = 6;
        toc[i].field = i;
        toc[i].severity_type = 2 | (fld[i].tp << 4);
        toc[i].format = fld[i].sz;
        toc[i].visibility =  1;
        strncpy(toc[i].desc, fld[i].nm, strlen(fld[i].nm));
        DEBUGMSGTL(("recorder:log_service", "toc[i] = %p\n", &toc[i]));
        dump_chunk("recorder:log_service", "descriptor", (const u_char *)&toc[i], 96);
    }

    dump_chunk("recorder:log_service","toc",(const u_char *)toc, toc_sz);

    if ((rc = recorder4_field(s_handle, toc_sz, toc))
            != OK) {
        DEBUGMSGTL(("recorder:log_service", "Service register descriptor failed %d\n", rc));
        return;
    }
    free(toc);

    /* Let's go ahead and set the code for */
    if ((rc = recorder3(s_handle, 6))
               != OK) {
        DEBUGMSGTL(("recorder:log_service", "recorder3 failed (%d)\n", rc));
        return;
    }

    /* Let's get our blob */
    svccount = getService();
    DEBUGMSGTL(("recorder:log_service","svccount = %d\n", svccount));
    for (j = 0; j < svccount; j++ ) {
        char *end;
        DEBUGMSGTL(("recorder:log_service","Iter = %d\n", j));
        blob_sz = sizeof(ServiceData);
        blob_sz += strlen(services[j]->name);
        blob_sz += strlen(services[j]->filename);
        blob_sz += 5;
        if (blob_sz > MAX_BLOB_SIZE) {
            DEBUGMSGTL(("recorder:log_service","Service Data too large %ld\n", blob_sz));
            return;
        }
        if ((blob = malloc(blob_sz)) == NULL ) {
            DEBUGMSGTL(("recorder:log_service","Service Unable to malloc() %ld blob\n", blob_sz));
            free(services[j]);
            continue;
        }
        memset(blob, 0, blob_sz);
    
        /* Got the Blob, so let's stuff in the data */
        Data = (ServiceData *)blob;
        Data->Flags = Service_Start;
        if (j == 0) 
                Data->Flags |= Service_First;
        if (j == (svccount - 1)) 
                Data->Flags |= Service_Last;

        end = blob + sizeof(ServiceData);
        strcpy(end, services[j]->name);

        end += strlen(services[j]->name) + 1;
        strcpy(end, services[j]->filename);

        DEBUGMSGTL(("recorder:log","Service name %s, Service File Name %s\n",
           services[j]->name, services[j]->filename));

        // Log the record
        if ((rc = record(s_handle, (const char*)blob, blob_sz)) !=
                            OK) {
            DEBUGMSGTL(("recorder:log", "service record failed (%d)\n",rc));
        }

        DEBUGMSGTL(("recorder:log", "Logged record for code %d\n", 6));
        free(blob);
        free(services[j]);
    }
    free(services);
}


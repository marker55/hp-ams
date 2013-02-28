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
#include "ams_rec.h"
#include "recorder.h"
#include "data.h"
#include "data_access/service.h"

/* routines needed for debug output */
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/output_api.h>

int rec_init();
int doCodeDescriptor(unsigned int, unsigned int, unsigned char, unsigned char,
                     unsigned char, unsigned char, unsigned char, char *);

svc_entry **services;

static field fld[]= {
    {REC_SIZE_WORD,   REC_TYP_NUMERIC, "Flags"},
    {REC_BINARY_DATA, REC_TYP_TEXT,    "Name"},
    {REC_BINARY_DATA, REC_TYP_TEXT,    "File"},
};

int getService();
void LOG_SERVICE(
        /*REC_AMS_OsType Type,
          REC_AMS_OsVendor Vendor,
          UINT16 Bits,
          const char* NameString,
          const char* VersionString,
          const UINT16 Version[4],
          UINT16 ServicePack*/)
{
    int i, j;

    int rc;
    int num_descript;
    size_t toc_sz = 0;
    size_t blob_sz = 0;
    REC_AMS_ServiceData *ServiceData;

    char *blob;
    descript *toc;
    int svccount = 0;


    if (!rec_init() )
        return;

   /* do Code Descriptor */
    if ((rc = doCodeDescriptor(s_ams_rec_handle,
                               s_ams_rec_class,
                               REC_CODE_AMS_SERVICE,
                               0,
                               REC_SEV_CONFIG,
                               REC_BINARY_DATA,
                               REC_VIS_CUSTOMER,
                               REC_CODE_AMS_SERVICE_STR)) != RECORDER_OK) {
        DEBUGMSGTL(("rec:rec_log_service","Service register descriptor failed %d\n", rc));
        return;
    }

    /* build descriptor toc */
    num_descript = sizeof(fld)/sizeof(field);
    DEBUGMSGTL(("rec:log","Service num_descript = %d\n", num_descript));
    if ((toc_sz = sizeof(descript) * num_descript)
            > RECORDER_MAX_BLOB_SIZE) {
        DEBUGMSGTL(("rec:log","Service Descriptor too large %ld\n", toc_sz));
        return;
    }
    DEBUGMSGTL(("rec:log","Service toc_sz = %ld\n", toc_sz));
    if ((toc = malloc(toc_sz)) == NULL) {
        DEBUGMSGTL(("rec:log","Service Unable to malloc() %ld bytes\n", toc_sz));
        return;
    }

    /* now do the field descriptor */
    memset(toc, 0, toc_sz);
    for ( i = 0; i < num_descript; i++) {
        toc[i].flags = REC_FLAGS_DESCRIPTOR | REC_FLAGS_DESC_FIELD;
        toc[i].classs = s_ams_rec_class;
        toc[i].code = REC_CODE_AMS_SERVICE;
        toc[i].field = i;
        toc[i].severity_type = REC_DESC_SEVERITY_CONFIGURATION | fld[i].tp;
        toc[i].format = fld[i].sz;
        toc[i].visibility =  REC_DESC_VISIBILITY_CUSTOMER;
        strncpy(toc[i].desc, fld[i].nm, strlen(fld[i].nm));
        DEBUGMSGTL(("rec:log_service", "toc[i] = %p\n", &toc[i]));
        dump_chunk("rec:log_service", "descriptor", (const u_char *)&toc[i], 96);
    }

    dump_chunk("rec:log_service","toc",(const u_char *)toc, toc_sz);

    if ((rc = rec_api4_field(s_ams_rec_handle, toc_sz, toc))
            != RECORDER_OK) {
        DEBUGMSGTL(("rec:log_service", "Service register descriptor failed %d\n", rc));
        return;
    }
    free(toc);

    /* Let's go ahead and set the code for */
    if ((rc = rec_api3(s_ams_rec_handle, REC_CODE_AMS_SERVICE))
               != RECORDER_OK) {
        DEBUGMSGTL(("rec:log_service", "SetRecorderFeederCode failed (%d)\n", rc));
        return;
    }

    /* Let's get our blob */
    svccount = getService();
    DEBUGMSGTL(("rec:log_service","svccount = %d\n", svccount));
    for (j = 0; j < svccount; j++ ) {
        char *end;
        DEBUGMSGTL(("rec:log_service","Iter = %d\n", j));
        blob_sz = sizeof(REC_AMS_ServiceData);
        blob_sz += strlen(services[j]->name);
        blob_sz += strlen(services[j]->filename);
        blob_sz += 2;
        if (blob_sz > RECORDER_MAX_BLOB_SIZE) {
            DEBUGMSGTL(("rec:log_service","Service Data too large %ld\n", blob_sz));
            return;
        }
        if ((blob = malloc(blob_sz)) == NULL ) {
            DEBUGMSGTL(("rec:log_service","Service Unable to malloc() %ld blob\n", blob_sz));
            free(services[j]);
            continue;
        }
        memset(blob, 0, blob_sz);
    
        /* Got the Blob, so let's stuff in the data */
        ServiceData = (REC_AMS_ServiceData *)blob;
        ServiceData->Flags = REC_AMS_Service_Start;
        if (j == 0) 
                ServiceData->Flags |= REC_AMS_Service_First;
        if (j == (svccount - 1)) 
                ServiceData->Flags |= REC_AMS_Service_Last;

        end = blob + sizeof(REC_AMS_ServiceData);
        strcpy(end, services[j]->name);

        end += strlen(services[j]->name) + 1;
        strcpy(end, services[j]->filename);

        DEBUGMSGTL(("rec:log","Service name %s, Service File Name %s\n",
           services[j]->name, services[j]->filename));

        // Log the record
        if ((rc = rec_log(s_ams_rec_handle, (const char*)blob, blob_sz)) !=
                            RECORDER_OK) {
            DEBUGMSGTL(("rec:log", "LogRecorderData failed (%d)\n",rc));
        }

        DEBUGMSGTL(("rec:log", "Logged record for code %d\n",
                    REC_CODE_AMS_SERVICE));
        free(blob);
        free(services[j]->name);
        free(services[j]->filename);
        free(services[j]);
    }
    free(services);
}


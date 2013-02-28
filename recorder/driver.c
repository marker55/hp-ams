#define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include "cpqHost/libcpqhost/cpqhost.h"
#include "ams_rec.h"
#include "recorder.h"
#include "data.h"
#include "data_access/driver.h"

/* routines needed for debug output */
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/output_api.h>

int rec_init();
int doCodeDescriptor(unsigned int, unsigned int, unsigned char, unsigned char,
                     unsigned char, unsigned char, unsigned char, char *);

int getDriver();

drv_entry **drivers;

static field fld[]= {
    {REC_SIZE_WORD,   REC_TYP_NUMERIC, "Flags"},
    {REC_BINARY_DATA, REC_TYP_TEXT,    "Name"},
    {REC_BINARY_DATA, REC_TYP_TEXT,    "Filename"},
    {REC_BINARY_DATA, REC_TYP_TEXT,    "Version"},
    {REC_BINARY_DATA, REC_TYP_TEXT,    "Timestamp"},
};
void LOG_DRIVER(
        /*REC_AMS_OsType Type,
          REC_AMS_OsVendor Vendor,
          UINT16 Bits,
          const char* NameString,
          const char* VersionString,
          const UINT16 Version[4],
          UINT16 ServicePack*/)
{
    int i = 0;
    int drvcount;
    int rc;
    int num_descript;
    size_t toc_sz = sizeof(descript);;
    size_t blob_sz = sizeof(REC_AMS_DriverData);
    size_t string_list_sz = 0;
    REC_AMS_DriverData *DriverData;

    char *blob;
    descript *toc;

    if (!rec_init() )
        return;

   /* do Code Descriptor */
    if ((rc = doCodeDescriptor(s_ams_rec_handle,
                               s_ams_rec_class,
                               REC_CODE_AMS_DRIVER,
                               0,
                               REC_SEV_CONFIG,
                               REC_BINARY_DATA,
                               REC_VIS_CUSTOMER,
                               REC_CODE_AMS_DRIVER_STR)) != RECORDER_OK) {
        DEBUGMSGTL(("rec:rec_log_driver","Driver register descriptor failed %d\n", rc));
        return;
    }

    /* build descriptor toc */
    num_descript = sizeof(fld)/sizeof(field);
    DEBUGMSGTL(("rec:logdriver","Driver num_descript = %d\n", num_descript));
    if ((toc_sz *= num_descript) > RECORDER_MAX_BLOB_SIZE) {
        DEBUGMSGTL(("rec:logdriver","Driver Descriptor too large %ld\n", toc_sz));
        return;
    }
    DEBUGMSGTL(("rec:logdriver","Driver toc_sz = %ld\n", toc_sz));
    if ((toc = malloc(toc_sz)) == NULL) {
        DEBUGMSGTL(("rec:logdriver","Driver Unable to malloc() %ld bytes\n", toc_sz));
        return;
    }

    /* now do the field descriptor */
    memset(toc, 0, toc_sz);
    for ( i = 0; i < num_descript; i++) {
        toc[i].flags = REC_FLAGS_DESCRIPTOR | REC_FLAGS_DESC_FIELD;
        toc[i].classs = s_ams_rec_class;
        toc[i].code = REC_CODE_AMS_DRIVER;
        toc[i].field = i;
        toc[i].severity_type = REC_DESC_SEVERITY_CONFIGURATION | fld[i].tp;
        toc[i].format = fld[i].sz;
        toc[i].visibility =  REC_DESC_VISIBILITY_CUSTOMER;
        strncpy(toc[i].desc, fld[i].nm, strlen(fld[i].nm));
        DEBUGMSGTL(("rec:logdriver", "toc[i] = %p\n", &toc[i]));
        dump_chunk("rec:rec_log", "descriptor", (const u_char *)&toc[i], 96);
    }

    dump_chunk("rec:rec_log", "toc", (const u_char *)toc, toc_sz);

    if ((rc = rec_api4_field(s_ams_rec_handle, toc_sz, toc))
            != RECORDER_OK) {
        DEBUGMSGTL(("rec:logdriver","Driver register descriptor failed %d\n", rc));
        return;
    }
    free(toc);

    /* Let's go ahead and set the code for */
    if ((rc = rec_api3(s_ams_rec_handle, REC_CODE_AMS_DRIVER))
               != RECORDER_OK) {
        DEBUGMSGTL(("rec:logdriver", "SetRecorderFeederCode failed (%d)\n", rc));
        return;
    }

    /* Let's get our blob */
    drvcount = getDriver();
    for (i = 0; i < drvcount; i++ ) {
        char * end;
        blob_sz = sizeof(REC_AMS_DriverData);
        string_list_sz = strlen(drivers[i]->name) +
                 strlen(drivers[i]->filename) +
                 strlen(drivers[i]->version) +
                 strlen(drivers[i]->timestamp) +
                 4;
        if ((blob_sz += string_list_sz) > RECORDER_MAX_BLOB_SIZE) {
            DEBUGMSGTL(("rec:logdriver","Driver Data too large %ld\n", blob_sz));
            return;
        }
        if ((blob = malloc(blob_sz)) == NULL ) {
            DEBUGMSGTL(("rec:logdriver","Driver Unable to malloc() %ld blob\n", blob_sz));
            goto free;
        }
        memset(blob, 0, blob_sz);

        /* Got the Blob, so let's stuff in the data */
        DriverData = (REC_AMS_DriverData *)blob;
        DriverData->Flags = REC_AMS_Driver_Load;
        if (i == 0)
                DriverData->Flags |= REC_AMS_Driver_First;
        if (i == (drvcount - 1))
                DriverData->Flags |= REC_AMS_Driver_Last;

        end = blob + sizeof(REC_AMS_DriverData);
        strcpy(end, drivers[i]->name);
        end += strlen(drivers[i]->name) + 1;

        strcpy(end, drivers[i]->filename);
        end += strlen(drivers[i]->filename) + 1;

        strcpy(end, drivers[i]->version);
        end += strlen(drivers[i]->version) + 1;

        strcpy(end, drivers[i]->timestamp);

        DEBUGMSGTL(("rec:logdriver","Module name %s, Driver filename  %s\n",
           drivers[i]->name, drivers[i]->filename));
        DEBUGMSGTL(("rec:logdriver","Module version %s, Driver timestamp  %s\n",
           drivers[i]->version, drivers[i]->timestamp));

        // Log the record
        if ((rc = rec_log(s_ams_rec_handle, (const char*)blob, blob_sz)) !=
                            RECORDER_OK) {
            DEBUGMSGTL(("rec:logdriver", "LogRecorderData failed (%d)\n",rc));
        }

        DEBUGMSGTL(("rec:logdriver", "Logged record for code %d\n",
                    REC_CODE_AMS_DRIVER));
free:
        free(blob);
        if (drivers[i]->timestamp != NULL) free(drivers[i]->timestamp);
        if (drivers[i]->version != NULL) free(drivers[i]->version);
        if (drivers[i]->filename != NULL) free(drivers[i]->filename);
        if (drivers[i]->name != NULL) free(drivers[i]->name);
        free(drivers[i]);
    }
    free(drivers);
}


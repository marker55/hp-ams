#define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include "cpqHost/libcpqhost/cpqhost.h"
#include "recorder.h"
#include "data_access/drivers.h"

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

typedef struct _DriverData
{
    UINT16   Flags;
} DriverData;

// OsDriverData.Flags
//
 enum DriverFlags
 {
     Driver_First             = 0x0001,
     Driver_Last              = 0x0002,
     Driver_Load              = 0x0004,
     Driver_Unload            = 0x0008
 };
//
extern int getDriver();
extern drv_entry **drivers;

static field fld[]= {
    {SIZE_WORD,   TYP_NUMERIC, "Flags"},
    {BINARY_DATA, TYP_TEXT,    "Name"},
    {BINARY_DATA, TYP_TEXT,    "Filename"},
    {BINARY_DATA, TYP_TEXT,    "Version"},
    {BINARY_DATA, TYP_TEXT,    "Timestamp"},
    {BINARY_DATA, TYP_TEXT,    "Vendor"},
};
void record_drivers()
{
    int i = 0;
    int drvcount;
    int rc;
    int num_descript;
    size_t toc_sz = 0;
    size_t blob_sz = 0;
    size_t string_list_sz = 0;
    DriverData *Data;

    char *blob;
    DESCRIPTOR_RECORD *toc;

    if (!recorder_init() )
        return;

   /* do Code Descriptor */
    if ((rc = doCodeDescriptor(s_handle, s_class,
                               5, 0, 2, 0, 1,
                               "Driver")) != OK) {
        DEBUGMSGTL(("recorder:log_driver","Driver register descriptor failed %d\n", rc));
        return;
    }

    /* build descriptor toc */
    num_descript = sizeof(fld)/sizeof(field);
    DEBUGMSGTL(("recorder:log","Driver num_descript = %d\n", num_descript));
    if ((toc_sz = sizeof(DESCRIPTOR_RECORD) * num_descript)
            > MAX_BLOB_SIZE) {
        DEBUGMSGTL(("recorder:log","Driver Descriptor too large %ld\n", toc_sz));
        return;
    }
    DEBUGMSGTL(("recorder:log","Driver toc_sz = %ld\n", toc_sz));
    if ((toc = malloc(toc_sz)) == NULL) {
        DEBUGMSGTL(("recorder:log","Driver Unable to malloc() %ld bytes\n", toc_sz));
        return;
    }

    /* now do the field descriptor */
    memset(toc, 0, toc_sz);
    for ( i = 0; i < num_descript; i++) {
        toc[i].flags =0x8c;
        toc[i].classs = s_class;
        toc[i].code = 5;
        toc[i].field = i;
        toc[i].severity_type = 2 | (fld[i].tp << 4);
        toc[i].format = fld[i].sz;
        toc[i].visibility = 1;
        strncpy(toc[i].desc, fld[i].nm, strlen(fld[i].nm));
        DEBUGMSGTL(("recorder:log", "toc[i] = %p\n", &toc[i]));
        dump_chunk("recorder:log", "descriptor", (const u_char *)&toc[i], 96);
    }

    dump_chunk("recorder:log", "toc", (const u_char *)toc, toc_sz);

    if ((rc = recorder4_field(s_handle, toc_sz, toc))
            != OK) {
        DEBUGMSGTL(("recorder:log","Driver register descriptor failed %d\n", rc));
        return;
    }
    free(toc);

    /* Let's go ahead and set the code for */
    if ((rc = recorder3(s_handle, 5))
               != OK) {
        DEBUGMSGTL(("recorder:log", "recorder3 failed (%d)\n", rc));
        return;
    }

    /* Let's get our blob */
    drvcount = getDriver();
    for (i = 0; i < drvcount; i++ ) {
        char * end;
        string_list_sz = strlen(drivers[i]->name) +
                 strlen(drivers[i]->filename) +
                 strlen(drivers[i]->version) +
                 strlen(drivers[i]->timestamp) +
                 6;
        if ((blob_sz = sizeof(DriverData) + string_list_sz)
                        > MAX_BLOB_SIZE) {
            DEBUGMSGTL(("recorder:log","Driver Data too large %ld\n", blob_sz));
            return;
        }
        if ((blob = malloc(blob_sz)) == NULL ) {
            DEBUGMSGTL(("recorder:log","Driver Unable to malloc() %ld blob\n", blob_sz));
            goto free;
        }
        memset(blob, 0, blob_sz);

        /* Got the Blob, so let's stuff in the data */
        Data = (DriverData *)blob;
        Data->Flags = Driver_Load;
        if (i == 0)
                Data->Flags |= Driver_First;
        if (i == (drvcount - 1))
                Data->Flags |= Driver_Last;

        end = blob + sizeof(DriverData);
        strcpy(end, drivers[i]->name);
        end += strlen(drivers[i]->name) + 1;

        strcpy(end, drivers[i]->filename);
        end += strlen(drivers[i]->filename) + 1;

        strcpy(end, drivers[i]->version);
        end += strlen(drivers[i]->version) + 1;

        strcpy(end, drivers[i]->timestamp);

        DEBUGMSGTL(("recorder:log","Module name %s, Driver filename  %s\n",
           drivers[i]->name, drivers[i]->filename));
        DEBUGMSGTL(("recorder:log","Module version %s, Driver timestamp  %s\n",
           drivers[i]->version, drivers[i]->timestamp));

        // Log the record
        if ((rc = record(s_handle, (const char*)blob, blob_sz)) !=
                            OK) {
            DEBUGMSGTL(("recorder:log", "driver record failed (%d)\n",rc));
        }

        DEBUGMSGTL(("recorder:log", "Logged record for code %d\n", 5));
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


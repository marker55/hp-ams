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
#include "data_access/package.h"

/* routines needed for debug output */
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/output_api.h>

int rec_init();
int doCodeDescriptor(unsigned int, unsigned int, unsigned char, unsigned char,
                     unsigned char, unsigned char, unsigned char, char *);

pkg_entry **packages;

static field fld[]= {
    {REC_SIZE_WORD,   REC_TYP_NUMERIC, "Flags"},
    {REC_BINARY_DATA, REC_TYP_TEXT,    "Vendor"},
    {REC_BINARY_DATA, REC_TYP_TEXT,    "Name"},
    {REC_BINARY_DATA, REC_TYP_TEXT,    "Version"},
    {REC_BINARY_DATA, REC_TYP_TEXT,    "Timestamp"},
    {REC_BINARY_DATA, REC_TYP_TEXT,    "Package"},
};

int getPackage();
void LOG_PACKAGE(
        /*REC_AMS_OsType Type,
          REC_AMS_OsVendor Vendor,
          UINT16 Bits,
          const char* NameString,
          const char* VersionString,
          const UINT16 Version[4],
          UINT16 PackagePack*/)
{
    int i, j;

    int rc;
    int num_descript;
    size_t toc_sz = 0;
    size_t blob_sz = 0;
    REC_AMS_PackageData *PackageData;

    char *blob;
    RECORDER_API4_RECORD *toc;
    int pkgcount = 0;


    if (!rec_init() )
        return;

   /* do Code Descriptor */
    if ((rc = doCodeDescriptor(s_ams_rec_handle,
                               s_ams_rec_class,
                               REC_CODE_AMS_SOFTWARE,
                               0,
                               REC_SEV_CONFIG,
                               REC_BINARY_DATA,
                               REC_VIS_CUSTOMER,
                               REC_CODE_AMS_SOFTWARE_STR)) != RECORDER_OK) {
        DEBUGMSGTL(("rec:rec_log_package","Package register descriptor failed %d\n", rc));
        return;
    }

    /* build descriptor toc */
    num_descript = sizeof(fld)/sizeof(field);
    DEBUGMSGTL(("rec:log","Package num_descript = %d\n", num_descript));
    if ((toc_sz = sizeof(RECORDER_API4_RECORD) * num_descript)
            > RECORDER_MAX_BLOB_SIZE) {
        DEBUGMSGTL(("rec:log","Package Descriptor too large %ld\n", toc_sz));
        return;
    }
    DEBUGMSGTL(("rec:log","Package toc_sz = %ld\n", toc_sz));
    if ((toc = malloc(toc_sz)) == NULL) {
        DEBUGMSGTL(("rec:log","Package Unable to malloc() %ld bytes\n", toc_sz));
        return;
    }

    /* now do the field descriptor */
    memset(toc, 0, toc_sz);
    for ( i = 0; i < num_descript; i++) {
        toc[i].flags = REC_FLAGS_API4 | REC_FLAGS_DESC_FIELD;
        toc[i].classs = s_ams_rec_class;
        toc[i].code = REC_CODE_AMS_SOFTWARE;
        toc[i].field = i;
        toc[i].severity_type = REC_DESC_SEVERITY_CONFIGURATION | fld[i].tp;
        toc[i].format = fld[i].sz;
        toc[i].visibility =  REC_DESC_VISIBILITY_CUSTOMER;
        strncpy(toc[i].desc, fld[i].nm, strlen(fld[i].nm));
        DEBUGMSGTL(("rec:log_package", "toc[i] = %p\n", &toc[i]));
        dump_chunk("rec:log_package", "descriptor", (const u_char *)&toc[i], 96);
    }

    dump_chunk("rec:log_package","toc",(const u_char *)toc, toc_sz);

    if ((rc = rec_api4_field(s_ams_rec_handle, toc_sz, toc))
            != RECORDER_OK) {
        DEBUGMSGTL(("rec:log_package", "Package register descriptor failed %d\n", rc));
        return;
    }
    free(toc);

    /* Let's go ahead and set the code for */
    if ((rc = rec_api3(s_ams_rec_handle, REC_CODE_AMS_SOFTWARE))
               != RECORDER_OK) {
        DEBUGMSGTL(("rec:log_package", "SetRecorderFeederCode failed (%d)\n", rc));
        return;
    }

    /* Let's get our blob */
    pkgcount = getPackage();
    DEBUGMSGTL(("rec:log_package","pkgcount = %d\n", pkgcount));
    for (j = 0; j < pkgcount; j++ ) {
        char *end;
        DEBUGMSGTL(("rec:log_package","Iter = %d\n", j));
        blob_sz = sizeof(REC_AMS_PackageData) + 5; /* account for null bytes */
	if (packages[j]->vendor != NULL)
            blob_sz += strlen(packages[j]->vendor);
	if (packages[j]->sname != NULL)
            blob_sz += strlen(packages[j]->sname);
	if (packages[j]->version != NULL)
            blob_sz += strlen(packages[j]->version);
	if (packages[j]->name != NULL)
            blob_sz += strlen(packages[j]->name);
	if (packages[j]->timestamp != NULL)
            blob_sz += strlen(packages[j]->timestamp);
        if (blob_sz > RECORDER_MAX_BLOB_SIZE) {
            DEBUGMSGTL(("rec:log_package","Package Data too large %ld\n", blob_sz));
            return;
        }
        if ((blob = malloc(blob_sz)) == NULL ) {
            DEBUGMSGTL(("rec:log_package","Package Unable to malloc() %ld blob\n", blob_sz));
            free(packages[j]);
            continue;
        }
        memset(blob, 0, blob_sz);
    
        /* Got the Blob, so let's stuff in the data */
        PackageData = (REC_AMS_PackageData *)blob;
        PackageData->Flags = REC_AMS_Package_Install;
        if (j == 0) 
                PackageData->Flags |= REC_AMS_Package_First;
        if (j == (pkgcount - 1)) 
                PackageData->Flags |= REC_AMS_Package_Last;

        end = blob + sizeof(REC_AMS_PackageData);
	if (packages[j]->vendor != NULL) {
            DEBUGMSGTL(("rec:logpackage","Package vendor %s\n", packages[j]->vendor));
            strcpy(end, packages[j]->vendor);
            end += strlen(packages[j]->vendor) + 1;
            free(packages[j]->vendor);
	} else
            end++;

	if (packages[j]->sname != NULL) {
            strcpy(end, packages[j]->sname);
            end += strlen(packages[j]->sname) + 1;
            free(packages[j]->sname);
	} else
            end++;

	if (packages[j]->version != NULL) {
            strcpy(end, packages[j]->version);
            end += strlen(packages[j]->version) + 1;
            free(packages[j]->version);
	} else
            end++;

	if (packages[j]->timestamp != NULL) {
            strcpy(end, packages[j]->timestamp);
            end += strlen(packages[j]->timestamp) + 1;
            free(packages[j]->timestamp);
	} else
            end++;

	if (packages[j]->name != NULL) {
            DEBUGMSGTL(("rec:logpackage","Package name %s\n", packages[j]->name));
            strcpy(end, packages[j]->name);
            free(packages[j]->name);
	}


        // Log the record
        if ((rc = rec_api6(s_ams_rec_handle, (const char*)blob, blob_sz)) !=
                            RECORDER_OK) {
            DEBUGMSGTL(("rec:log", "LogRecorderData failed (%d)\n",rc));
        }

        DEBUGMSGTL(("rec:log", "Logged record for code %d\n",
                    REC_CODE_AMS_SOFTWARE));
        free(blob);
        free(packages[j]);
    }
    free(packages);
}


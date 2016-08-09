#define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "cpqHost/libcpqhost/cpqhost.h"
#include "ams_rec.h"
#include "recorder.h"
#include "data.h"

/* routines needed for debug output */
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/output_api.h>

int rec_init();
int doCodeDescriptor(unsigned int, unsigned int, unsigned char, unsigned char,
                     unsigned char, unsigned char, unsigned char, char *);

static field fld[] = {
      {REC_SIZE_BYTE,   REC_TYP_NUMERIC, "Name"},
      {REC_SIZE_BYTE,   REC_TYP_NUMERIC, "Vendor"},
      {REC_SIZE_WORD,   REC_TYP_NUMERIC, "Bits"},
      {REC_SIZE_WORD,   REC_TYP_NUMERIC, "Version-Major"},
      {REC_SIZE_WORD,   REC_TYP_NUMERIC, "Version-Minor"},
      {REC_SIZE_WORD,   REC_TYP_NUMERIC, "Build Number"},
      {REC_SIZE_WORD,   REC_TYP_NUMERIC, "Patch Level"},
      {REC_SIZE_WORD,   REC_TYP_NUMERIC, "Service Pack"},
      {REC_BINARY_DATA, REC_TYP_TEXT,    "OS Version"},
      {REC_BINARY_DATA, REC_TYP_TEXT,    "Kernel Version"},
};

void LOG_OS_INFORMATION()
{
    int i = 0;
    int rc;
    int num_descript;
    size_t toc_sz = sizeof(RECORDER_API4_RECORD);
    size_t blob_sz = sizeof(REC_AMS_OsInfoData);
    size_t string_list_sz = 0;
    REC_AMS_OsInfoData *OsInfoData;
    distro_id_t *myDistro;
    char * end;

    char *blob;
    RECORDER_API4_RECORD *toc;

    if (!rec_init() )
        return;
        
   /* do Code Descriptor */
    if ((rc = doCodeDescriptor(s_ams_rec_handle,
                               s_ams_rec_class,
                               REC_CODE_AMS_OS_INFORMATION,
                               0,
                               REC_SEV_CONFIG,
                               REC_BINARY_DATA,
                               REC_VIS_CUSTOMER,
                               REC_CODE_AMS_OS_INFORMATION_STR)) != RECORDER_OK) {
        DEBUGMSGTL(("rec:rec_log_osinfo","OS Information register descriptor failed %d\n", rc));
        return;
    }

    /* build descriptor toc */
    num_descript = sizeof(fld)/sizeof(field);
    DEBUGMSGTL(("rec:log","OS Info num_descript = %d\n", num_descript));
    if ((toc_sz *= num_descript) > RECORDER_MAX_BLOB_SIZE) {
        DEBUGMSGTL(("rec:log","OS Info Descriptor too large %ld\n", toc_sz));
        return;
    }
    DEBUGMSGTL(("rec:log","OS Info toc_sz = %ld\n", toc_sz));
    if ((toc = malloc(toc_sz)) == NULL) {
        DEBUGMSGTL(("rec:log","OS Info Unable to malloc() %ld bytes\n", toc_sz));
        return;
    }

    /* now do the field descriptor */
    memset(toc, 0, toc_sz);
    for ( i = 0; i < num_descript; i++) {
        toc[i].flags = REC_FLAGS_API4 | REC_FLAGS_DESC_FIELD;
        toc[i].classs = s_ams_rec_class;
        toc[i].code = REC_CODE_AMS_OS_INFORMATION;
        toc[i].field = i;
        toc[i].severity_type = REC_DESC_SEVERITY_CONFIGURATION | fld[i].tp;
        toc[i].format = fld[i].sz;
        toc[i].visibility =  REC_DESC_VISIBILITY_CUSTOMER;
        strncpy(toc[i].desc, fld[i].nm, strlen(fld[i].nm));
        DEBUGMSGTL(("rec:log", "toc[i] = %p\n", &toc[i]));
        dump_chunk("rec:rec_log","descriptor", (const u_char *)&toc[i], 96);
    }
       
    dump_chunk("rec:rec_log","toc", (const u_char *)toc, toc_sz);

    if ((rc = rec_api4_field(s_ams_rec_handle, toc_sz, toc)) 
            != RECORDER_OK) {
        DEBUGMSGTL(("rec:log","OS Info register descriptor failed %d\n", rc));
        return;
    }
    free(toc);

    /* Let's go ahead and set the code for */
    if ((rc = rec_api3(s_ams_rec_handle, REC_CODE_AMS_OS_INFORMATION)) 
                != RECORDER_OK) {
        DEBUGMSGTL(("rec:log", "SetRecorderFeederCode failed (%d)\n", rc));
        return;
    }

    /* Get the distro Info */
    myDistro = getDistroInfo();

    /* Let's get our blob */
    string_list_sz = strlen((const char *)myDistro->LongDistro) +
                       strlen((const char *)myDistro->KernelVersion) + 3;
    if ((blob_sz += string_list_sz) > RECORDER_MAX_BLOB_SIZE) {
        DEBUGMSGTL(("rec:log","OS Info Data too large %ld\n", blob_sz));
        return;
    }
    if ((blob = malloc(blob_sz)) == NULL ) {
        DEBUGMSGTL(("rec:log","OS Info Unable to malloc() %ld blob\n", blob_sz));
        return;
    }
    memset(blob, 0, blob_sz);

    /* Got the Blob, so let's stuff in the data */
    OsInfoData = (REC_AMS_OsInfoData *)blob;
    OsInfoData->Type = REC_AMS_OsType_Linux;

    OsInfoData->Vendor = REC_AMS_OsVendor_Other;
    if (strcmp((const char *)myDistro->Vendor, "Red Hat") == 0)
        OsInfoData->Vendor = REC_AMS_OsVendor_RedHat;
    else
        if (strcmp((const char *)myDistro->Vendor, "SuSE") == 0)
            OsInfoData->Vendor = REC_AMS_OsVendor_SUSE;

    OsInfoData->Bits = myDistro->Bits;
    DEBUGMSGTL(("rec:log","OS Info Type %d, Vendor %d, Bits %d\n",
                OsInfoData->Type, OsInfoData->Vendor, OsInfoData->Bits));

    OsInfoData->Version[0] = myDistro->Version;
    OsInfoData->Version[1] = myDistro->SubVersion;
    OsInfoData->Version[2] = myDistro->BuildNumber;
    OsInfoData->Version[3] = 0;
    DEBUGMSGTL(("rec:log","OS Info "
                "Version[0] %d, Version[1] %d, Version[2] %d, Version[3] %d\n",
                OsInfoData->Version[0], OsInfoData->Version[1], 
                OsInfoData->Version[2], OsInfoData->Version[3]));

    OsInfoData->ServicePack = myDistro->ServicePack;

    end  = blob + sizeof(REC_AMS_OsInfoData);

    strcpy(end, (const char *)myDistro->LongDistro);
    DEBUGMSGTL(("rec:log","OS Info %s, KernelVersion %s\n",
                myDistro->LongDistro, myDistro->LongDistro));

    end += strlen((const char *)myDistro->LongDistro) + 1;

    strcpy(end, (const char *)myDistro->KernelVersion);
    DEBUGMSGTL(("rec:log","OS Info %s, KernelVersion %s\n",
                myDistro->LongDistro, myDistro->KernelVersion));

    // Log the record
    if ((rc = rec_api6(s_ams_rec_handle, (const char*)blob, blob_sz)) != 
             RECORDER_OK) {
        DEBUGMSGTL(("rec:log", "LogRecorderData failed (%d)\n",rc));
    }

    DEBUGMSGTL(("rec:log", "Logged record for code %d\n", 
                REC_CODE_AMS_OS_INFORMATION));
    free(blob);
    return;

}



#define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "ams_rec.h"
#include "recorder.h"
#include "data.h"

#include "net-snmp/net-snmp-config.h"
#include "net-snmp/library/snmp_impl.h"
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/output_api.h>
#include "net-snmp/library/tools.h"

int rec_init();
int doCodeDescriptor(unsigned int, unsigned int, unsigned char, unsigned char,
                     unsigned char, unsigned char, unsigned char, char *);

static field fld[] = {
    {REC_BINARY_DATA, REC_TYP_TEXT,    "Server Name"},
};

void LOG_HOST_NAME()
{
    int i = 0;
    int rc;
    int num_descript;
    size_t toc_sz = 0;
    size_t blob_sz = 0;
    char * string1;
    char * string2;

    char *blob;
    descript *toc;

    if (!rec_init() ) {
        return;
    }
        
   /* do Code Descriptor */
    if ((rc = doCodeDescriptor(s_ams_rec_handle,
                               s_ams_rec_class,
                               REC_CODE_AMS_HOST_NAME,
                               0,
                               REC_SEV_NONCRIT,
                               REC_BINARY_DATA,
                               REC_VIS_CUSTOMER,
                               REC_CODE_AMS_HOST_NAME_STR)) != RECORDER_OK) {
        DEBUGMSGTL(("rec:rec_api6_hostname","Host Name register descriptor failed %d\n", rc));
        return;
    }

    /* build descriptor toc */
    num_descript = sizeof(fld)/sizeof(field);
    DEBUGMSGTL(("rec:log","Host Name num_descript = %d\n", num_descript));
    if ((toc_sz = sizeof(descript) * num_descript) 
            > RECORDER_MAX_BLOB_SIZE) {
        DEBUGMSGTL(("rec:log","Host Name Descriptor too large %ld\n", toc_sz));
        return;
    }
    DEBUGMSGTL(("rec:log","Host Naqme toc_sz = %ld\n", toc_sz));
    if ((toc = malloc(toc_sz)) == NULL) {
        DEBUGMSGTL(("rec:log","Host Name Unable to malloc %ld bytes\n", toc_sz));
        return;
    }
    memset(toc, 0, toc_sz);
    /* now do the field descriptor */
    for ( i = 0; i < num_descript; i++) {
        toc[i].flags = REC_FLAGS_DESCRIPTOR | REC_FLAGS_DESC_FIELD;
        toc[i].classs = s_ams_rec_class;
        toc[i].code = REC_CODE_AMS_HOST_NAME;
        toc[i].field = i;
        toc[i].severity_type = REC_DESC_SEVERITY_NON_CRITICAL | fld[i].tp;
        toc[i].format = fld[i].sz;
        toc[i].visibility =  REC_DESC_VISIBILITY_CUSTOMER;
        strncpy(toc[i].desc, fld[i].nm, strlen(fld[i].nm));
        DEBUGMSGTL(("rec:log", "toc[i] = %p\n", &toc[i]));
        dump_chunk("rec:rec_api6","descriptor", (const u_char *)&toc[i], 96);
    }
       
    dump_chunk("rec:rec_api6","toc", (const u_char *)toc, toc_sz);

    if ((rc = rec_api4_field(s_ams_rec_handle, toc_sz, toc)) 
            != RECORDER_OK) {
        DEBUGMSGTL(("rec:log","Host Name register descriptor failed %d\n", rc));
        free(toc);
        return;
    }
    free(toc);

    /* Let's go ahead and set the code for */
    if ((rc = rec_api3(s_ams_rec_handle, REC_CODE_AMS_HOST_NAME)) 
                != RECORDER_OK) {
        DEBUGMSGTL(("rec:log", "SetRecorderFeederCode failed (%d)\n", rc));
        return;
    }

    /* Let's get our blob */
    blob_sz = RECORDER_MAX_BLOB_SIZE;
    if ((blob = malloc(blob_sz)) == NULL ) {
        DEBUGMSGTL(("rec:log","OS Info Unable to malloc() %ld blob\n", blob_sz));
        return;
    }
    memset(blob, 0, blob_sz);

    /* Got the Blob, so let's stuff in the data */
    if (gethostname(blob, blob_sz)) {
        free(blob);
        DEBUGMSGTL(("rec:log","gethostname failed - errno = %d\n", errno));
        return;
    }

    DEBUGMSGTL(("rec:log","Host Name %s\n", blob));

    blob_sz = strlen(blob) + 1;
    // Log the record
    if ((rc = rec_api6(s_ams_rec_handle, (const char*)blob, blob_sz)) != 
             RECORDER_OK) {
        DEBUGMSGTL(("rec:log", "LogRecorderData failed (%d)\n",rc));
    }

    DEBUGMSGTL(("rec:log", "Logged record for code %d\n", 
                REC_CODE_AMS_HOST_NAME));
    return;

}



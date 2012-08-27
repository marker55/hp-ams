#define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "recorder.h"

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

static field fld[] = {
    {BINARY_DATA, TYP_TEXT,    "Server Name"},
    {BINARY_DATA, TYP_TEXT,    "Fully Qualified Domain Name"},
};

void record_hostname()
{
    int i = 0;
    int rc;
    int num_descript;
    size_t toc_sz = 0;
    size_t blob_sz = 0;
    char * string1;
    char * string2;

    char *blob;
    DESCRIPTOR_RECORD *toc;

    if (!recorder_init() ) {
        return;
    }
        
   /* do Code Descriptor */
    if ((rc = doCodeDescriptor(s_handle,
                               s_class,
                               4,
                               0,
                               2,
                               0,
                               1,
                               "Host Name")) != OK) {
        DEBUGMSGTL(("recorder:log_hostname","Host Name register descriptor failed %d\n", rc));
        return;
    }

    /* build descriptor toc */
    num_descript = sizeof(fld)/sizeof(field);
    DEBUGMSGTL(("recorder:log","Host Name num_descript = %d\n", num_descript));
    if ((toc_sz = sizeof(DESCRIPTOR_RECORD) * num_descript) 
            > MAX_BLOB_SIZE) {
        DEBUGMSGTL(("recorder:log","Host Name Descriptor too large %ld\n", toc_sz));
        return;
    }
    DEBUGMSGTL(("recorder:log","Host Naqme toc_sz = %ld\n", toc_sz));
    if ((toc = malloc(toc_sz)) == NULL) {
        DEBUGMSGTL(("recorder:log","Host Name Unable to malloc %ld bytes\n", toc_sz));
        return;
    }
    memset(toc, 0, toc_sz);
    /* now do the field descriptor */
    for ( i = 0; i < num_descript; i++) {
        toc[i].flags = 0x8c;
        toc[i].classs = s_class;
        toc[i].code = 4;
        toc[i].field = i;
        toc[i].severity_type = 2 | ( fld[i].tp << 4);
        toc[i].format = fld[i].sz;
        toc[i].visibility = 1;
        strncpy(toc[i].desc, fld[i].nm, strlen(fld[i].nm));
        DEBUGMSGTL(("recorder:log", "toc[i] = %p\n", &toc[i]));
        dump_chunk("recorder:log","descriptor", (const u_char *)&toc[i], 96);
    }
       
    dump_chunk("recorder:log","toc", (const u_char *)toc, toc_sz);

    if ((rc = recorder4_field(s_handle, toc_sz, toc)) 
            != OK) {
        DEBUGMSGTL(("recorder:log","Host Name register descriptor failed %d\n", rc));
        free(toc);
        return;
    }
    free(toc);

    /* Let's go ahead and set the code for */
    if ((rc = recorder3(s_handle, 4)) 
                != OK) {
        DEBUGMSGTL(("recorder:log", "recorder3 failed (%d)\n", rc));
        return;
    }

    /* Let's get our blob */
    blob_sz = MAX_BLOB_SIZE;
    if ((blob = malloc(blob_sz)) == NULL ) {
        DEBUGMSGTL(("recorder:log","OS Info Unable to malloc() %ld blob\n", blob_sz));
        return;
    }
    memset(blob, 0, blob_sz);

    /* Got the Blob, so let's stuff in the data */
    if (gethostname(blob, blob_sz)) {
        free(blob);
        DEBUGMSGTL(("recorder:log","gethostname failed - errno = %d\n", errno));
        return;
    }
    string1 = blob + strlen(blob) + 1;
    strncpy(string1, blob, strlen(blob));
    string2 = string1 + strlen(string1) + 1;
    if (getdomainname(string2, SPRINT_MAX_LEN)) 
        DEBUGMSGTL(("recorder:log","getdomainname failed - errno = %d\n", errno));
    else 
        *(string2 -1 ) = '.';

    DEBUGMSGTL(("recorder:log","Host Name %s, FQDN %s\n",
                blob, string1));

    blob_sz = strlen(blob) + strlen(string1) + 3;
    // Log the record
    if ((rc = record(s_handle, (const char*)blob, blob_sz)) != 
             OK) {
        DEBUGMSGTL(("recorder:log", "hostname record failed (%d)\n",rc));
    }

    DEBUGMSGTL(("recorder:log", "Logged record for code %d\n", 4));
    return;

}



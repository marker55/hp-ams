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

/* routines needed for debug output */
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/output_api.h>

int rec_init();
int doCodeDescriptor(unsigned int, unsigned int, unsigned char, unsigned char,
                     unsigned char, unsigned char, unsigned char, char *);

void LOG_OS_BOOT()
{
    int rc;

    if (!rec_init() ) {
        return;
    }
        
   /* do Code Descriptor */
    if ((rc = doCodeDescriptor(s_ams_rec_handle,
                               s_ams_rec_class,
                               REC_CODE_AMS_OS_BOOT,
                               0,
                               REC_SEV_CHANGE,
                               REC_BINARY_DATA,
                               REC_VIS_CUSTOMER,
                               REC_CODE_AMS_OS_BOOT_STR)) != RECORDER_OK) {
        DEBUGMSGTL(("rec:rec_log_osboot","OS Boot  register descriptor failed %d\n", rc));
        return;
    }

    /* Let's go ahead and set the code for */
    if ((rc = rec_api3(s_ams_rec_handle, REC_CODE_AMS_OS_BOOT)) 
                != RECORDER_OK) {
        DEBUGMSGTL(("rec:log", "OS Boot rec_api3 failed (%d)\n", rc));
        return;
    }

    // Log the record
    if ((rc = rec_log(s_ams_rec_handle, (const char*)0, 0)) != 
             RECORDER_OK) {
        DEBUGMSGTL(("rec:log", "OS Boot rec_log failed (%d)\n",rc));
    }

    DEBUGMSGTL(("rec:log", "OS Boot Logged record for code %d\n", 
                REC_CODE_AMS_OS_BOOT));
    return;
}

void LOG_OS_SHUTDOWN()
{
    int rc;

    if (!rec_init() ) {
        return;
    }

   /* do Code Descriptor */
    if ((rc = doCodeDescriptor(s_ams_rec_handle,
                               s_ams_rec_class,
                               REC_CODE_AMS_OS_SHUTDOWN,
                               0,
                               REC_SEV_CHANGE,
                               REC_BINARY_DATA,
                               REC_VIS_CUSTOMER,
                               REC_CODE_AMS_OS_SHUTDOWN_STR)) != RECORDER_OK) {
        DEBUGMSGTL(("rec:rec_log_osboot","OS Boot  register descriptor failed %d\n", rc));
        return;
    }

    /* Let's go ahead and set the code for */
    if ((rc = rec_api3(s_ams_rec_handle, REC_CODE_AMS_OS_SHUTDOWN)) 
                != RECORDER_OK) {
        DEBUGMSGTL(("rec:log", "OS Shutdown rec_api3 failed (%d)\n", rc));
        return;
    }

    // Log the record
    if ((rc = rec_log(s_ams_rec_handle, (const char*)0, 0)) != 
             RECORDER_OK) {
        DEBUGMSGTL(("rec:log", "OS Shutdown LogRecorderData failed (%d)\n",rc));
    }

    DEBUGMSGTL(("rec:log", "OS Shutdown Logged record for code %d\n", 
                REC_CODE_AMS_OS_SHUTDOWN));
    return;

}



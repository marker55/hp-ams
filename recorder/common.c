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

int doCodeDescriptor(unsigned int handle,
                     unsigned int class,
                     unsigned char code,
                     unsigned char field,
                     unsigned char sev_type,
                     unsigned char format,
                     unsigned char vis,
                     char *descripstr)
{
    RECORDER_API4_RECORD toc;
    int rc;

    memset(&toc, 0, sizeof(RECORDER_API4_RECORD));
    toc.flags = REC_FLAGS_API4 | REC_FLAGS_DESC_CODE;
    toc.classs = class;
    toc.code = code;
    toc.field = 0;
    toc.severity_type = sev_type;
    toc.format = format;
    toc.visibility = vis;
    strcpy(&toc.desc[0], descripstr);
    if ((rc = rec_api4_code(s_ams_rec_handle,
                    sizeof(RECORDER_API4_RECORD), &toc)) != RECORDER_OK) {
        DEBUGMSGTL(("rec:log","Host Name register descriptor failed %d\n", rc));
    }
    return rc;
}


//#include <net-snmp/net-snmp-config.h>

#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "recorder.h"
#include "common/utils.h"

#ifdef UTEST
#define snmp_log(level, format ...) fprintf(stderr, format, ## __VA_ARGS)
#define DEBUGMSGTL(level, format, ...) fprintf(stderr, format, ## __VA_ARGS)`
#else
/* routines needed for debug output */
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/output_api.h>
#endif

#define MAXHPILOCHANNELS 24

#define HPILO_CCB "/dev/hpilo/d0ccb"
extern void
dump_chunk(const char *debugtoken, const char *title, const u_char * buf,
           int size);

int rec_fd = -1;
extern int s_ams_rec_class;
extern int s_ams_rec_handle;
extern int proc_usage_reinit;
extern int mem_usage_reinit;
extern int os_usage_reinit;

void close_rec(int reset)
{
    if (rec_fd != -1) {
        DEBUGMSGTL(("rec:arch", "close %d\n", rec_fd));
        close(rec_fd);
        rec_fd = -1;
    }
    if (reset) {
        s_ams_rec_class = -1;
        s_ams_rec_handle = -1;
        proc_usage_reinit = 1;
        mem_usage_reinit = 1;
        os_usage_reinit = 1;
        
    }
}

void init_rec(void)  {

    int  start = -1;
    char dev[256] = "";
    char *max_ccb = NULL;
    struct stat buf;

    if (rec_fd != -1 )
        return;

    if ((max_ccb = get_sysfs_str("/sys/module/hpilo/parameters/max_ccb")) == NULL) {
        start = MAXHPILOCHANNELS - 1;
        
        sprintf(dev,"%s%d", HPILO_CCB, start);
        while ((start > 0) &&  (stat(dev, &buf) < 0)) {
            start = start - 8;
            sprintf(dev,"%s%d", HPILO_CCB, start);
        }
    } else {
        start = atoi(max_ccb);
        free(max_ccb);
    }

    while (start >= 0 && ((rec_fd = open(dev, O_RDWR|O_EXCL)) == -1)) 
        sprintf(dev,"%s%d", HPILO_CCB, --start);

    if (start <  0) {
        snmp_log(LOG_ERR, "Failed: no available HP iLO channels\n");
        rec_fd = -1;
    }  
    DEBUGMSGTL(("rec:arch", "open %d\n", rec_fd));
    /* give the iLO a change to digest the open */
    usleep(1000);
    return;
}

int doWrite( _req * req, int size)
{
    int cnt = -1;

    if (rec_fd == -1)
        init_rec();

    req->hdr.pktsz = size;
    dump_chunk("rec:arch","Write", (const u_char *)req, size);
    if((cnt = write(rec_fd, req, size)) < 0) {
        return -1;
    }
    DEBUGMSGTL(("rec:arch", "write %d bytes\n", cnt));
    return cnt;
}

int doRead( void * resp, int size)
{
    int cnt = -1;

    if (rec_fd == -1)
        init_rec();

    while ((cnt = read(rec_fd, resp, size)) < 0) {
        if (cnt != EINTR) {
	    DEBUGMSGTL(("rec:rec_log_error", "Read error = %d fd = %d\n",errno, rec_fd));
            return -1;
	}
    }
    DEBUGMSGTL(("rec:arch", "read %d bytes fd = %d\n", cnt, rec_fd));
    dump_chunk("rec:arch","Read", (const u_char *)resp, cnt);

    return (cnt);
}

int doWriteRead( void * req, int size, _resp * resp)
{
    int cnt = -1;

    if (rec_fd == -1)
        init_rec();

    dump_chunk("rec:arch","WriteRead-write", (const u_char *)req, size);
    if((cnt = write(rec_fd, req, size)) < 0) {
	DEBUGMSGTL(("rec:rec_log_error", "WriteRead write error = %d fd = %d\n",errno, rec_fd));
        close_rec(1);
        return -1;
    }
    DEBUGMSGTL(("rec:arch", "WriteRead write %d bytes fd = %d\n", cnt, rec_fd));

    memset (resp, 0, sizeof(_resp));
    while ((cnt = read(rec_fd, resp, sizeof(_resp))) < 0) {
        if (cnt != EINTR) {
	    DEBUGMSGTL(("rec:rec_log_error", "WriteRead read error = %d fd = %d\n",errno, rec_fd));
            close_rec(1);
            return -1;
	}
        DEBUGMSGTL(("rec:arch", "WriteRead read %d bytes fd = snmpHPILODomain.csnmpHPILODomain.c%d\n", cnt, rec_fd));
    }
    dump_chunk("rec:arch","WriteRead-read", (const u_char *)resp, cnt);
    DEBUGMSGTL(("rec:arch", "WriteRead-read %d bytes\n", cnt));
    return (resp->msg.rcode);
}


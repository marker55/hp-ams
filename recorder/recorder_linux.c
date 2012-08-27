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

#ifdef UTEST
#define snmp_log(level, format ...) fprintf(stderr, format, ## __VA_ARGS)
#define DEBUGMSGTL(level, format, ...) fprintf(stderr, format, ## __VA_ARGS)`
#else
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/output_api.h>
#endif

#define MAXHPILODEVS 8

#define HPILO_CCB "/dev/hpilo/d0ccb"

int fd = -1;

void init_recorder(void)  {

    struct timeval randseed;
    int r, i=0;
    char dev[20];

    if (fd != -1 )
        return;

    gettimeofday(&randseed, NULL);
    srand(randseed.tv_usec);
    /* let's pick a random place to start looking for an avialable channel */
    r = rand();
    do {
        sprintf(dev,"%s%d", HPILO_CCB, ((i+r) % MAXHPILODEVS));
        i++;
    } while (i <= MAXHPILODEVS && 
        ((fd = open(dev, O_RDWR|O_EXCL)) == -1));
    if (i ==  MAXHPILODEVS) {
        snmp_log(LOG_ERR, "Failed: no available HP iLO devices\n");
        fd = -1;
    } else 
        return;
}

int doWrite(ilo_req * req, int size)
{
    int cnt;

    if (fd == -1)
        init_recorder();

    req->hdr.pktsz = size;
    if((cnt = write(fd, req, size)) < 0) {
        return -1;
    }
    DEBUGMSGTL(("recorder:arch", "write %d bytes\n", cnt));
    return cnt;
}

int doRead( void * resp, int size)
{
    int cnt;

    if (fd == -1)
        init_recorder();

    while ((cnt = read(fd, resp, size)) < 0) {
        if (cnt != EINTR) {
	    DEBUGMSGTL(("recorder:arch", "Read error = %d\n",errno));
            return -1;
	}
    }
    DEBUGMSGTL(("recorder:arch", "read %d bytes\n", cnt));
    return (cnt);
}

int doWriteRead( void * req, int size, ilo_resp * resp)
{
    int cnt;

    if (fd == -1)
        init_recorder();

    if((cnt = write(fd, req, size)) < 0) {
	DEBUGMSGTL(("recorder:arch", "WriteRead write error = %d\n",errno));
        return -1;
    }
    DEBUGMSGTL(("recorder:arch", "WriteRead write %d bytes\n", cnt));

    memset (resp, 0, sizeof(ilo_resp));
    while ((cnt = read(fd, resp, sizeof(ilo_resp))) < 0) {
        if (cnt != EINTR) {
	    DEBUGMSGTL(("recorder:arch", "WriteRead read error = %d\n",errno));
            return -1;
	}
        DEBUGMSGTL(("recorder:arch", "WriteRead read %d bytes\n", cnt));
    }

    return (resp->msg.rcode);
}


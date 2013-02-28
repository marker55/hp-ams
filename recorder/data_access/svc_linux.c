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
#include "service.h"

#include "net-snmp/net-snmp-config.h"
#include "net-snmp/library/snmp_impl.h"
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/output_api.h>

extern int proc_select(const struct dirent *);

char * get_sysfs_str(char *);

enum {
    PROC_NAME,
    PROC_PID,
    PROC_PPID,
    PROC_TGID,
    MAX_PROC_ITEMS
};

static const char *proc_items[MAX_PROC_ITEMS] = {
    "Name:",
    "Pid:",
    "PPid:",
    "Tgid:"
};

char *values[MAX_PROC_ITEMS];

extern svc_entry **services;

int getService()
{
    struct dirent **proclist;
    char procname[80];
    char input[80];
    char procEntry[1024];
    int procFd;
    int proccount;
    int i, j, length;
    int pid = 0, ppid = 0, tgid = 0;

    int svccount = 0;
    int execount;

    /* Let's get our blob */
    proccount = scandir("/proc/", &proclist, proc_select, alphasort);
    /* get all proc dirs */
    if ((services = (svc_entry**)malloc(proccount * sizeof(svc_entry))) 
                    == NULL) {
        return 0;
    }
    for (i = 0; i <proccount; i++ ) {
        snprintf(procname, 80, "/proc/%s/fd/0",  proclist[i]->d_name);
        if (readlink(procname, input, 80) <= 0) {
            free(proclist[i]);
            continue;
        }
        if ((strncmp(input, "/dev/null", 9) != 0)  &&
            (strncmp(input, "/dev/console", 12) != 0)) {
            free(proclist[i]);
            continue;
        }

        /*This is a service */
        snprintf(procname, 80, "/proc/%s/status",  proclist[i]->d_name);
        DEBUGMSGTL(("rec:log","service proc  status %s\n",procname));
        if ((procFd = open(procname, O_RDONLY)) == -1) {
            free(proclist[i]);
            continue;
        }
        if (read(procFd, procEntry, 1024) == -1) {
            free(proclist[i]);
            close(procFd);
            continue;
        }
        for (j = 0; j < MAX_PROC_ITEMS; j++) {
            values[j] = strstr(procEntry, proc_items[j]);
            if (values[j] != NULL ) {
                values[j] += (strlen(proc_items[j]) + 1);
            }
        }
        if (values[PROC_PPID] != NULL) {
            if ((ppid = atoi(values[PROC_PPID])) != 1) {
                free(proclist[i]);
                close(procFd);
                continue;
            }
        }
        /* Not daemonized so continue */
        if (values[PROC_PID] != NULL)
            pid = atoi(values[PROC_PID]);
        DEBUGMSGTL(("rec:log","service proc pid = %d \n", pid));
        if (values[PROC_TGID] != NULL)
            tgid = atoi(values[PROC_TGID]);
        DEBUGMSGTL(("rec:log","service proc tgid = %d \n", tgid));
        if (tgid != pid) {
            close(procFd);
            free(proclist[i]);
            continue;
        }

        length = index(values[PROC_NAME], 0x0a) - values[PROC_NAME];
        if ((services[svccount] = (svc_entry*)malloc(sizeof(svc_entry))) 
                                        == NULL) {
            close(procFd);
            free(proclist[i]);
            continue;
        }
        memset(services[svccount], 0, sizeof(svc_entry));
        services[svccount]-> pid = pid;
        services[svccount]-> ppid = ppid;
        services[svccount]-> tgid = tgid;
        services[svccount]->name = malloc(length + 1);
        memset(services[svccount]->name, 0, length + 1);
        strncpy(services[svccount]->name, values[PROC_NAME], length);
        DEBUGMSGTL(("rec:log","service proc name %s\n",  services[svccount]->name));
        /* not a thread */
        services[svccount]->filename = malloc(256);
        memset(services[svccount]->filename, 0, 256);
        snprintf(procname, 80, "/proc/%s/exe",  proclist[i]->d_name);
        if ((execount = readlink(procname, services[svccount]->filename, 256)) 
                        == -1) {
            free(proclist[i]);
            close(procFd);
            if (services[svccount]->filename != NULL )
                free(services[svccount]->filename);
            if (services[svccount]->name != NULL )
                free(services[svccount]->name);
            free(services[svccount]);
            continue;
        }
        DEBUGMSGTL(("rec:log","service file name %s\n", 
                                services[svccount]->filename));
        svccount++;
        free(proclist[i]);
        close(procFd);
    }
    free(proclist);
    return svccount;
}


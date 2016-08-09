#include <net-snmp/net-snmp-config.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

#include <dirent.h>
#include <fcntl.h>

#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <net-snmp/library/container.h>
#include <net-snmp/library/snmp_debug.h>

#include "../cpqHoSWRunningTable.h"
int
cpqhost_arch_swrun_container_load( netsnmp_container *container)
{
    DIR                 *procdir = NULL;
    struct dirent       *procentry_p;
    int			 fd;
    int                  pid, newpid, ppid, rc = 0;
    char                 buf[BUFSIZ],  buf2[BUFSIZ];
    char		command[256], state[10];
     
    cpqHoSWRunningTable_entry *entry;

    procdir = opendir("/proc");
    if ( NULL == procdir ) {
        snmp_log( LOG_ERR, "Failed to open /proc" );
        return -1;
    }

    DEBUGMSGTL(("cpqHost:swrun:container:load", "loading\n"));
    DEBUGMSGTL(("cpqHost:swrun:container:load", "Container=%p\n",container));

    /*
     * Walk through the list of processes in the /proc tree
     */
    while ( NULL != (procentry_p = readdir( procdir ))) {
        pid = atoi( procentry_p->d_name );
        if ( 0 == pid )
            continue;   /* Presumably '.' or '..' */

        /*
         * Now extract the interesting information
         *   from the various /proc{PID}/ interface files
         */

        snprintf( buf, sizeof(buf), "/proc/%d/stat", pid );
        if ((fd = open( buf, O_RDONLY )) < 0) 
		continue;
        memset(buf, 0, sizeof(buf));
        if (read(fd,  buf, BUFSIZ-1) < 0) {
            close(fd);
            continue;
        }

	sscanf(buf, "%d %s %s %d %s", &newpid, &(command[0]), &(state[0]), &ppid, buf2);
	if (ppid == 2) {
                close(fd);
		continue;
        }
	if (ppid == 0) {
                close(fd);
		continue;
        }
	if (pid != newpid) {
                close(fd);
		continue;
        }
        entry = cpqHoSWRunningTable_createEntry(container, (oid)pid);
        if (NULL == entry) {
                close(fd);
		continue;
        }

	memset(entry->cpqHoSWRunningName, 0, sizeof(entry->cpqHoSWRunningName));
	strncpy(entry->cpqHoSWRunningName, &(command[1]), 254);
	entry->cpqHoSWRunningName_len = strlen(command) - 2;

	memset(entry->cpqHoSWRunningDesc, 0, sizeof(entry->cpqHoSWRunningDesc));
	entry->cpqHoSWRunningDesc_len = 0;

	memset(entry->cpqHoSWRunningVersion, 0, sizeof(entry->cpqHoSWRunningVersion));
	entry->cpqHoSWRunningVersion_len = 0;

	memset(entry->cpqHoSWRunningDate, 0, sizeof(entry->cpqHoSWRunningDate));
	entry->cpqHoSWRunningDate_len = sizeof(entry->cpqHoSWRunningDate);

	entry->cpqHoSWRunningMonitor = 1;
	entry->cpqHoSWRunningState = 2;

	entry->cpqHoSWRunningCount = 1;
	entry->cpqHoSWRunningCountMin = 1;
	entry->cpqHoSWRunningCountMax = 1;

	memset(entry->cpqHoSWRunningEventTime, 0, sizeof(entry->cpqHoSWRunningEventTime));
	entry->cpqHoSWRunningEventTime_len = sizeof(entry->cpqHoSWRunningEventTime);

	entry->cpqHoSWRunningStatus = 1;
	entry->cpqHoSWRunningConfigStatus = 1;
	entry->cpqHoSWRunningRedundancyMode = 1;

        rc = CONTAINER_INSERT(container, entry);

        close(fd);
    }
    closedir( procdir );

    DEBUGMSGTL(("cpqHost:swrun:container:load"," loaded %d entries\n",
                (int)CONTAINER_SIZE(container)));

    return(rc);
}


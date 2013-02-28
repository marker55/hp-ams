#define _BSD_SOURCE
#include <stdio.h> 
#include <stdlib.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

#include "ams_rec.h"
#include "recorder.h"
#include "data.h"

#include "net-snmp/net-snmp-config.h"
#include "net-snmp/library/snmp_impl.h"
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/output_api.h>

#define netsnmp_cache void
#include "net-snmp/agent/hardware/memory.h"
#define SNMP_MAXBUF (4 * 1024)
#include "net-snmp/agent/hardware/cpu.h"

int os_usage_reinit = 0;
extern int proc_select(const struct dirent *);
extern void close_rec(int);
extern void LOG_OS_USAGE(void);

REC_AMS_OsUsageData OsUsageData;

void rec_log_os_usage(int regNo, void *unUsed)
{
    float LoadAverage[3];
    int rc;
    int cur, nextpid;
    struct dirent **proclist;
    int i, count, tcount;
    FILE *proc_loadavg;

    DEBUGMSGTL(("rectimer:log", "OS Usage Called %d\n", regNo));

    if (os_usage_reinit)
        LOG_OS_USAGE();

    /* Let's go ahead and set the code for */
    if ((rc = rec_api3(s_ams_rec_handle, REC_CODE_AMS_OS_USAGE))
                != RECORDER_OK) {
        DEBUGMSGTL(("rectimer:log", "SetRecorderFeederCode failed (%d)\n", rc));
        close_rec(1);
        return;
    }

    memset(&OsUsageData, 0, sizeof(OsUsageData));

    OsUsageData.ProcessCount = 
                    scandir("/proc/", &proclist, proc_select, alphasort);
    for (i=0; i < OsUsageData.ProcessCount; i++)
        free(proclist[i]);
    free(proclist);
    if ((proc_loadavg = fopen("/proc/loadavg", "r"))) {
        count = fscanf(proc_loadavg, "%f %f %f %d/%d %d", 
                                     &LoadAverage[0], 
                                     &LoadAverage[1], 
                                     &LoadAverage[2], 
                                     &cur, 
                                     &tcount, 
                                     &nextpid); 
        fclose(proc_loadavg);
    }
    OsUsageData.LoadAverage[0] = LoadAverage[0] * 100; 
    OsUsageData.LoadAverage[1] = LoadAverage[1] * 100; 
    OsUsageData.LoadAverage[2] = LoadAverage[2] * 100; 
    OsUsageData.ThreadCount = tcount;
    OsUsageData.HandleCount = 0;
    DEBUGMSGTL(("rectimer:log","OS Usage "
                "ProcessCount %d, ThreadCount %d, HandleCount %d\n",
                OsUsageData.ProcessCount, OsUsageData.ThreadCount,
                OsUsageData.HandleCount));

    DEBUGMSGTL(("rectimer:log","OS Usage "
                "LoadAvg[0] %d, LoadAvg[1] %d, LoadAvg[2] %d\n",
                OsUsageData.LoadAverage[0], OsUsageData.LoadAverage[1],
                OsUsageData.LoadAverage[2]));

    DEBUGMSGTL(("rectimer:log","OS Usage "
                "LoadAvg[0] %f, LoadAvg[1] %f, LoadAvg[2] %f\n",
                LoadAverage[0], LoadAverage[1], LoadAverage[2]));

    // Log the record
    if ((rc = rec_log(s_ams_rec_handle, (const char*)&OsUsageData, 
                        sizeof(OsUsageData))) != RECORDER_OK) {
        DEBUGMSGTL(("rec:log", "LogRecorderData failed (%d)\n",rc));
    }

    DEBUGMSGTL(("rec:log", "Logged record for code %d\n",
                REC_CODE_AMS_OS_USAGE));
    close_rec(0);
    return;

}


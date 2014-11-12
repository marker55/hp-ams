#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <net-snmp/library/container.h>
#include <net-snmp/library/snmp_debug.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <getopt.h>

#include "cpqLinOsMgmt.h"

#include "common/utils.h"

extern struct cpqLinOsMib    mib;
extern struct cpqLinOsCommon common;
extern struct cpqLinOsSys    sys;
extern struct cpqLinOsMem    mem;
extern struct cpqLinOsSwap   swap;

extern int proc_select(const struct dirent *);

int cpqLinOsCommon_load(netsnmp_cache * cache, void *vmagic)
{
    static struct timeval the_time[2];
    struct timeval *curr_time = &the_time[0];
    struct timeval *prev_time = &the_time[1];
    struct timeval e_time;
    unsigned long long PollInt = 0;

    common.PollFreq = 0;

    gettimeofday(curr_time, NULL);
    if (prev_time->tv_sec > 0) {
        PollInt = 
	    ((curr_time->tv_sec * 1000) + (curr_time->tv_usec / 1000) - 
	    (prev_time->tv_sec * 1000) + (prev_time->tv_usec / 1000)) / 1000;
        common.LastObservedPollCycle = PollInt;
    } else
        common.LastObservedPollCycle = 0;

    common.LastObservedTimeSec = curr_time->tv_sec;
    common.LastObservedTimeMSec = curr_time->tv_usec / 1000;

    prev_time->tv_sec = curr_time->tv_sec;
    prev_time->tv_usec = curr_time->tv_usec;

    return(0);

}

void cpqLinOsCommon_free(netsnmp_cache * cache, void *vmagic)
{
    memset(&common, 0, sizeof(common));
}

int cpqLinOsSys_load(netsnmp_cache * cache, void *vmagic)
{
    static struct timeval the_time[2];
    struct timeval *curr_time = &the_time[0];
    struct timeval *prev_time = &the_time[1];
    struct timeval e_time;
    struct dirent **proclist;

    unsigned long long PollInt = 0;

    long ret_value = -1;
    char *uptime;
    int count;
    int i;
    read_line_t *statlines = NULL;

    gettimeofday(curr_time, NULL);
    if (prev_time->tv_sec > 0) {
        timersub(curr_time, prev_time, &e_time);
    /* interval between samples is in .01 sec */
        PollInt = 
            (e_time.tv_sec*1000 + e_time.tv_usec/1000)/10;
    } else
        PollInt = 0;

    DEBUGMSGTL(("cpqLinOsMgmt", "PollInt = %llu\n", PollInt));
    uptime = get_sysfs_str("/proc/uptime");

    DEBUGMSGTL(("cpqLinOsMgmt", "uptime = %s\n", uptime));

    sys.SystemUpTime_len = index(uptime,'.') - uptime;
    DEBUGMSGTL(("cpqLinOsMgmt", "uptime_len = %ld\n", sys.SystemUpTime_len));
    
    strncpy(sys.SystemUpTime, uptime, sys.SystemUpTime_len);

    free(uptime);
    if ((statlines = ReadFileLines("/proc/stat")) != NULL) {
        for (i = 0; strncmp(statlines->line[i], "ctxt", 4); i++) { }

        DEBUGMSGTL(("cpqLinOsMgmt", "line = %s\n", statlines->line[i]));
        sys.ContextSwitchPrev = sys.ContextSwitchCur;
        sscanf(statlines->line[i], "ctxt %llu", &sys.ContextSwitchCur);
        if ((sys.ContextSwitchPrev != 0)  && (PollInt != 0)) 
            sys.ContextSwitchesPersec =
            (sys.ContextSwitchCur - sys.ContextSwitchPrev)/(PollInt/100);
        else 
            sys.ContextSwitchesPersec = 0;

        sys.Processes = scandir("/proc/", &proclist, proc_select, alphasort);
        for (i = 0;i < sys.Processes; i++)
            free(proclist[i]);
        free(proclist);

        prev_time->tv_sec = curr_time->tv_sec;
        prev_time->tv_usec = curr_time->tv_usec;

        free(statlines->read_buf);
        free(statlines);
    }
    return(0);
}

void cpqLinOsSys_free(netsnmp_cache * cache, void *vmagic)
{
    memset(&sys, 0, sizeof(sys));
}

int cpqLinOsMem_load(netsnmp_cache * cache, void *vmagic)
{
    read_line_t *memlines = NULL;
    int i;
    long ret_value = -1;

    if ((memlines = ReadFileLines("/proc/meminfo")) != NULL) {
        for (i = 0; strncmp(memlines->line[i], "MemTotal:", 9); i++) { }
            sscanf(memlines->line[i], "MemTotal: %llu", &mem.Total);

        for (i = 0; strncmp(memlines->line[i], "MemFree:", 8); i++) { }
            sscanf(memlines->line[i], "MemFree: %llu", &mem.Free);

        for (i = 0; strncmp(memlines->line[i], "SwapTotal:", 10); i++) { }
            sscanf(memlines->line[i], "SwapTotal: %llu", &mem.SwapTotal);

        for (i = 0; strncmp(memlines->line[i], "SwapFree:", 9); i++) { }
            sscanf(memlines->line[i], "SwapFree: %llu", &mem.SwapFree);

        for (i = 0; strncmp(memlines->line[i], "Cached:", 7); i++) { }
            sscanf(memlines->line[i], "Cached: %llu", &mem.Cached);

        for (i = 0; strncmp(memlines->line[i], "SwapCached:", 11); i++) { }
            sscanf(memlines->line[i], "SwapCached: %llu", &mem.SwapCached);

        for (i = 0; strncmp(memlines->line[i], "Active:", 7); i++) { }
            sscanf(memlines->line[i], "Active: %llu", &mem.Active);

        free(memlines->read_buf);
        free(memlines);
    }
    return(0);
}

void cpqLinOsMem_free(netsnmp_cache * cache, void *vmagic)
{
    memset(&mem, 0, sizeof(mem));
}

int cpqLinOsSwap_load(netsnmp_cache * cache, void *vmagic)
{
    static struct timeval the_time[2];
    struct timeval e_time;
    struct timeval *curr_time = &the_time[0];
    struct timeval *prev_time = &the_time[1];
    read_line_t *vmlines = NULL;
    int i;
    unsigned long long  PollInt = 0;
    long ret_value = -1;

    gettimeofday(curr_time, NULL);
    if (prev_time->tv_sec > 0) {
        timersub(curr_time, prev_time, &e_time);
    /* interval between samples is in .01 sec */
        PollInt = 
            (e_time.tv_sec*1000 + e_time.tv_usec/1000)/10;
    } else
        PollInt = 0;

    if ((vmlines = ReadFileLines("/proc/vmstat")) != NULL) {

        for (i = 0; strncmp(vmlines->line[i], "pgpgin ", 7); i++) { }
            sscanf(vmlines->line[i], "pgpgin %llu", &swap.SwapInCur);
            if ((swap.SwapInCur != 0) && (PollInt != 0)) 
                swap.SwapInPerSec = 
                    (swap.SwapInCur - swap.SwapInPrev)/(PollInt/100); 
            swap.SwapInPrev = swap.SwapInCur;

        for (i = 0; strncmp(vmlines->line[i], "pgpgout ", 8); i++) { }
            sscanf(vmlines->line[i], "pgpgout %llu", &swap.SwapOutCur);
            if ((swap.SwapOutCur != 0) && (PollInt != 0)) 
                swap.SwapOutPerSec = 
                    (swap.SwapOutCur - swap.SwapOutPrev)/(PollInt/100); 
            swap.SwapOutPrev = swap.SwapOutCur;
    
        for (i = 0; strncmp(vmlines->line[i], "pswpin ", 7); i++) { }
            sscanf(vmlines->line[i], "pswpin %llu", &swap.PageSwapInCur);
            if ((swap.PageSwapInCur != 0) && (PollInt != 0)) 
                swap.PageSwapInPerSec = 
			        (swap.PageSwapInCur - swap.PageSwapInPrev)/(PollInt/100); 
            swap.PageSwapInPrev = swap.PageSwapInCur;
    
        for (i = 0; strncmp(vmlines->line[i], "pswpout ", 8); i++) { }
            sscanf(vmlines->line[i], "pswpout %llu", &swap.PageSwapOutCur);
            if ((swap.PageSwapOutCur != 0) && (PollInt != 0)) 
                swap.PageSwapOutPerSec = 
			        (swap.PageSwapOutCur - swap.PageSwapOutPrev)/(PollInt/100); 
            swap.PageSwapOutPrev = swap.PageSwapOutCur;
    
        for (i = 0; strncmp(vmlines->line[i], "pgfault ", 8); i++) { }
            sscanf(vmlines->line[i], "pgfault %llu", &swap.MinFltCur);
            if ((swap.MinFltCur != 0) && (PollInt != 0)) 
                swap.MinFltPerSec = (swap.MinFltCur - swap.MinFltPrev)/(PollInt/100); 
            swap.MinFltPrev = swap.MinFltCur;
    
        for (i = 0; strncmp(vmlines->line[i], "pgmajfault ", 11); i++) { }
            sscanf(vmlines->line[i], "pgmajfault %llu", &swap.MajFltCur);
            if ((swap.MajFltCur != 0) && (PollInt != 0)) 
                swap.MajFltPerSec = (swap.MajFltCur - swap.MajFltPrev)/(PollInt/100); 
            swap.MajFltPrev = swap.MajFltCur;
    
        prev_time->tv_sec = curr_time->tv_sec;
        prev_time->tv_usec = curr_time->tv_usec;
    
        free(vmlines->read_buf);
        free(vmlines);
    }
    return(0);
}
void cpqLinOsSwap_free(netsnmp_cache * cache, void *vmagic)
{
    memset(&swap, 0, sizeof(swap));
}


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

#include "cpqLinOsProcessorTable.h"

#include "common/utils.h"


int netsnmp_arch_processor_container_load(netsnmp_container* container)
{
    cpqLinOsProcessorTable_entry *entry;
    netsnmp_index tmp;

    oid oid_index[2];
    oid  index = 0;
    unsigned long long interval = 0;

    read_line_t *statlines = NULL;
    read_line_t *intrlines = NULL;
    char *b;
    long  rc = 0;
    static int   num_cpuline_elem = 0;
    int num_cpus = 0;
    unsigned long long *bigArray = NULL;
    int bigArray_sz;
    int cnt, item;
    struct timeval curr_time;
    char * numbers;

    gettimeofday(&curr_time, NULL);
    
    DEBUGMSGTL(("linosprocessor:container:load", "loading\n"));

    if ((intrlines = ReadFileLines("/proc/interrupts")) != NULL) {
        num_cpus = (strlen(intrlines->line[0]) - 12) / 11;

        bigArray_sz = sizeof(unsigned long long) * (num_cpus + 1);
        bigArray = malloc(bigArray_sz);
        memset(bigArray, 0, bigArray_sz);
        for (cnt = 1; cnt < intrlines->count; cnt ++) { 
            numbers = intrlines->line[cnt] + 5; /*skip over interrupt */
            for (item = 1; item <= num_cpus; item++) {
                char *endptr;
                bigArray[item] += strtoll(numbers, &endptr, 10);
                numbers = endptr;
            }
         
        }
        for (item = 1; item <= num_cpus; item++)
            bigArray[0] +=bigArray[item];
        free(intrlines->read_buf);
        free(intrlines);
    }

    if ((statlines = ReadFileLines("/proc/stat")) != NULL) {
        cnt = 0;
        while (!strncmp(statlines->line[cnt], "cpu", 3)) {
            unsigned long long cusell = 0, cicell = 0, csysll = 0, cidell = 0,
                            ciowll = 0, cirqll = 0, csoftll = 0, cstealll = 0,
                            cguestll = 0, cguest_nicell = 0;
            b = statlines->line[cnt++];
            if (b[3] == ' ') {
                b += 4; /* Skip "cpu " */
            } else {
                sscanf( b, "cpu%ld", &index );
                index++;
                if (b[4] == ' ')
                    b += 5; /* Skip "cpuN " */
                else if (b[5] == ' ')
                    b += 6; /* Skip "cpuNN " */
                else if (b[6] == ' ')
                    b += 7; /* Skip "cpuNNN " */
                else 
                    b += 8; /* Skip "cpuNNNN " */
            }
    
            num_cpuline_elem = sscanf(b, 
                                "%llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
                                &cusell, &cicell, &csysll, &cidell, &ciowll, 
                                &cirqll, &csoftll, &cstealll, &cguestll, 
                                &cguest_nicell);
    
            oid_index[0] = index;
            tmp.len = 1;
            tmp.oids = &oid_index[0];
    
            entry = CONTAINER_FIND(container, &tmp);
            if (entry) {
                struct timeval e_time;
    
                entry->prev_time.tv_sec = entry->curr_time.tv_sec;
                entry->prev_time.tv_usec = entry->curr_time.tv_usec;
    
                entry->prev_interrupts = entry->curr_interrupts;
    
                entry->prev_user_tics = entry->curr_user_tics;
                entry->prev_user_lp_tics = entry->curr_user_lp_tics;
                entry->prev_sys_tics  = entry->curr_sys_tics;
                entry->prev_idle_tics = entry->curr_idle_tics;
                entry->prev_iowait_tics = entry->curr_iowait_tics;
                entry->prev_irq_tics  = entry->curr_irq_tics;
                entry->prev_softirq_tics = entry->curr_softirq_tics;
    
                if (bigArray != NULL) 
                    entry->curr_interrupts = bigArray[index];
                entry->curr_user_tics =    cusell;
                entry->curr_user_lp_tics = cicell;
                entry->curr_sys_tics  =    csysll;
                entry->curr_idle_tics =    cidell;
                /* kernel 2.6 */
                if (num_cpuline_elem >= 5) {
                    entry->curr_iowait_tics   = (unsigned long long)ciowll;
                    entry->curr_irq_tics = (unsigned long long)cirqll;
                    entry->curr_softirq_tics   = (unsigned long long)csoftll;
                }
    
                entry->curr_time.tv_sec = curr_time.tv_sec;
                entry->curr_time.tv_usec = curr_time.tv_usec;
                timersub(&entry->curr_time, &entry->prev_time, &e_time);
                /* interval between samples is in .01 sec */
                interval = (e_time.tv_sec*1000 + e_time.tv_usec/1000)/10;
             
                if (interval) {
                    entry->cpqLinOsCpuInterruptsPerSec = 
                        (entry->curr_interrupts - entry->prev_interrupts)/
                        (interval/100);
    
                    if ((index == 0) || (num_cpus == 0)){
                        entry->cpqLinOsCpuTimePercent = 100 - 
                                ((100*((entry->curr_idle_tics - entry->prev_idle_tics)/num_cpus))
                                /interval);
                        entry->cpqLinOsCpuUserTimePercent =  
                                ((100*(((entry->curr_user_tics - entry->prev_user_tics) + 
                                        (entry->curr_user_lp_tics - entry->prev_user_lp_tics))/num_cpus))
                                /interval);
                        entry->cpqLinOsCpuPrivilegedTimePercent =  
                                ((100*((entry->curr_sys_tics - entry->prev_sys_tics)/num_cpus))
                                /interval);
                    } else {
                        entry->cpqLinOsCpuTimePercent = 100 -
                            ((100*(entry->curr_idle_tics - entry->prev_idle_tics))/interval);
                        entry->cpqLinOsCpuUserTimePercent =  
                            ((100*((entry->curr_user_tics - entry->prev_user_tics) +
                                    (entry->curr_user_lp_tics - entry->prev_user_lp_tics)))/interval);
                        entry->cpqLinOsCpuPrivilegedTimePercent = 
                            ((100*(entry->curr_sys_tics - entry->prev_sys_tics))/interval);
    
                    }
                }
            } else {
                entry = cpqLinOsProcessorTable_createEntry(container, (oid)index);
    
                if (index == 0) {
                    strcpy(entry->cpqLinOsCpuInstance, "Total");
                    entry->cpqLinOsCpuInstance_len = strlen(entry->cpqLinOsCpuInstance);
                } else {
                    entry->cpqLinOsCpuInstance_len = 
                            sprintf(entry->cpqLinOsCpuInstance,"Processor %ld", index);
                }
                entry->curr_interrupts = 0;
                entry->prev_user_tics = 0;
                entry->prev_user_lp_tics = 0;
                entry->prev_sys_tics  = 0;
                entry->prev_idle_tics =0;
                entry->prev_iowait_tics =0;
                entry->prev_irq_tics  =0;
                entry->prev_softirq_tics =0;
    
                if (bigArray != NULL) 
                    entry->curr_interrupts = bigArray[index];
                entry->curr_user_tics =    cusell;
                entry->curr_user_lp_tics = cicell;
                entry->curr_sys_tics  =    csysll;
                entry->curr_idle_tics =    cidell;
                /* kernel 2.6 */
                if (num_cpuline_elem >= 5) {
                    entry->curr_iowait_tics   = (unsigned long long)ciowll;
                    entry->curr_irq_tics = (unsigned long long)cirqll;
                    entry->curr_softirq_tics   = (unsigned long long)csoftll;
                }
                entry->curr_time.tv_sec = curr_time.tv_sec;
                entry->curr_time.tv_usec = curr_time.tv_usec;
                rc = CONTAINER_INSERT(container, entry);
            }
    
        }
        free(statlines->read_buf);
        free(statlines);
    }
    if (bigArray != NULL) 
        free(bigArray);
    
    return(rc);
}


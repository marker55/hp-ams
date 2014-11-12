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

#include "cpqLinOsDiskTable.h"

#include "common/utils.h"

int netsnmp_arch_linosdiskio_container_load(netsnmp_container* container)
{
    cpqLinOsDiskTable_entry *entry;
    netsnmp_index tmp;
    oid oid_index[2];

    long  rc = 0;
    int i;
    struct timeval curr_time;
    struct timeval prev_time;
    struct timeval e_time;
    int interval;

    read_line_t *disklines = NULL;
    int maj, min;
    char diskname[16];

    gettimeofday(&curr_time, NULL);
    DEBUGMSGTL(("linosdiskio:container:load", "loading\n"));

    DEBUGMSGTL(("linosdiskio:container:load", "Container=%p\n",container));

    if ((disklines = ReadFileLines("/proc/diskstats")) != NULL) {

        for (i =0; i < disklines->count;i++) {
            unsigned long long reads, rmerged, read_sec, read_ms, 
                                writes, wmerged, write_sec, write_ms, 
                                io_inflight, io_ms, io_ms_weightd;

            if (sscanf(disklines->line[i],"%d %d %s ", &maj, &min, diskname) != 3)
                continue;
            if (min % 16)
                continue;
            if (!strncmp(diskname,"sd", 2) || 
                !strncmp(diskname,"hd", 2) ||
                !strncmp(diskname,"cciss", 5)) {
                sscanf(disklines->line[i],
                    "%d %d %s %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu", 
                    &maj, &min, diskname, 
                    &reads, &rmerged, &read_sec, &read_ms, 
                    &writes, &wmerged, &write_sec, &write_ms, 
                    &io_inflight, &io_ms, &io_ms_weightd);

                oid_index[0] = maj;
                oid_index[1] = min;
                tmp.len = 2;
                tmp.oids = &oid_index[0];
                entry = CONTAINER_FIND(container, &tmp);
                if (entry) {
                    entry->prev_time.tv_sec = entry->curr_time.tv_sec;
                    entry->prev_time.tv_usec = entry->curr_time.tv_usec;
    
                    entry->prev_reads = entry->curr_reads;
                    entry->prev_read_merge = entry->curr_read_merge;
                    entry->prev_read_sectors = entry->curr_read_sectors;
                    entry->prev_read_ms = entry->curr_read_ms;
                    entry->prev_writes = entry->curr_writes;
                    entry->prev_write_merge = entry->curr_write_merge;
                    entry->prev_write_sectors = entry->curr_write_sectors;
                    entry->prev_write_ms = entry->curr_write_ms;
     
                    entry->curr_reads = reads;
                    entry->curr_read_merge = rmerged;
                    entry->curr_read_sectors = read_sec;
                    entry->curr_read_ms = read_ms;
                    entry->curr_writes = writes;
                    entry->curr_write_merge = wmerged;
                    entry->curr_write_sectors = write_sec;
                    entry->curr_write_ms = write_ms;
     
                    entry->curr_time.tv_sec = curr_time.tv_sec;
                    entry->curr_time.tv_usec = curr_time.tv_usec;
                    timersub(&entry->curr_time, &entry->prev_time, &e_time);
                    /* interval between samples is in .01 sec */
                    interval = (e_time.tv_sec*1000 + e_time.tv_usec/1000)/10;
     
                    if (interval) {
                        entry->cpqLinOsDiskReadIos = 
                                (entry->curr_reads - entry->prev_reads);
                        entry->cpqLinOsDiskReadMerges =
                                (entry->curr_read_merge - entry->prev_read_merge);
                        entry->cpqLinOsDiskReadSectors = 
                                (entry->curr_read_sectors - entry->prev_read_sectors);
                        entry->cpqLinOsDiskReadDurationMs = 
                                (entry->curr_read_ms - entry->prev_read_ms);
    
                        entry->cpqLinOsDiskReadIosPerSec = 
                                entry->cpqLinOsDiskReadIos/(interval/100);
                        entry->cpqLinOsDiskReadSectorsPerSec = 
                                entry->cpqLinOsDiskReadSectors/(interval/100);
                        if (entry->cpqLinOsDiskReadIos)
                            entry->cpqLinOsDiskReadDurationMsPerIos = 
                                entry->cpqLinOsDiskReadDurationMs/entry->cpqLinOsDiskReadIos;
                        else
                            entry->cpqLinOsDiskReadDurationMsPerIos = 0;
    
                        entry->cpqLinOsDiskWriteIos = 
                                (entry->curr_writes - entry->prev_writes);
                        entry->cpqLinOsDiskWriteMerges =
                                (entry->curr_write_merge - entry->prev_write_merge);
                        entry->cpqLinOsDiskWriteSectors = 
                                (entry->curr_write_sectors - entry->prev_write_sectors);
                        entry->cpqLinOsDiskWriteDurationMs = 
                                (entry->curr_write_ms - entry->prev_write_ms);
    
                        entry->cpqLinOsDiskWriteIosPerSec = 
                                entry->cpqLinOsDiskWriteIos/(interval/100);
                        entry->cpqLinOsDiskWriteSectorsPerSec = 
                                entry->cpqLinOsDiskWriteSectors/(interval/100);
                        if (entry->cpqLinOsDiskWriteIos)
                            entry->cpqLinOsDiskWriteDurationMsPerIos = 
                                entry->cpqLinOsDiskWriteDurationMs/entry->cpqLinOsDiskWriteIos;
                        else
                            entry->cpqLinOsDiskWriteDurationMsPerIos = 0;
                    }
    
                } else {
                    entry = cpqLinOsDiskTable_createEntry(container, (oid)maj, (oid)min);
                    entry->cpqLinOsDiskScsiIndex = 0;
                    entry->cpqLinOsDiskMajorIndex = maj;
                    entry->cpqLinOsDiskMinorIndex = min;
                    entry->cpqLinOsDiskName_len = strlen(diskname);
                    strncpy(entry->cpqLinOsDiskName, diskname, strlen(diskname));
     
                    entry->prev_reads = 0;
                    entry->prev_read_merge = 0;
                    entry->prev_read_sectors = 0;
                    entry->prev_read_ms = 0;
                    entry->prev_writes = 0;
                    entry->prev_write_merge = 0;
                    entry->prev_write_sectors = 0;
                    entry->prev_write_ms = 0;
     
                    entry->curr_reads = reads;
                    entry->curr_read_merge = rmerged;
                    entry->curr_read_sectors = read_sec;
                    entry->curr_read_ms = read_ms;
                    entry->curr_writes = writes;
                    entry->curr_write_merge = wmerged;
                    entry->curr_write_sectors = write_sec;
                    entry->curr_write_ms = write_ms;
     
                    entry->cpqLinOsDiskReadIos = 0;
                    entry->cpqLinOsDiskReadMerges = 0;
                    entry->cpqLinOsDiskReadSectors = 0;
                    entry->cpqLinOsDiskReadDurationMs = 0;
    
                    entry->cpqLinOsDiskWriteIos = 0;
                    entry->cpqLinOsDiskWriteMerges =0;
                    entry->cpqLinOsDiskWriteSectors = 0;
                    entry->cpqLinOsDiskWriteDurationMs = 0;
    
                    entry->cpqLinOsDiskReadIosPerSec = 0;
                    entry->cpqLinOsDiskReadSectorsPerSec = 0;
                    entry->cpqLinOsDiskReadDurationMsPerIos = 0;
    
                    entry->cpqLinOsDiskWriteIosPerSec = 0;
                    entry->cpqLinOsDiskWriteSectorsPerSec = 0;
                    entry->cpqLinOsDiskWriteDurationMsPerIos = 0;
    
                    entry->curr_time.tv_sec = curr_time.tv_sec;
                    entry->curr_time.tv_usec = curr_time.tv_usec;
                    rc = CONTAINER_INSERT(container, entry);
                    DEBUGMSGTL(("linosdiskio:container:load", "Entry created\n"));
                }
            }
        } 
        free(disklines->read_buf);
        free(disklines);
    }    
    
    return(rc);
}


#define _BSD_SOURCE
#include <stdio.h> 
#include <stdlib.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "ams_rec.h"
#include "recorder.h"
#include "data.h"

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/output_api.h>

#define netsnmp_cache void
#define SNMP_MAXBUF (4 * 1024)
#include "net-snmp/agent/hardware/memory.h"
#include "net-snmp/agent/hardware/cpu.h"

REC_AMS_MemoryUsageData MemoryUsageData;
int proc_usage_reinit = 0;
int mem_usage_reinit = 0;

extern void close_record(int);
extern void LOG_MEMORY_USAGE(void);
extern void LOG_PROCESSOR_USAGE(void);

void rec_log_memory_usage(int regNo, void* unUsed)
{
    int rc;
    netsnmp_memory_info* mem_info;
    size_t blob_sz = sizeof(REC_CODE_AMS_MEMORY_USAGE);

    DEBUGMSGTL(("rectimer:log", "Memmory Usage Called %d\n", regNo));
    
    if (mem_usage_reinit) 
        LOG_MEMORY_USAGE();

    memset(&MemoryUsageData, 0, blob_sz);

    /* Let's go ahead and set the code for */
    if ((rc = rec_api3(s_ams_rec_handle, REC_CODE_AMS_MEMORY_USAGE))
                != RECORDER_OK) {
        DEBUGMSGTL(("record:log", "SetRecorderFeederCode failed (%d)\n", rc));
        close_rec(1);
        return;
    }

    netsnmp_memory_load();
    mem_info = netsnmp_memory_get_byIdx(NETSNMP_MEM_TYPE_PHYSMEM, 0);
    if (mem_info) {
        MemoryUsageData.PhysicalTotal = (UINT64)mem_info->size * 
                                        (UINT64)mem_info->units;
        MemoryUsageData.PhysicalFree = (UINT64)mem_info->free * 
                                       (UINT64)mem_info->units;
        //MemoryUsageData.PhysicalTotal = mem_info->size;
        //MemoryUsageData.PhysicalTotal *= mem_info->units;

        //MemoryUsageData.PhysicalFree = mem_info->free;
        //MemoryUsageData.PhysicalFree *= mem_info->units;
    }

    mem_info = netsnmp_memory_get_byIdx(NETSNMP_MEM_TYPE_SWAP, 0);
    if (mem_info) {
        MemoryUsageData.VirtualTotal = (UINT64)mem_info->size * 
                                       (UINT64)mem_info->units;
        MemoryUsageData.VirtualFree = (UINT64)mem_info->free * 
                                      (UINT64)mem_info->units;

        //MemoryUsageData.VirtualFree = mem_info->free;
        //MemoryUsageData.VirtualFree *= mem_info->units;
    }

    // Log the record
    if ((rc = rec_api6(s_ams_rec_handle, (const char*)&MemoryUsageData,
                        sizeof(MemoryUsageData))) != RECORDER_OK) {
        DEBUGMSGTL(("record:log", "LogRecorderData failed (%d)\n",rc));
    }

    DEBUGMSGTL(("record:log", "Logged record for code %d\n",
                REC_CODE_AMS_MEMORY_USAGE));
    close_rec(0);
    return;

}

void rec_log_proc_usage(unsigned int regNo, void *cpucount)
{
    int rc;
    int i;
    REC_AMS_ProcessorUsageData *ProcessorUsageData;
    static void *blob = NULL;
    netsnmp_cpu_info* pInfo;
    size_t blob_sz;
    int *pcount;
    int count;
    pcount  = cpucount;
    count = *pcount;

    DEBUGMSGTL(("recordtimer:log", "Processor Usage Called %d\n", regNo));
    blob_sz = sizeof(REC_AMS_ProcessorUsageData)*count;
    
    if (proc_usage_reinit) 
        LOG_PROCESSOR_USAGE();

    /* Let's go ahead and set the code for */
    if ((rc = rec_api3(s_ams_rec_handle, REC_CODE_AMS_PROCESSOR_USAGE))
                != RECORDER_OK) {
        DEBUGMSGTL(("record:log", "SetRecorderFeederCode failed (%d)\n", rc));
        close_rec(1);
        return;
    }

    if (blob == NULL) 
        if ((blob = malloc(blob_sz)) == NULL ) {
            DEBUGMSGTL(("record:log", "Processor Usage Unable to malloc() %ld blob\n", 
                                blob_sz));
            close_rec(1);
            return;
        }
    memset(blob, 0, blob_sz);

    ProcessorUsageData = blob;
    netsnmp_cpu_load();
    pInfo = netsnmp_cpu_get_first();
    for (i = 0; i <  count && pInfo; ++i) {
	if (pInfo->user_ticks != 0)
            ProcessorUsageData->User = 
                    pInfo->total_ticks * 100 / pInfo->user_ticks;
	if (pInfo->kern_ticks != 0)
            ProcessorUsageData->Kernel = 
                    pInfo->total_ticks * 100 / pInfo->kern_ticks;
        ProcessorUsageData += sizeof(ProcessorUsageData);
        pInfo = netsnmp_cpu_get_next(pInfo);
    }

    // Log the record
    if ((rc = rec_api6(s_ams_rec_handle, (const char*)blob,
                        blob_sz)) != RECORDER_OK) {
        DEBUGMSGTL(("record:log", "LogRecorderData failed (%d)\n",rc));
    }

    DEBUGMSGTL(("record:log", "Logged record for code %d\n",
                REC_CODE_AMS_OS_USAGE));
    close_rec(0);
    return;

}

/*
 * Note: this file originally auto-generated by mib2c using
 *  $
 */
#ifndef CPQLINOSPROCESSORTABLE_H
#define CPQLINOSPROCESSORTABLE_H

/** Typical data structure for a row entry */
typedef struct cpqLinOsProcessorTable_entry_s {
    netsnmp_index   oid_index;

    /*
     * Index values
     */
    oid            cpqLinOsCpuIndex;

    /*
     * Column values
     */
    char            cpqLinOsCpuInstance[64];
    size_t          cpqLinOsCpuInstance_len;
    long            cpqLinOsCpuInterruptsPerSec;
    long            cpqLinOsCpuTimePercent;
    long            cpqLinOsCpuUserTimePercent;
    long            cpqLinOsCpuPrivilegedTimePercent;

    unsigned long long curr_interrupts;
    unsigned long long curr_user_tics;
    unsigned long long curr_user_lp_tics;
    unsigned long long curr_sys_tics;
    unsigned long long curr_idle_tics;
    unsigned long long curr_iowait_tics;
    unsigned long long curr_irq_tics;
    unsigned long long curr_softirq_tics;

    unsigned long long prev_interrupts;
    unsigned long long prev_user_tics;
    unsigned long long prev_user_lp_tics;
    unsigned long long prev_sys_tics;
    unsigned long long prev_idle_tics;
    unsigned long long prev_iowait_tics;
    unsigned long long prev_irq_tics;
    unsigned long long prev_softirq_tics;

    struct timeval prev_time;
    struct timeval curr_time;

    time_t	    last_tic;

    int             valid;
} cpqLinOsProcessorTable_entry;

/*
 * function declarations 
 */
cpqLinOsProcessorTable_entry *
cpqLinOsProcessorTable_createEntry(netsnmp_container * container,
                                   oid cpqLinOsCpuIndex);
void            init_cpqLinOsProcessorTable(void);
void            initialize_table_cpqLinOsProcessorTable(void);
Netsnmp_Node_Handler cpqLinOsProcessorTable_handler;

/*
 * column number definitions for table cpqLinOsProcessorTable 
 */
#define COLUMN_CPQLINOSCPUINDEX		        1
#define COLUMN_CPQLINOSCPUINSTANCE	        2
#define COLUMN_CPQLINOSCPUINTERRUPTSPERSEC      3
#define COLUMN_CPQLINOSCPUTIMEPERCENT	        4
#define COLUMN_CPQLINOSCPUUSERTIMEPERCENT       7
#define COLUMN_CPQLINOSCPUPRIVILEGEDTIMEPERCENT	8

#endif                          /* CPQLINOSPROCESSORTABLE_H */
/*
 * Note: this file originally auto-generated by mib2c using
 *  : mib2c.container.conf 17798 2009-10-27 06:44:54Z magfr $
 */
#ifndef CPQIDECONTROLLERTABLE_H
#define CPQIDECONTROLLERTABLE_H

extern oid       cpqIdeControllerTable_oid[];
extern size_t    cpqIdeControllerTable_oid_len;

typedef struct cpqIdeControllerTable_entry_s {
    netsnmp_index   oid_index;

    /*
     * Index values 
     */
    oid            cpqIdeControllerIndex;

    /*
     * Column values 
     */
    long            cpqIdeControllerOverallCondition;
    char            cpqIdeControllerModel[80];
    size_t          cpqIdeControllerModel_len;
    char            cpqIdeControllerFwRev[27];
    size_t          cpqIdeControllerFwRev_len;
    long            cpqIdeControllerSlot;
    long            cpqIdeControllerStatus;
    long            cpqIdeControllerCondition;
    char            cpqIdeControllerSerialNumber[81];
    size_t          cpqIdeControllerSerialNumber_len;
    char            cpqIdeControllerHwLocation[128];
    size_t          cpqIdeControllerHwLocation_len;
    char            cpqIdeControllerPciLocation[128];
    size_t          cpqIdeControllerPciLocation_len;

    int             valid;
} cpqIdeControllerTable_entry;

/*
 * function declarations 
 */
cpqIdeControllerTable_entry *
    cpqIdeControllerTable_createEntry(netsnmp_container *, oid);

void            init_cpqIdeControllerTable(void);
void            initialize_table_cpqIdeControllerTable(void);
Netsnmp_Node_Handler cpqIdeControllerTable_handler;

/*
 * column number definitions for table cpqIdeControllerTable 
 */
#define COLUMN_CPQIDECONTROLLERINDEX		1
#define COLUMN_CPQIDECONTROLLEROVERALLCONDITION		2
#define COLUMN_CPQIDECONTROLLERMODEL		3
#define COLUMN_CPQIDECONTROLLERFWREV		4
#define COLUMN_CPQIDECONTROLLERSLOT		5
#define COLUMN_CPQIDECONTROLLERSTATUS		6
#define COLUMN_CPQIDECONTROLLERCONDITION		7
#define COLUMN_CPQIDECONTROLLERSERIALNUMBER		8
#define COLUMN_CPQIDECONTROLLERHWLOCATION		9
#define COLUMN_CPQIDECONTROLLERPCILOCATION		10
#endif                          /* CPQIDECONTROLLERTABLE_H */

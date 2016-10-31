/*
 * Note: this file originally auto-generated by mib2c using
 *  : mib2c.container.conf 17695 2009-07-21 12:22:18Z dts12 $
 */
#ifndef CPQFCAHOSTCNTLRTABLE_H
#define CPQFCAHOSTCNTLRTABLE_H

extern int       FcaCondition;
extern oid       cpqFcaHostCntlrTable_oid[];
extern size_t    cpqFcaHostCntlrTable_oid_len;

/** Typical data structure for a row entry */
typedef struct cpqFcaHostCntlrTable_entry_s {
    netsnmp_index   oid_index;

    /*
     *       Index values
     */
    oid            cpqFcaHostCntlrIndex;

    /*
     * Column values
     */
    unsigned int    cpqFcaHostCntlrSlot;
    long            cpqFcaHostCntlrModel;
    long            cpqFcaHostCntlrStatus;
    long            cpqFcaHostCntlrCondition;
    char            cpqFcaHostCntlrWorldWideName[16];
    size_t          cpqFcaHostCntlrWorldWideName_len;
    char            cpqFcaHostCntlrStorBoxList[32];
    size_t          cpqFcaHostCntlrStorBoxList_len;
    long            cpqFcaHostCntlrOverallCondition;
    char            cpqFcaHostCntlrTapeCntlrList[32];
    size_t          cpqFcaHostCntlrTapeCntlrList_len;
    char            cpqFcaHostCntlrSerialNumber[128];
    size_t          cpqFcaHostCntlrSerialNumber_len;
    char            cpqFcaHostCntlrHwLocation[256];
    size_t          cpqFcaHostCntlrHwLocation_len;
    char            cpqFcaHostCntlrWorldWidePortName[16];
    size_t          cpqFcaHostCntlrWorldWidePortName_len;
    char            cpqFcaHostCntlrFirmwareVersion[256];
    size_t          cpqFcaHostCntlrFirmwareVersion_len;
    char            cpqFcaHostCntlrOptionRomVersion[256];
    size_t          cpqFcaHostCntlrOptionRomVersion_len;
    char            cpqFcaHostCntlrPciLocation[256];
    size_t          cpqFcaHostCntlrPciLocation_len;

    long            oldStatus;
    char            host[20];
    oid             cpqHoFwVerIndex;

    int             valid;
} cpqFcaHostCntlrTable_entry;
/*
 * function declarations
 */
cpqFcaHostCntlrTable_entry *
        cpqFcaHostCntlrTable_createEntry(netsnmp_container * container,
                                   oid cpqFcaHostCntlrIndex);

void            init_cpqFcaHostCntlrTable(void);
void            initialize_table_cpqFcaHostCntlrTable(void);
Netsnmp_Node_Handler cpqFcaHostCntlrTable_handler;



/*
 * column number definitions for table cpqFcaHostCntlrTable 
 */
#define COLUMN_CPQFCAHOSTCNTLRINDEX		1
#define COLUMN_CPQFCAHOSTCNTLRSLOT		2
#define COLUMN_CPQFCAHOSTCNTLRMODEL		3
#define COLUMN_CPQFCAHOSTCNTLRSTATUS		4
#define COLUMN_CPQFCAHOSTCNTLRCONDITION		5
#define COLUMN_CPQFCAHOSTCNTLRWORLDWIDENAME		6
#define COLUMN_CPQFCAHOSTCNTLRSTORBOXLIST		7
#define COLUMN_CPQFCAHOSTCNTLROVERALLCONDITION		8
#define COLUMN_CPQFCAHOSTCNTLRTAPECNTLRLIST		9
#define COLUMN_CPQFCAHOSTCNTLRSERIALNUMBER		10
#define COLUMN_CPQFCAHOSTCNTLRHWLOCATION		11
#define COLUMN_CPQFCAHOSTCNTLRWORLDWIDEPORTNAME		12
#define COLUMN_CPQFCAHOSTCNTLRFIRMWAREVERSION		13
#define COLUMN_CPQFCAHOSTCNTLROPTIONROMVERSION		14
#define COLUMN_CPQFCAHOSTCNTLRPCILOCATION		15
#endif                          /* CPQFCAHOSTCNTLRTABLE_H */

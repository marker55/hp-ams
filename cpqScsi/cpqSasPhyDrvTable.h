/*
 * Note: this file originally auto-generated by mib2c using
 *  : mib2c.container.conf 17798 2009-10-27 06:44:54Z magfr $
 */
#ifndef CPQSASPHYDRVTABLE_H
#define CPQSASPHYDRVTABLE_H

extern int SasDiskCondition;

typedef struct cpqSasPhyDrvTable_entry_s {
    netsnmp_index   oid_index;

    /*
     * Index values 
     */
    oid            cpqSasPhyDrvHbaIndex;
    oid            cpqSasPhyDrvIndex;

    /*
     * Column values 
     */
    char            cpqSasPhyDrvLocationString[256];
    size_t          cpqSasPhyDrvLocationString_len;
    char            cpqSasPhyDrvModel[41];
    size_t          cpqSasPhyDrvModel_len;
    long            cpqSasPhyDrvStatus;
    long            cpqSasPhyDrvCondition;
    char            cpqSasPhyDrvFWRev[9];
    size_t          cpqSasPhyDrvFWRev_len;
    unsigned long   cpqSasPhyDrvSize;
    u_long          cpqSasPhyDrvUsedReallocs;
    char            cpqSasPhyDrvSerialNumber[41];
    size_t          cpqSasPhyDrvSerialNumber_len;
    long            cpqSasPhyDrvMemberLogDrv;
    long            cpqSasPhyDrvRotationalSpeed;
    char            cpqSasPhyDrvOsName[256];
    size_t          cpqSasPhyDrvOsName_len;
    long            cpqSasPhyDrvPlacement;
    long            cpqSasPhyDrvHotPlug;
    long            cpqSasPhyDrvType;
    char            cpqSasPhyDrvSasAddress[17];
    size_t          cpqSasPhyDrvSasAddress_len;
    long            cpqSasPhyDrvMediaType;

    long            OldStatus;
    cpqSasHbaTable_entry *hba;
    oid             FW_idx;
    int             valid;
} cpqSasPhyDrvTable_entry;
/*
 * function declarations 
 */
cpqSasPhyDrvTable_entry *
         cpqSasPhyDrvTable_createEntry(netsnmp_container *, oid, oid);
void            init_cpqSasPhyDrvTable(void);
void            initialize_table_cpqSasPhyDrvTable(void);
Netsnmp_Node_Handler cpqSasPhyDrvTable_handler;

/*
 * column number definitions for table cpqSasPhyDrvTable 
 */
#define COLUMN_CPQSASPHYDRVHBAINDEX		1
#define COLUMN_CPQSASPHYDRVINDEX		2
#define COLUMN_CPQSASPHYDRVLOCATIONSTRING		3
#define COLUMN_CPQSASPHYDRVMODEL		4
#define COLUMN_CPQSASPHYDRVSTATUS		5
#define COLUMN_CPQSASPHYDRVCONDITION		6
#define COLUMN_CPQSASPHYDRVFWREV		7
#define COLUMN_CPQSASPHYDRVSIZE		8
#define COLUMN_CPQSASPHYDRVUSEDREALLOCS		9
#define COLUMN_CPQSASPHYDRVSERIALNUMBER		10
#define COLUMN_CPQSASPHYDRVMEMBERLOGDRV		11
#define COLUMN_CPQSASPHYDRVROTATIONALSPEED		12
#define COLUMN_CPQSASPHYDRVOSNAME		13
#define COLUMN_CPQSASPHYDRVPLACEMENT		14
#define COLUMN_CPQSASPHYDRVHOTPLUG		15
#define COLUMN_CPQSASPHYDRVTYPE		16
#define COLUMN_CPQSASPHYDRVSASADDRESS		17
#define COLUMN_CPQSASPHYDRVMEDIATYPE		18
#endif                          /* CPQSASPHYDRVTABLE_H */

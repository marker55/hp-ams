#ifndef CPQHOFWVERTABLE_H
#define CPQHOFWVERTABLE_H

    /*
     * Typical data structure for a row entry
     */
typedef struct cpqHoFwVerTable_entry_s {
    netsnmp_index   oid_index;
    /*
     * Index values
     */
    oid            cpqHoFwVerIndex;
    /*
     * Column values
     */
    long            cpqHoFwVerCategory;
    long            cpqHoFwVerDeviceType;
    char            cpqHoFwVerDisplayName[128];
    size_t          cpqHoFwVerDisplayName_len;
    char            cpqHoFwVerVersion[32];
    size_t          cpqHoFwVerVersion_len;
    char            cpqHoFwVerLocation[256];
    size_t          cpqHoFwVerLocation_len;
    char            cpqHoFwVerXmlString[256];
    size_t          cpqHoFwVerXmlString_len;
    char            cpqHoFwVerKeyString[128];
    size_t          cpqHoFwVerKeyString_len;
    long            cpqHoFwVerUpdateMethod;

    int             valid;
} cpqHoFwVerTable_entry;

/*
 * function declarations 
 */
void            init_cpqHoFwVerTable(void);
void            initialize_table_cpqHoFwVerTable(void);
Netsnmp_Node_Handler cpqHoFwVerTable_handler;
extern cpqHoFwVerTable_entry * cpqHoFwVerTable_createEntry(netsnmp_container *, oid);


/*
 * column number definitions for table cpqHoFwVerTable 
 */
#define COLUMN_CPQHOFWVERINDEX		        1
#define COLUMN_CPQHOFWVERCATEGORY		2
#define COLUMN_CPQHOFWVERDEVICETYPE		3
#define COLUMN_CPQHOFWVERDISPLAYNAME		4
#define COLUMN_CPQHOFWVERVERSION		5
#define COLUMN_CPQHOFWVERLOCATION		6
#define COLUMN_CPQHOFWVERXMLSTRING		7
#define COLUMN_CPQHOFWVERKEYSTRING		8
#define COLUMN_CPQHOFWVERUPDATEMETHOD		9

#endif                          /* CPQHOFWVERTABLE_H */

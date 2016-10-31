#define        CPQSEMIBREVMAJOR   1
#define        CPQSEMIBREVMINOR   2
#define        CPQSEMIB           1

#define        CPQSCSIMIBREVMAJOR  1
#define        CPQSCSIMIBREVMINOR  2
#define        CPQSCSIMIB          5

#define MIB_CONDITION_OK          2
#define MIB_STATUS_AVAILABLE      1

#define CPQHO_MIBHEALTHSTATUSARRAY_LEN 256

typedef struct _MibStatusEntry {
    unsigned char stat;
    unsigned char cond;
    unsigned char major;
    unsigned char minor;
} MibStatusEntry;
extern MibStatusEntry cpqHostMibStatusArray[];
extern unsigned char cpqHoMibHealthStatusArray[];


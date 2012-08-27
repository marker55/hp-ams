#define SYS_CLASS_NET "/sys/class/net/"
#define MAX_INTERFACE_NAME  64           /* max interface name, i.e. bond0.x */
#define MAX_VLAN_ID_LEN     32

/* directory for ethtool ethX.info files */
#define ETHTOOL_INFO_DIR        "/var/spool/compaq/nic/nicinfo"

/* a big tmp buffer for local usage in functions */
#define TEMP_LEN 1024

#define NUM_CPQ_PCI_ITEMS   6  /* number of items on each line in the */
/* /proc/cpqpci file                   */

#define MAX_PHYSICAL_SLOTS  128  /* max number of physical slots, yes   */
/* this is very large number but it    */
/* is better than all the logic to     */
/* dynamically alloc it...             */

#define MAP_IF_NUMBER_LEN  4
#define UNKNOWN_SLOT -1
#define UNKNOWN_PORT -1
#define MAC_ADDRESS_BYTES      6

#define TAG_FOUND       1
#define TAG_NOT_FOUND   2

#define PROCFS_OK                 0
#define OUT_OF_MEMORY            -1
#define PROCFS_NO_DRIVER_NAME    -2
#define PROCFS_NO_VERSION_NUMBER -3
#define PROCFS_REMAP_NEEDED      -4   /* things less than this need re-load */
#define PROCFS_CANT_READ         -5
#define PROCFS_DRIVER_CHANGE     -6
#define PROCFS_VERSION_CHANGE    -7
#define PROCFS_EMPTY_FILE        -8

#define MII_STATUS_UP       1
#define MII_STATUS_DOWN     2

/* values from cpqnic.mib mib, not in header file because we only use them */
/* here                                                                    */
#define UNKNOWN_DUPLEX                1
#define HALF_DUPLEX                   2
#define FULL_DUPLEX                   3

#define STATUS_UNKNOWN                1
#define STATUS_OK                     2
#define STATUS_GENERAL_FAILURE        3
#define STATUS_LINK_FAILURE           4

#define ADAPTER_CONDITION_OTHER       1
#define ADAPTER_CONDITION_OK          2
#define ADAPTER_CONDITION_DEGRADED    3
#define ADAPTER_CONDITION_FAILED      4

#define MAP_CONDITION_OTHER           1
#define MAP_CONDITION_OK              2
#define MAP_CONDITION_DEGRADED        3
#define MAP_CONDITION_FAILED          4

#define MAP_GROUP_TYPE_UNKNOWN            1
#define MAP_GROUP_TYPE_NONE               2
#define MAP_GROUP_TYPE_REDUNDANTPAIR      3 /* no longer used per MIB */
#define MAP_GROUP_TYPE_NFT                4
#define MAP_GROUP_TYPE_ALB                5
#define MAP_GROUP_TYPE_FEC                6
#define MAP_GROUP_TYPE_GEC                7
#define MAP_GROUP_TYPE_AD                 8
#define MAP_GROUP_TYPE_SLB                9
#define MAP_GROUP_TYPE_TLB                10
#define MAP_GROUP_TYPE_NOT_IDENTIFIABLE   11

#define SWITCHOVER_MODE_UNKNOWN              1
#define SWITCHOVER_MODE_NONE                 2
#define SWITCHOVER_MODE_MANUAL               3
#define SWITCHOVER_MODE_SWITCH_ON_FAIL       4
#define SWITCHOVER_MODE_PREFERRED_PRIMARY    5

#define MAP_STATUS_UNKNOWN            1
#define MAP_STATUS_OK                 2
#define MAP_STATUS_PRIMARY_FAILED     3
#define MAP_STATUS_STANDBY_FAILED     4
#define MAP_STATUS_GROUP_FAILED       5


#define ADAPTER_ROLE_UNKNOWN          1
#define ADAPTER_ROLE_PRIMARY          2
#define ADAPTER_ROLE_SECONDARY        3
#define ADAPTER_ROLE_MEMBER           4
#define ADAPTER_ROLE_TXRX             5
#define ADAPTER_ROLE_TX               6
#define ADAPTER_ROLE_STANDBY          7
#define ADAPTER_ROLE_NONE             8
#define ADAPTER_ROLE_NOTAPPLICABLE    255

#define ADAPTER_STATE_UNKNOWN         1
#define ADAPTER_STATE_ACTIVE          2
#define ADAPTER_STATE_STANDBY         3
#define ADAPTER_STATE_FAILED          4

#define ADAPTER_STATS_VALID_TRUE      1
#define ADAPTER_STATS_VALID_FALSE     2

#define MIB_CONDITION_UNKNOWN         1
#define MIB_CONDITION_OK              2
#define MIB_CONDITION_DEGRADED        3
#define MIB_CONDITION_FAILED          4

#define MAP_OVERALL_CONDITION_OTHER      1
#define MAP_OVERALL_CONDITION_OK         2
#define MAP_OVERALL_CONDITION_DEGRADED   3
#define MAP_OVERALL_CONDITION_FAILED     4

#define UNKNOWN_SPEED                    0

#define CONF_SPEED_DUPLEX_OTHER            1
#define CONF_SPEED_DUPLEX_AUTO             2
#define CONF_SPEED_DUPLEX_ENET_HALF        3
#define CONF_SPEED_DUPLEX_ENET_FULL        4
#define CONF_SPEED_DUPLEX_FAST_ENET_HALF   5
#define CONF_SPEED_DUPLEX_FAST_ENET_FULL   6
#define CONF_SPEED_DUPLEX_GIG_ENET_FULL    8
#define CONF_SPEED_DUPLEX_10GIG_ENET_FULL  9
#define MAX_BOND_IFACE_LEN  32  /* max interface name length, i.e. eth0 */
#define MAX_BOND_DIR_LEN   128  /* max directory name len to bond dir   */

#ifndef NIC_DEBUG
#ifdef UNIT_TEST_GET_STATS
#define NIC_DEBUG 1
#else /* UNIT_TEST_GET_STATS */
#define NIC_DEBUG 0
#endif /* UNIT_TEST_GET_STATS */
#endif

typedef struct tBondDevice {
    char achInterfaceName[MAX_BOND_IFACE_LEN];
    int  iMIIStatus;
    int  iLinkFailureCount;
    int  iSNMPIndex; /* snmp index for this bond member interface */
    struct tBondDevice* pstrNext;
} BondDevice;

/* struct below holds contents of /proc/net/bondX/info files */
typedef struct tBondInterface {
    char achBondName[MAX_BOND_IFACE_LEN];        /* name of bond, i.e. bond0 */
    char achBondDir[MAX_BOND_DIR_LEN];           /* i.e "/proc/net/bond0"    */
    int  iBondingMode;
    int  iMIIStatus;
    char achActiveInterface[MAX_BOND_IFACE_LEN]; /* only used in NFT mode */
    int  iSNMPIndex;  /* snmp index for this bond interface */
    int  iLinkFailureCount;  /* sum of link failures for all members */
    int  iNumInterfaces;     /* number of ethX interfaces associated with */
    /* this bond                                 */
    int  iLinkOKCount;       /* number of adapters in this bond that have */
    /* a good link                               */
    uint uiMapSpeedMbps;     /* in NFT mode this is the speed of the active   */
    /* active interface, in SLB mode it is the speed */
    /* of the fastest active interface.              */
    BondDevice* pstrBondDevice; /* bonded devices */
    struct tBondInterface* pstrNext;
} BondInterface;

typedef struct tVlanRecord {
    char achDeviceName[MAX_INTERFACE_NAME];    /* i.e. eth0.4092  */
    char achId[MAX_VLAN_ID_LEN];               /* i.e. 4092       */
    char achInterfaceName[MAX_INTERFACE_NAME]; /* i.e. eth0       */
    int  iSNMPIndex; /* snmp index for interface this vlan is on */
    int  iMapping; /* assigned when VlanMapTable is filled in */
    struct tVlanRecord* pstrNext;
} VlanRecord;

typedef struct tNicTag {
    char* pchTagName;      /* the tag name this location maps to          */

    int   iTagStatus;      /* status of tag, TAG_NOT_FOUND, TAG_FOUND     */

    int   iLineNumber;     /* line number of tag in ethX.info procfs file */
    /* if tag status is TAG_FOUND.                 */

    int   iMemberOffset;   /* offset of member in struct that tag value   */
    /* shold be copied to...                       */

    void (*pCopyValue)(void* pvDest, char* pchSource);
    /* function to copy value to struct, could be  */
    /* int, string, octect, any special string     */
    /* parsing needed, etc.                        */

} NicTag;

/* one of these structs exist for each driver we detect in the procfs */
typedef struct tNicPort {
    int   iStructIndex;         /* the index of this struct, used to fill in */
    /* mib values... range from 0 to N           */
    int   iInterfaceNumber;     /* the interface number ethX.info, X==iface# */
    char* pchNicProcFile;       /* the procfs file, including path           */

    int   iNumProcfsLines;      /* number of lines in above procfs file for */
    /* sanity checks... if value is zero then   */
    /* there was a problem mapping the procfs   */
    /* file                                     */

    char* pchDriverName;    /*use link @pchNicProcFile/device/driver/module*/
    char* pchDriverVersion; /*use pchNicProcFile/device/driver/module/version */

    int   iProcfsStatus;        /* status of procfs file...                 */
    char* pciVendorName;        /* Name from pci.ids */
    char* pciDeviceName;        /* Name from pci.ids */
    char* pciSubVenName;        /* Name from pci.ids */
    char* pciSubDevName;        /* Name from pci.ids */
    ushort pciVendor;   /* use pchNicProcFile/device/vendor */
    ushort pciDevice;   /* use pchNicProcFile/device/device */
    ushort pciSubVen;   /* use pchNicProcFile/device/subsystem_vendor */
    ushort pciSubDev;   /* use pchNicProcFile/device/subsystem_device */
    struct tNicPort* next;
} NicPort;

#define PROCFS_LINE_LEN   256  /* max length of procfs line */
typedef char ProcfsLine[PROCFS_LINE_LEN];


/*
 * NIC hardware database

 * This is the hardware structure built from the Hardware Teams PDF file
 * located under \\txnfile01\Projects\ADG\PRJINFO\DesignDocs.
 * 
 * note anytime a member is added to this function, the get_db_struct_info()
 * and free_db_struct_info() functions must be updated too.
 */
typedef struct _nic_hw_db{
   int32_t pci_ven;
   int32_t pci_dev;
   int32_t pci_sub_ven;
   int32_t pci_sub_dev;
   char  *poem;
   char *ppca_part_number;
   char *pspares_part_number;
   char *pboard_rev;
   char *pport_type;
   char *pbus_type;
   int32_t inf_name_len;
   char *pname;
} nic_hw_db;

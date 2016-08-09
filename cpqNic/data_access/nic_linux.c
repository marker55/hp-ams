/******************************************************************************
 *         Copyright (c) 2011  Hewlett Packard Corporation                
 ******************************************************************************
 *                                                                        
 *   Description:                                                         
 *     This file contains the routines to gather the information the NIC  
 *     drivers place in the ethN.info files in the /proc/net/ directory;  
 *     Please see the design document for the structure to proc variable  
 *     mapping algorithm;                                                 
 *                                                                        
 ******************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <linux/pci.h>
#include <sys/time.h>
#include <dlfcn.h>
#include <dirent.h>
#include <linux/types.h>
typedef __u64 u64;
typedef __u32 u32;
typedef __u16 u16;
typedef __u8 u8;
#include <linux/ethtool.h>
#include <fcntl.h>

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <net-snmp/agent/agent_index.h>
#include <net-snmp/library/container.h>
#include <net-snmp/library/snmp_debug.h>

#include "cpqNicIfLogMapTable.h"
#include "cpqNicIfPhysAdapterTable.h"

#include "hpHelper.h"
#include "cpqNic.h"
#include "cpqnic.h"
#include "nic_linux.h"
#include "pci_linux.h"
//#include "nic_db.h"
#include "utils.h"
#include "smbios.h"

#define SYSTEM_ID_LEN     9
#define BOARD_NAME_LEN    256
#define PART_NUMBER_LEN   256
#define IPADDR_LEN        41    /* length derived from ifconfig utility */
#define TMP_OBJ_ID_LEN    128

#define MAKE_CONDITION(total, new)  (new > total ? new : total)

#define LINK_UP_TRAP_OID                         18011
#define LINK_DOWN_TRAP_OID                       18012
#define REDUNDANCY_INCREASED                     18013
#define REDUNDANCY_REDUCED                       18014

#define MAX_UINT_32 0xffffffff

extern unsigned char cpqHoMibHealthStatusArray[];
extern int trap_fire;
extern pci_node *pci_root;

static struct ifreq  ifr;
char    ifLogMapOverallStatus = 2;

#define SYSBUSPCISLOTS "/sys/bus/pci/slots/"
#define PROCFILE "/proc/net/if_inet6"

/* below array corresponds to above _BOND defines*/
char *(MapGroupType[]) = 
{
    "unknown team type",
    "Switch-assisted Load Balancing (round-robin)",
    "Network Fault Tolerance (active backup)",
    "Switch-assisted Load Balancing (xor)",
    "Switch-assisted Load Balancing (Broadcast)",
    "IEEE 802.3ad Link Aggregation",
    "Transmit Load Balancing",
    "Adaptive Load Balancing"
};

char *(MapNicType[]) = 
{
    "Unknown interface",
    "Bridge - Physical",
    "Bridge - Logical",
    "Team ",
    "Loopback",
    "IPV6inV4",
    "Virtual NIC",
    "NIC",
    "VLAN"
};
#define BRIDGE 1
#define VBRIDGE 2
#define BOND 3
#define LOOP 4
#define SIT  5
#define VNIC 6
#define NIC  7
#define VLAN 8

#define UNKNOWN_BOND        -1
#define ROUND_ROBIN_BOND    0
#define ACTIVE_BACKUP_BOND  1
#define XOR_BOND            2
#define BROADCAST_BOND      3
#define IEEE_8023AD_BOND    4
#define TLB_BOND            5
#define ALB_BOND            6

/* for reading procfs...              */
extern int get_nic_port(char * name);
extern int name2pindx(char *name);
int32_t get_pcislot(char *);
static void GetConfSpeedDuplex(cpqNicIfPhysAdapterTable_entry *);

oid sysUpTime[] = {1, 3, 6, 1, 2, 1, 1, 3, 0};

void free_ethtool_info_members(ethtool_info *); 
int32_t get_ethtool_info(char *, ethtool_info *);
static int SendNicTrap(int, cpqNicIfPhysAdapterTable_entry*);

void  get_bondtype(char *netdev, cpqNicIfLogMapTable_entry *row)
{
    char buffer[256];
    char *mode;
    int bt = -1;

    sprintf(buffer, "/sys/class/net/%s/bonding/mode", netdev);

    if ((mode = get_sysfs_str(buffer)) == NULL)
        return;
    sscanf(mode, "%s %d", buffer, &bt);
    free(mode);

    if (bt < 7){
        strcpy(row->cpqNicIfLogMapDescription, MapGroupType[bt + 1]);
        row->cpqNicIfLogMapDescription_len = strlen(MapGroupType[bt + 1]);
    }

    switch (bt) {
        case ROUND_ROBIN_BOND:
        case XOR_BOND:
        case BROADCAST_BOND:
            row->cpqNicIfLogMapGroupType = MAP_GROUP_TYPE_SLB;
            row->cpqNicIfLogMapSwitchoverMode = SWITCHOVER_MODE_SWITCH_ON_FAIL;
            break;

        case ACTIVE_BACKUP_BOND:
            row->cpqNicIfLogMapGroupType = MAP_GROUP_TYPE_NFT;
            row->cpqNicIfLogMapSwitchoverMode = SWITCHOVER_MODE_PREFERRED_PRIMARY;
            break;

        case IEEE_8023AD_BOND:
            row->cpqNicIfLogMapGroupType = MAP_GROUP_TYPE_AD;
            row->cpqNicIfLogMapSwitchoverMode = SWITCHOVER_MODE_SWITCH_ON_FAIL;
            break;

        case TLB_BOND:
            row->cpqNicIfLogMapGroupType = MAP_GROUP_TYPE_TLB;
            row->cpqNicIfLogMapSwitchoverMode = SWITCHOVER_MODE_PREFERRED_PRIMARY;
            break;

        case ALB_BOND:
            row->cpqNicIfLogMapGroupType = MAP_GROUP_TYPE_ALB;
            row->cpqNicIfLogMapSwitchoverMode = SWITCHOVER_MODE_PREFERRED_PRIMARY;
            break;

        case UNKNOWN_BOND:
            row->cpqNicIfLogMapGroupType = MAP_GROUP_TYPE_NOT_IDENTIFIABLE;
            row->cpqNicIfLogMapSwitchoverMode = SWITCHOVER_MODE_UNKNOWN;
            break;
        default:
            DEBUGMSGTL(("cpqnic:arch",
                        "ERROR: cmanic, unknown bonding mode "
                        "for %s\n",
                        netdev));
            break;
    }
}

int get_ifrole(cpqNicIfPhysAdapterTable_entry* nic)
{
    struct stat sb;
    char buffer[256];
    char *value;

    if (nic->iflogmapp_entry == NULL) 
        return 255;
    if (nic->iflogmapp_entry->cpqNicIfLogMapGroupType == 2)
        return 4;
    if ((nic->iflogmapp_entry->cpqNicIfLogMapGroupType >= 5) &&
        (nic->iflogmapp_entry->cpqNicIfLogMapGroupType <= 9))
        return 4;
    sprintf(buffer, "/sys/class/net/%s/master", nic->devname);

    if (stat(buffer, &sb) == 0) {
        strcat(buffer,"/bonding/primary");
        value = get_sysfs_str(buffer);
        if (value != NULL) {
            if (strcmp(value, nic->devname) == 0) {
                free(value);
                return 2;
            } else
                return 3;
        } else 
            return 4;
    }
    return 255;
}

int get_parent_bridge(char *netdev)
{
    char buffer[256];
    struct stat sb;

    sprintf(buffer, "/sys/class/net/%s/brport/bridge/ifindex", netdev);
    if (stat(buffer, &sb) == 0) 
        return (get_sysfs_int(buffer));
    return -1;
}

int get_parent_bond(char *netdev)
{
    char buffer[256];
    struct stat sb;

    sprintf(buffer, "/sys/class/net/%s/master/ifindex", netdev);
    if (stat(buffer, &sb) == 0)
        return (get_sysfs_int(buffer));
    return -1;
}

int get_parent(char *netdev)
{
    int ifindex;
    if ((ifindex = get_parent_bridge(netdev)) == -1)
        return (get_parent_bond(netdev));
    return ifindex;
}


int get_iftype(char *netdev) 
{
    struct dirent **filelist;
    struct stat sb;
    char buffer[256];
    int ret;

    int count;
    int i = 0;

    sprintf(buffer, "/sys/class/net/%s/device", netdev);
    if (stat(buffer, &sb) == 0)
        return NIC;

    if (!strcmp(netdev, "lo"))
        return LOOP;

    if (!strncmp(netdev, "sit", 3))
        return SIT;

    if (strchr(netdev, '.') !=  (char *) NULL)
                return VLAN;

    sprintf(buffer, "/sys/class/net/%s/bonding", netdev);
    if (stat(buffer, &sb) == 0)
        return BOND;

    sprintf(buffer, "/sys/class/net/%s/brport", netdev);
    if (stat(buffer, &sb) == 0) 
            return VNIC;

    sprintf(buffer, "/sys/class/net/%s/brif/", netdev);
    if (stat(buffer, &sb) == 0) {
        if ((count = scandir(buffer, &filelist, file_select, alphasort)) > 0 ) {
            ret = BRIDGE;
            for (i = 0; i < count; i++) {
                if (get_iftype(filelist[i]->d_name) == VNIC)
                    ret = VBRIDGE;
                free(filelist[i]);
            }
            free(filelist);
            return ret;
        }
    }
    return 0;
}

int get_iftype_idx(int index)
{
    char  ifname[IF_NAMESIZE];
    char *devname;

    if (index < 0 )
        return NIC;

    devname = if_indextoname((unsigned int) index, ifname);

    if (devname == NULL)
        return NIC;

    return (get_iftype(devname));
}


int setMACaddr(char * name, unsigned char * MacAddr) {

    unsigned int MAC[6];
    char  *buffer;
    char  address[256];
    char  address_len[256];
    int len = 0;
    int j;

    if (!strncmp(name, "lo", 2)) {
        MacAddr[0] = '\0';
        return 0;
    }
        
    sprintf(address, "/sys/class/net/%s/address", name);
    sprintf(address_len, "/sys/class/net/%s/addr_len", name);

    if ((buffer = get_sysfs_str(address)) != NULL )  {
        DEBUGMSGTL(("cpqnic:arch","MAC length file = %s\n", address_len));

        if ((len = get_sysfs_int(address_len)) == 4) 
            sscanf(buffer, "%x:%x:%x:%x", &MAC[0], &MAC[1], &MAC[2], &MAC[3]);
        else if (len == 6) 
            sscanf(buffer, "%x:%x:%x:%x:%x:%x", &MAC[0], &MAC[1], &MAC[2], 
                    &MAC[3], &MAC[4], &MAC[5]);
        free(buffer);
        for (j = 0; j < len; j++) 
            MacAddr[j] = (unsigned char) MAC[j];
    }
    DEBUGMSGTL(("cpqnic:arch","MAC length = %d\n",len));
    return (len);
}

unsigned int get_if_speed(char *interface) {

    char attribute[256];
    unsigned int speed;

    snprintf(attribute, 255, "/sys/class/net/%s/speed", interface);

    speed = get_sysfs_uint(attribute);
    if (((int)speed == -1) ||((short)speed == -1)) speed = 0;
    DEBUGMSGTL(("cpqnic:arch", "Speed (%s) = %d\n", attribute, speed));

    return speed * 1000000;
}

unsigned int get_if_speedMbps(char *interface) {

    char attribute[256];
    unsigned int speed;
    
    snprintf(attribute, 255, "/sys/class/net/%s/speed", interface);

    speed = get_sysfs_uint(attribute);
    if (((int)speed == -1) ||((short)speed == -1)) speed = 0;
    DEBUGMSGTL(("cpqnic:arch", "Speed Mbps (%s) = %d\n", attribute, speed));

    return speed;
}

int get_if_status(char *interface) {

    char attribute[256];
    char *value;
    int link;
    unsigned short iff;

    snprintf(attribute, 255, "/sys/class/net/%s/flags", interface);

    iff = get_sysfs_shex(attribute);
    DEBUGMSGTL(("cpqnic:arch", "Flags (%s) = %x\n", attribute, iff));

    if (iff & IFF_UP) {
        snprintf(attribute, 255, "/sys/class/net/%s/carrier", interface);
        link = get_sysfs_int(attribute);
        DEBUGMSGTL(("cpqnic:arch", "Links (%s) = %x\n", attribute, link));
        if (link) 
            return ADAPTER_CONDITION_OK;

        snprintf(attribute, 255, "/sys/class/net/%s/operstate", interface);
        if ((value = get_sysfs_str(attribute)) != NULL) {
            DEBUGMSGTL(("cpqnic:arch", "Operstate (%s) = %s\n", attribute, value));
            if (strcmp(value, "up") == 0) 
                return ADAPTER_CONDITION_OK;
            else if ((strcmp(value, "unknown") == 0) && (iff & IFF_LOOPBACK))
                return ADAPTER_CONDITION_OK;
            else if (strcmp(value, "down") == 0) 
                return ADAPTER_CONDITION_FAILED;
            else 
                return ADAPTER_CONDITION_OTHER;

            free(value);
        }
    }
    return ADAPTER_CONDITION_OTHER;
}

unsigned long long get_nic_memaddr(char * name)
{
    char buffer[256];

    memset(&buffer[0], 0, 256);
    snprintf(&buffer[0], 255, "/sys/class/net/%s/device/resource", name);
    return get_sysfs_llhex(&buffer[0]);
}


/******************************************************************************
 *   Description: Determine advertised speed/duplex, for                  
 *                cpqNicIfPhysAdapterConfSpeedDuplex MIB object           
 *   Parameters : ptr to physical NIC struct                           
 *   Returns    : void                                                    
 ******************************************************************************/
static void GetConfSpeedDuplex(cpqNicIfPhysAdapterTable_entry *entry)
{
    /* TODO: if the link is not up we can't determine the configured  */
    /*       speed/duplex. there is no clean way to get it today if   */
    /*       the link is not up.                                      */
    if (entry->AdapterAutoNegotiate) {
        entry->cpqNicIfPhysAdapterConfSpeedDuplex = CONF_SPEED_DUPLEX_AUTO;
        return;
    }

    if ((UNKNOWN_SPEED == entry->cpqNicIfPhysAdapterSpeedMbps) ||
            (UNKNOWN_DUPLEX == entry->cpqNicIfPhysAdapterDuplexState)) {
        entry->cpqNicIfPhysAdapterConfSpeedDuplex = CONF_SPEED_DUPLEX_OTHER;
        return;
    }

    if (FULL_DUPLEX == entry->cpqNicIfPhysAdapterDuplexState) {
        switch (entry->cpqNicIfPhysAdapterSpeedMbps) {

            case 10000:  /* 10Gb   */
                entry->cpqNicIfPhysAdapterConfSpeedDuplex = 
                    CONF_SPEED_DUPLEX_10GIG_ENET_FULL;
                break;

            case 1000:   /* 1Gb   */
                entry->cpqNicIfPhysAdapterConfSpeedDuplex = 
                    CONF_SPEED_DUPLEX_GIG_ENET_FULL;
                break;

            case 100:    /* 100 Mb */
                entry->cpqNicIfPhysAdapterConfSpeedDuplex = 
                    CONF_SPEED_DUPLEX_FAST_ENET_FULL;
                break;

            case 10:     /* 10 Mb   */
                entry->cpqNicIfPhysAdapterConfSpeedDuplex = 
                    CONF_SPEED_DUPLEX_ENET_FULL;
                break;

            default:
                entry->cpqNicIfPhysAdapterConfSpeedDuplex = 
                    CONF_SPEED_DUPLEX_OTHER;
                break;
        }
    } else if (HALF_DUPLEX == entry->cpqNicIfPhysAdapterDuplexState) {
        switch (entry->cpqNicIfPhysAdapterSpeedMbps) {

            case 100: /* 100 Mb */
                entry->cpqNicIfPhysAdapterConfSpeedDuplex = 
                    CONF_SPEED_DUPLEX_FAST_ENET_HALF;
                break;

            case 10: /* 10Mb   */
                entry->cpqNicIfPhysAdapterConfSpeedDuplex = 
                    CONF_SPEED_DUPLEX_ENET_HALF;
                break;

            default:
                entry->cpqNicIfPhysAdapterConfSpeedDuplex = 
                    CONF_SPEED_DUPLEX_OTHER;
                break;
        }
    } else {
        entry->cpqNicIfPhysAdapterConfSpeedDuplex = CONF_SPEED_DUPLEX_OTHER;
    }
    return;
}

/*----------------------------------------------------------------------*\
 * |                                                                        |
 * |  Description:                                                          |
 * |               Send the cpqnic linkup (18011) or
 * |               linkdown (18012) trap to iLO                             |
 * |  Returns    :                                                          |
 * |               zero on success; non-zero on failure                     |
 * |                                                                        |
 * \*----------------------------------------------------------------------*/
static int SendNicTrap(int specific_type,     
        cpqNicIfPhysAdapterTable_entry* nic)
{

    int fd;
    register struct sockaddr_in *sin;
    char *ipDigit;
    struct utsname sys_name;
    char ServerSystemId[SYSTEM_ID_LEN + 14]; /* +14 for "Not Available" */
    char PciSlotBoardName[BOARD_NAME_LEN];
    char IfPhysAdapterPartNumber[PART_NUMBER_LEN];
    char ipLinkAddr[IPADDR_LEN]; /* IPv4 address of failed/restored link */
    unsigned char ipAddr[4];
    char IfLogMapIPV6Address[IPADDR_LEN]; /* IPv6 address of link */

    unsigned char *pRecord = NULL;
    netsnmp_variable_list *var_list = NULL;

    static oid compaq[] = { 1, 3, 6, 1, 4, 1, 232 };
    static oid compaq_len = 7;

    /* add trap indexes to each object if possible */
    static oid sysName[] = {1, 3, 6, 1, 2, 1, 1, 5, 0}; /* true index */
    static oid cpqHoTrapFlags[]
        = {1, 3, 6, 1, 4, 1, 232, 11, 2, 11, 1, 0}; /* true index */
    static oid cpqNicIfPhysAdapterSlot[]
        = {1, 3, 6, 1, 4, 1, 232, 18, 2, 3, 1, 1, 5, 255};  /*index holder*/
    static oid cpqNicIfPhysAdapterPort[]
        = {1, 3, 6, 1, 4, 1, 232, 18, 2, 3, 1, 1, 10, 255}; /*index holder*/
    static oid cpqSiServerSystemId[]
        = {1, 3, 6, 1, 4, 1, 232, 2, 2, 4, 17, 0}; /* true index */
    static oid cpqNicIfPhysAdapterStatus[]
        = {1, 3, 6, 1, 4, 1, 232, 18, 2, 3, 1, 1, 14, 255}; /*index holder*/
    static oid cpqSePciSlotBoardName[]
        = {1, 3, 6, 1, 4, 1, 232, 1, 2, 13, 1, 1, 5, 255,255}; /*idxholder*/
    static oid cpqNicIfPhysAdapterPartNumber[]
        = {1, 3, 6, 1, 4, 1, 232, 18, 2, 3, 1, 1, 32, 255}; /*index holder*/
    static oid ipAdEntAddr[]
        = {1, 3, 6, 1, 2, 1, 4, 20, 1, 1, 0, 0, 0, 0}; /*index holder*/
    static oid cpqNicIfLogMapIPV6Address[]
        = {1, 3, 6, 1, 4, 1, 232, 18, 2, 2, 1, 1, 20, 255}; /*index holder*/
    static oid cpqNicIfLogMapAdapterOKCount[]
        = {1, 3, 6, 1, 4, 1, 232, 18, 2, 2, 1, 1, 6, 255}; /*index holder*/

    unsigned int cpqHoTrapFlag;

    netsnmp_cache  *cpqNicIfLogMapTable_cache = NULL;
    netsnmp_container *iflogmap_container;
    netsnmp_iterator  *it;
    cpqNicIfLogMapTable_entry* iflogmap_entry = NULL;

    cpqNicIfLogMapTable_cache = netsnmp_cache_find_by_oid(cpqNicIfLogMapTable_oid,
                                            cpqNicIfLogMapTable_oid_len);

    DEBUGMSGTL(("cpqnic:trap", "trap = %d\n", specific_type));
    switch (specific_type) {
        case (LINK_DOWN_TRAP_OID):
            cpqHoTrapFlag = 4 << 2;
            break;

        case (LINK_UP_TRAP_OID):
            cpqHoTrapFlag = 2 << 2;
            break;

        case REDUNDANCY_REDUCED:
            cpqHoTrapFlag = 3 << 2;
            break;

        case REDUNDANCY_INCREASED:
            cpqHoTrapFlag = 2 << 2;
            break;

        default:
            cpqHoTrapFlag = 0;
            break;
    }
    if (trap_fire)
        cpqHoTrapFlag = trap_fire << 2;

    strcpy(ServerSystemId, "Not Available");
    strcpy(PciSlotBoardName, "EMPTY"); /* default "empty" per mib */
    strcpy(IfPhysAdapterPartNumber, "Not Available");

    if (cpqNicIfLogMapTable_cache != NULL) {
        iflogmap_container = cpqNicIfLogMapTable_cache->magic;
        it = CONTAINER_ITERATOR(iflogmap_container);
        if ((specific_type == REDUNDANCY_REDUCED) || (specific_type == REDUNDANCY_INCREASED)) {
            iflogmap_entry = ITERATOR_FIRST( it );
            while ( iflogmap_entry != NULL ) {
                if (iflogmap_entry->cpqNicIfLogMapIndex == nic->cpqNicIfPhysAdapterAggregationGID)
                    break;
                iflogmap_entry = ITERATOR_NEXT( it );
            }
            ITERATOR_RELEASE( it );
        } else {
            iflogmap_entry = ITERATOR_FIRST( it );
            while ( iflogmap_entry != NULL ) {
                if (iflogmap_entry->cpqNicIfLogMapIndex == nic->cpqNicIfPhysAdapterIndex)
                    break;
                iflogmap_entry = ITERATOR_NEXT( it );

            }
            ITERATOR_RELEASE( it );
        }
    }

    /* find the slot and port associated with this interface */
    if ( nic->cpqNicIfPhysAdapterIndex >= 0 ) {
        /* Get SiServerSystemId from SmBIOS  */
        SmbGetRecordByType(SMBIOS_HPOEM_SYSID, 0, (void *)&pRecord);
        memset(ServerSystemId, 0, SYSTEM_ID_LEN + 14);
        strncpy(ServerSystemId, (char *)SmbGetStringByNumber(pRecord, 1), 9);

        /* get IPv4/6 address associated with this interface */
        strcpy(ipLinkAddr, "0.0.0.0");
        if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0) {
            memset(&ifr, 0, sizeof(struct ifreq));
            if ((iflogmap_entry != NULL) && 
                (if_indextoname(iflogmap_entry->cpqNicIfLogMapIndex, ifr.ifr_name) 
                    != NULL)) {
                DEBUGMSGTL(("cpqnic:trap",
                            "name = %s\n",
                            ifr.ifr_name));

                if (ioctl(fd, SIOCGIFADDR, &ifr) >= 0) {
                    sin = (struct sockaddr_in *) &ifr.ifr_addr;
                    strcpy(ipLinkAddr,
                            inet_ntoa(sin->sin_addr));
                }
            }
            close(fd);
        }

        if (iflogmap_entry != NULL) 
            strncpy(IfLogMapIPV6Address, iflogmap_entry->cpqNicIfLogMapIPV6Address, IPADDR_LEN);
        else
            strcpy(IfLogMapIPV6Address, "0.0.0.0");
    } else {
        snmp_log(LOG_ERR, "ERROR: SendNicTrap(): physNicIndex is < 0!!!\n");
        return(1);
    }

    cpqNicIfPhysAdapterSlot[
        OID_LENGTH(cpqNicIfPhysAdapterSlot)-1] = nic->cpqNicIfPhysAdapterIndex;
    cpqNicIfPhysAdapterPort[
        OID_LENGTH(cpqNicIfPhysAdapterPort)-1] = nic->cpqNicIfPhysAdapterIndex;
    cpqNicIfPhysAdapterStatus[
        OID_LENGTH(cpqNicIfPhysAdapterStatus)-1] = nic->cpqNicIfPhysAdapterIndex;
    cpqSePciSlotBoardName[OID_LENGTH(cpqSePciSlotBoardName)-2] = 
        nic->PCI_Bus;
    cpqSePciSlotBoardName[OID_LENGTH(cpqSePciSlotBoardName)-1] = 
        nic->PCI_Slot;
    cpqNicIfPhysAdapterPartNumber[
        OID_LENGTH(cpqNicIfPhysAdapterPartNumber)-1] = 
        nic->cpqNicIfPhysAdapterIndex;

    /* skip over "." (dots) in the ip address */
    /* TODO: this is IPv4 specific...         */
    DEBUGMSGTL(("cpqnic:trap", "ipLinkAddr %s\n", ipLinkAddr));
    DEBUGMSGTL(("cpqnic:trap", "IfLogMapIPV6Address %s\n",
                IfLogMapIPV6Address));

    ipDigit = ipLinkAddr;
    ipAddr[0] = ipAdEntAddr[OID_LENGTH(ipAdEntAddr)-4] = atoi(ipDigit);

    ipDigit = strchr(ipDigit, '.');
    if (ipDigit != (char*)0)
        ipDigit++;
    ipAddr[1] = ipAdEntAddr[OID_LENGTH(ipAdEntAddr)-3] = atoi(ipDigit);

    ipDigit = strchr(ipDigit, '.');
    if (ipDigit != (char*)0)
        ipDigit++;
    ipAddr[2] = ipAdEntAddr[OID_LENGTH(ipAdEntAddr)-2] = atoi(ipDigit);

    ipDigit = strchr(ipDigit, '.');
    if (ipDigit != (char*)0)
        ipDigit++;
    ipAddr[3] = ipAdEntAddr[OID_LENGTH(ipAdEntAddr)-1] = atoi(ipDigit);

    cpqNicIfLogMapIPV6Address[OID_LENGTH(cpqNicIfLogMapIPV6Address)-1] = 
        nic->cpqNicIfPhysAdapterIndex;

    uname(&sys_name);   /* get sysName */

    snmp_varlist_add_variable(&var_list, sysName,
            sizeof(sysName) / sizeof(oid),
            ASN_OCTET_STR,
            (u_char *) sys_name.nodename,
            strlen(sys_name.nodename));

    snmp_varlist_add_variable(&var_list, cpqHoTrapFlags,
            sizeof(cpqHoTrapFlags) / sizeof(oid),
            ASN_INTEGER, &cpqHoTrapFlag,
            sizeof(ASN_INTEGER));

    snmp_varlist_add_variable(&var_list, cpqNicIfPhysAdapterSlot,
            sizeof(cpqNicIfPhysAdapterSlot) / sizeof(oid),
            ASN_INTEGER, (u_char *) &nic->cpqNicIfPhysAdapterSlot,
            sizeof(ASN_INTEGER));

    snmp_varlist_add_variable(&var_list, cpqNicIfPhysAdapterPort,
            sizeof(cpqNicIfPhysAdapterPort) / sizeof(oid),
            ASN_INTEGER, (u_char *) &nic->cpqNicIfPhysAdapterPort,
            sizeof(ASN_INTEGER));

    snmp_varlist_add_variable(&var_list, cpqSiServerSystemId,
            sizeof(cpqSiServerSystemId) / sizeof(oid),
            ASN_OCTET_STR,
            (u_char *) ServerSystemId,
            strlen(ServerSystemId));

    snmp_varlist_add_variable(&var_list, cpqNicIfPhysAdapterStatus,
            sizeof(cpqNicIfPhysAdapterStatus) / sizeof(oid),
            ASN_INTEGER, (u_char *) &nic->cpqNicIfPhysAdapterStatus,
            sizeof(ASN_INTEGER));

    snmp_varlist_add_variable(&var_list, cpqSePciSlotBoardName,
            sizeof(cpqSePciSlotBoardName) / sizeof(oid),
            ASN_OCTET_STR,
            (u_char *) nic->cpqNicIfPhysAdapterName,
            strlen(nic->cpqNicIfPhysAdapterName));

    snmp_varlist_add_variable(&var_list, cpqNicIfPhysAdapterPartNumber,
            sizeof(cpqNicIfPhysAdapterPartNumber) / sizeof(oid),
            ASN_OCTET_STR,
            (u_char *) nic->cpqNicIfPhysAdapterPartNumber,
            strlen(nic->cpqNicIfPhysAdapterPartNumber));

    snmp_varlist_add_variable(&var_list, ipAdEntAddr,
            sizeof(ipAdEntAddr) / sizeof(oid),
            ASN_IPADDRESS,
            (u_char *) ipAddr,
            sizeof(ASN_IPADDRESS));

    snmp_varlist_add_variable(&var_list, cpqNicIfLogMapIPV6Address,
            sizeof(cpqNicIfLogMapIPV6Address) / sizeof(oid),
            ASN_OCTET_STR,
            (u_char *) IfLogMapIPV6Address,
            strlen(IfLogMapIPV6Address));

    if ((nic->cpqNicIfPhysAdapterAggregationGID != -1) && 
        ((specific_type == REDUNDANCY_INCREASED) || (specific_type == REDUNDANCY_REDUCED))) {
        cpqNicIfLogMapAdapterOKCount[OID_LENGTH(cpqNicIfLogMapAdapterOKCount) - 1] =
                         nic->cpqNicIfPhysAdapterAggregationGID;
        snmp_varlist_add_variable(&var_list, cpqNicIfLogMapAdapterOKCount,
                sizeof(cpqNicIfLogMapAdapterOKCount) / sizeof(oid),
                ASN_INTEGER,
                &iflogmap_entry->cpqNicIfLogMapAdapterOKCount,
                sizeof(ASN_INTEGER));
    }

    DEBUGMSGTL(("cpqnic:trap","calling netsnmp_send_trap...\n"));

    send_enterprise_trap_vars(SNMP_TRAP_ENTERPRISESPECIFIC,
            specific_type,
            compaq,
            compaq_len,
            var_list);

    DEBUGMSGTL(("cpqnic:trap","returned from netsnmp_send_trap; call snmp_free_varbind()....\n"));

    snmp_free_varbind(var_list);

    DEBUGMSGTL(("cpqnic:trap","done snmp_free_varbind().\n"));

    return (0);
}

void initcpqNicIfPhys_entry(cpqNicIfPhysAdapterTable_entry* entry)
{
    entry->cpqNicIfPhysAdapterIoBayNo = UNKNOWN_SLOT;
    entry->cpqNicIfPhysAdapterVirtualPortNumber = UNKNOWN_PORT;

    entry->cpqNicIfPhysAdapterVirtualPortNumber = -1;
    entry->cpqNicIfPhysAdapterIoBayNo = -1;
    entry->cpqNicIfPhysAdapterSpeedMbps = UNKNOWN_SPEED;
    entry->cpqNicIfPhysAdapterConfSpeedDuplex = CONF_SPEED_DUPLEX_OTHER;
    entry->AdapterAutoNegotiate = 0;
    entry->cpqNicIfPhysAdapterMemAddr = 0;
    entry->cpqNicIfPhysAdapterAggregationGID = -1;
    entry->cpqNicIfPhysAdapterFWVersion[0] = '\0';
    strcpy(entry->cpqNicIfPhysAdapterPartNumber, "UNKNOWN");
    /* default "empty" per mib */
    strcpy(entry->cpqNicIfPhysAdapterName, ""); 
    entry->cpqNicIfPhysAdapterName_len = 0; 

    entry->cpqNicIfPhysAdapterRole = ADAPTER_ROLE_NOTAPPLICABLE;

    entry->cpqNicIfPhysAdapterPrevCondition = ADAPTER_CONDITION_OTHER;
    entry->cpqNicIfPhysAdapterCondition = ADAPTER_CONDITION_OTHER;
    entry->cpqNicIfPhysAdapterPrevStatus = STATUS_UNKNOWN;
    entry->cpqNicIfPhysAdapterStatus = STATUS_UNKNOWN;

    entry->cpqNicIfPhysAdapterPrevState = ADAPTER_STATE_UNKNOWN;
    entry->cpqNicIfPhysAdapterState = ADAPTER_STATE_UNKNOWN;

    entry->cpqNicIfPhysAdapterHwLocation[0] = '\0';
    entry->cpqNicIfPhysAdapterHwLocation_len = 0;

}

void cpqNicIfPhysAdapter_reload_entry(cpqNicIfPhysAdapterTable_entry *entry)
{
    ethtool_info einfo;
    netsnmp_cache  *cpqNicIfLogMapTable_cache = NULL;
    netsnmp_container *iflogmap_container;
    netsnmp_iterator  *it;
    cpqNicIfLogMapTable_entry* iflogmap_entry = NULL;

    //netsnmp_container *iflogmap_container;
    //netsnmp_iterator  *it;

    char buffer[256];
    char attribute[256];

    DEBUGMSGTL(("cpqnic:container:reload", "\nreload cpqIfPhysTable %s\n",
                entry->devname)); 
    if (get_ethtool_info(entry->devname, &einfo) == 0) {
        DEBUGMSGTL(("cpqnic:container:load", "Got ethtool info for %s\n", 
                    entry->devname));  
        DEBUGMSGTL(("cpqnic:container:load", "ethtool Duplex info for %s = %d\n",
                    entry->devname, einfo.duplex));
        DEBUGMSGTL(("cpqnic:container:load", "ethtool AutoNegotiate info for %s = %d\n",
                    entry->devname, einfo.autoneg));
        entry->AdapterAutoNegotiate =  einfo.autoneg; 
        entry->cpqNicIfPhysAdapterDuplexState =  UNKNOWN_DUPLEX; 
        if (einfo.duplex == DUPLEX_FULL)  
            entry->cpqNicIfPhysAdapterDuplexState =  FULL_DUPLEX; 
        if (einfo.duplex == DUPLEX_HALF)  
            entry->cpqNicIfPhysAdapterDuplexState =  HALF_DUPLEX; 
        entry->cpqNicIfPhysAdapterSpeedMbps = einfo.link_speed;
        if (entry->cpqNicIfPhysAdapterSpeedMbps != UNKNOWN_SPEED) {
            if (entry->cpqNicIfPhysAdapterSpeedMbps <= 4294 )
                entry->cpqNicIfPhysAdapterSpeed = 
                    entry->cpqNicIfPhysAdapterSpeedMbps * 1000000;
        }
    }

    memset(buffer, 0, 256);
    sprintf(buffer, "/sys/class/net/%s", entry->devname);

    GetConfSpeedDuplex(entry);

    entry->cpqNicIfPhysAdapterPrevCondition = entry->cpqNicIfPhysAdapterCondition;
    entry->cpqNicIfPhysAdapterCondition = get_if_status(entry->devname);

    entry->cpqNicIfPhysAdapterPrevStatus = entry->cpqNicIfPhysAdapterStatus;
    entry->cpqNicIfPhysAdapterStatus = entry->cpqNicIfPhysAdapterCondition;

    entry->cpqNicIfPhysAdapterPrevState = entry->cpqNicIfPhysAdapterState;

    if (entry->cpqNicIfPhysAdapterStatus == STATUS_OK)
            entry->cpqNicIfPhysAdapterState = ADAPTER_STATE_ACTIVE;
    else 
        if (entry->cpqNicIfPhysAdapterStatus == STATUS_LINK_FAILURE)
            entry->cpqNicIfPhysAdapterState = ADAPTER_STATE_FAILED;
        else 
            entry->cpqNicIfPhysAdapterState = ADAPTER_STATE_UNKNOWN;

    entry->cpqNicIfPhysAdapterStatsValid = ADAPTER_STATS_VALID_TRUE;

    entry->cpqNicIfPhysAdapterBadTransmits =
        entry->cpqNicIfPhysAdapterDeferredTransmissions +
        entry->cpqNicIfPhysAdapterLateCollisions +
        entry->cpqNicIfPhysAdapterCarrierSenseErrors +
        entry->cpqNicIfPhysAdapterInternalMacTransmitErrors;

    entry->cpqNicIfPhysAdapterBadReceives  =
        entry->cpqNicIfPhysAdapterAlignmentErrors +
        entry->cpqNicIfPhysAdapterFCSErrors +
        entry->cpqNicIfPhysAdapterFrameTooLongs +
        entry->cpqNicIfPhysAdapterInternalMacReceiveErrors;

    strcpy(attribute, buffer);
    strcat(attribute, "/statistics/rx_bytes");
    entry->cpqNicIfPhysAdapterInOctets = get_sysfs_int(attribute);

    strcpy(attribute, buffer);
    strcat(attribute, "/statistics/tx_bytes");
    entry->cpqNicIfPhysAdapterOutOctets = get_sysfs_int(attribute);

    DEBUGMSGTL(("cpqnic:container:load", "rx_bytes= %ld tx_bytes = %ld\n", 
                entry->cpqNicIfPhysAdapterInOctets,
                entry->cpqNicIfPhysAdapterOutOctets));

    strcpy(attribute, buffer);
    strcat(attribute, "/statistics/rx_packets");
    entry->cpqNicIfPhysAdapterGoodReceives = get_sysfs_int(attribute);

    strcpy(attribute, buffer);
    strcat(attribute, "/statistics/tx_packets");
    entry->cpqNicIfPhysAdapterGoodTransmits = get_sysfs_int(attribute);

    DEBUGMSGTL(("cpqnic:container:load", 
                "rx_packets= %ld tx_packets = %ld\n", 
                entry->cpqNicIfPhysAdapterGoodReceives,
                entry->cpqNicIfPhysAdapterGoodTransmits));

    entry->cpqNicIfPhysAdapterAggregationGID = get_parent(entry->devname);
    

    cpqNicIfLogMapTable_cache = netsnmp_cache_find_by_oid(cpqNicIfLogMapTable_oid,
                                        cpqNicIfLogMapTable_oid_len);

    entry->iflogmapp_entry = NULL;
    entry->iflogmap_entry = NULL;
    if (cpqNicIfLogMapTable_cache != NULL) {
        iflogmap_container = cpqNicIfLogMapTable_cache->magic;
        it = CONTAINER_ITERATOR(iflogmap_container);

        iflogmap_entry = ITERATOR_FIRST( it );
        while ( iflogmap_entry != NULL ) {
            if (entry->cpqNicIfPhysAdapterAggregationGID != -1) {
                if (iflogmap_entry->cpqNicIfLogMapIndex == entry->cpqNicIfPhysAdapterAggregationGID) 
                    entry->iflogmapp_entry = iflogmap_entry;
            } else {
                if (iflogmap_entry->cpqNicIfLogMapIndex == entry->cpqNicIfPhysAdapterIndex) 
                    entry->iflogmap_entry = iflogmap_entry;
            }
            iflogmap_entry = ITERATOR_NEXT( it );
        }
        ITERATOR_RELEASE( it );
    }
    if (iflogmap_entry != NULL ) { 
        entry->iflogmap_entry = iflogmap_entry;
        DEBUGMSGTL(("cpqnic:iflog","logMap index = %ld\n", iflogmap_entry->cpqNicIfLogMapIndex));
    }
    
    entry->cpqNicIfPhysAdapterRole = get_ifrole(entry);

    DEBUGMSGTL(("cpqnic:trap","Check NIC trap = %s Status = %ld, Prev = %ld, GID = %ld\n",
                    entry->devname,
                    entry->cpqNicIfPhysAdapterStatus,
                    entry->cpqNicIfPhysAdapterPrevStatus,
                    entry->cpqNicIfPhysAdapterAggregationGID));
    if ((entry->cpqNicIfPhysAdapterStatus >= STATUS_GENERAL_FAILURE) &&
            (entry->cpqNicIfPhysAdapterPrevStatus == STATUS_OK))  {
        SendNicTrap(LINK_DOWN_TRAP_OID, entry);
        if (get_iftype_idx(entry->cpqNicIfPhysAdapterAggregationGID) == BOND) 
            SendNicTrap(REDUNDANCY_REDUCED, entry);
    } else if ((entry->cpqNicIfPhysAdapterStatus == STATUS_OK) &&
            (entry->cpqNicIfPhysAdapterPrevStatus >= STATUS_GENERAL_FAILURE)) {
        SendNicTrap(LINK_UP_TRAP_OID, entry);
        if (get_iftype_idx(entry->cpqNicIfPhysAdapterAggregationGID) == BOND)
            SendNicTrap(REDUNDANCY_INCREASED, entry);
    }
    if (trap_fire) {
        SendNicTrap(LINK_DOWN_TRAP_OID, entry);
        SendNicTrap(LINK_UP_TRAP_OID, entry);
        SendNicTrap(REDUNDANCY_INCREASED, entry);
        SendNicTrap(REDUNDANCY_REDUCED, entry);
    }

    free_ethtool_info_members(&einfo);
    DEBUGMSGTL(("cpqnic:container:load", 
                "Finished processing  ethtool info for %s\n", 
                entry->devname));
}

cpqNicIfPhysAdapterTable_entry* netsnmp_arch_ifphys_entry_load(netsnmp_container *container, oid index) 
{
    char  ifname[IF_NAMESIZE];
    char *devname;
    netsnmp_index tmp;

    cpqNicIfPhysAdapterTable_entry *entry = NULL;

    char buffer[256];
    char attribute[256];
    char pciattribute[256];
    int     iMacLoop;
    oid oid_index[2];

    static int sock = -1;
    struct ifreq ifr;

    unsigned short VendorID, DeviceID, SubsysVendorID, SubsysDeviceID;

    nic_hw_db *pnic_hw_db;
    ethtool_info einfo;

    int     domain, bus, device, function;
    char    linkBuf[1024];
    ssize_t link_sz;
    char    devlinkBuf[1024];
    ssize_t devlink_sz;
    char *pcidevice;

    devname = if_indextoname((unsigned int) index, ifname);
    if (devname == NULL)
        return NULL;

    memset(buffer, 0, 256);
    sprintf(buffer, "/sys/class/net/%s", devname);

    strcpy(attribute, buffer);
    strcat(attribute, "/device");
    DEBUGMSGTL(("cpqnic:container:load", "attribute = %s\n", attribute));
    if ((link_sz = readlink(attribute, linkBuf, 1024)) < 0) {
        return NULL;
    }
    linkBuf[link_sz] = 0;

    DEBUGMSGTL(("cpqnic:container:load", "Create new entry\n"));
    oid_index[0] = index;
    tmp.len = 1;
    tmp.oids = &oid_index[0];


    entry = CONTAINER_FIND(container, &tmp);
    if (entry == NULL) {
        pci_node* node = pci_root;

        entry = cpqNicIfPhysAdapterTable_createEntry(container, index);
        if (NULL == entry) {
            return NULL;
        }
        DEBUGMSGTL(("cpqnic:container:load", "Created new entry\n"));

        initcpqNicIfPhys_entry(entry);
        strncpy(entry->devname, devname, 255);

        pcidevice = strrchr(&linkBuf[0],'/') + 1;
        while (node != NULL) {
            if (!strncmp((char *)node->sysdev, pcidevice, strlen(pcidevice)))
                break;
            else
                node = node->pci_next;
        }
     
        entry->cpqNicIfPhysAdapterSlot = -1;
    
        entry->cpqNicIfPhysAdapterIfNumber[0] = (char)(index & 0xff);
        entry->cpqNicIfPhysAdapterIfNumber[1] = (char)((index>>8)  & 0xff);
        entry->cpqNicIfPhysAdapterIfNumber[2] = (char)((index>>16) & 0xff);
        entry->cpqNicIfPhysAdapterIfNumber[3] = (char)((index>>24) & 0xff);
    
        entry->cpqNicIfPhysAdapterIfNumber[4] = 0;
        entry->cpqNicIfPhysAdapterIfNumber[5] = 0;
        entry->cpqNicIfPhysAdapterIfNumber[6] = 0;
        entry->cpqNicIfPhysAdapterIfNumber[7] = 0;
        entry->cpqNicIfPhysAdapterIfNumber_len = 8;
    
        entry->cpqNicIfPhysAdapterPort = -1;
   
        entry->cpqNicIfPhysAdapterMemAddr = (int) get_nic_memaddr(devname);
    
        if (sock < 0) 
            sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
        if (sock >= 0) {
            memset(&ifr,0,sizeof(ifr));
            ifr.ifr_data = (char*)0;
            strncpy(ifr.ifr_name, devname, IFNAMSIZ - 1);
            if (ioctl(sock, SIOCGIFMAP, &ifr) >= 0) {
                entry->cpqNicIfPhysAdapterIoAddr = ifr.ifr_map.base_addr;
                entry->cpqNicIfPhysAdapterDma = ((int)ifr.ifr_map.dma == 0)?
                    -1:(int)ifr.ifr_map.dma;
            }
        }
    
        if (get_ethtool_info(devname, &einfo) == 0) {
            int nic_port;	
            DEBUGMSGTL(("cpqnic:container:load", "Got ethtool info for %s\n", 
                        devname));  
 
            if ((nic_port = get_nic_port(devname)) > 0)
                entry->cpqNicIfPhysAdapterPort = nic_port;
            DEBUGMSGTL(("cpqnic:data", "NIC port = %d\n", entry->cpqNicIfPhysAdapterPort));

            entry->cpqNicIfPhysAdapterSlot = getPCIslot_str(einfo.bus_info);
 
            if (entry->cpqNicIfPhysAdapterSlot > 0 ) {
                DEBUGMSGTL(("cpqnic:container:load", "Got pcislot info for %s = %u\n",
                            devname,
                            entry->cpqNicIfPhysAdapterSlot));
            } 
 
            if ( einfo.firmware_version ) {
                strcpy(entry->cpqNicIfPhysAdapterFWVersion, 
                        einfo.firmware_version);
                entry->cpqNicIfPhysAdapterFWVersion_len = 
                    strlen(entry->cpqNicIfPhysAdapterFWVersion);
            }
            for (iMacLoop = 0; iMacLoop<MAC_ADDRESS_BYTES; iMacLoop++) {
            /* If einfo.perm_addr is all 0's, the ioctl must've failed*/
                if (einfo.perm_addr[iMacLoop]) {
                    memcpy(entry->cpqNicIfPhysAdapterMACAddress, 
                            einfo.perm_addr, 
                            sizeof(entry->cpqNicIfPhysAdapterMACAddress));
                    entry->cpqNicIfPhysAdapterMACAddress_len = 
                        MAC_ADDRESS_BYTES;
                    break;
                }
            }
            free_ethtool_info_members(&einfo);
        } else 
            DEBUGMSGTL(("cpqnic:container:load", "ethtool FAILED for %s\n",
                        devname));

        if (node == NULL) { 
            sscanf(pcidevice, "%x:%x:%x.%x",
                    &domain, &bus, &device, &function);

            entry->PCI_Bus = bus;
            entry->PCI_Slot = ((device & 0xFF) << 3) | (function & 0xFF);
            /* init values to defaults in case they are not in procfs */
            DEBUGMSGTL(("cpqnic:data", "Before NIC port = %d\n", entry->cpqNicIfPhysAdapterPort));
            if (!entry->cpqNicIfPhysAdapterPort)
                entry->cpqNicIfPhysAdapterPort = function + 1;
            DEBUGMSGTL(("cpqnic:data", "PCI Function = %d\n", function));
            DEBUGMSGTL(("cpqnic:data", "After NIC port = %d\n", entry->cpqNicIfPhysAdapterPort));

        strcpy(attribute, buffer);
        strcat(attribute, "/device");
        if ((devlink_sz = readlink(attribute, devlinkBuf, 1024)) < 0) {
            return NULL;
        }
        devlinkBuf[link_sz] = 0;
        /* get PCI device and subdevice from Function 0.
         * some Devices have corrupted subsystem ID for
         * functions other than 0
         */
        devlinkBuf[link_sz - 1] = '0';

        strcpy(pciattribute, buffer);
        strcat(pciattribute, "/");
        strcat(pciattribute, devlinkBuf);

        strcpy(attribute, pciattribute);
        strcat(attribute, "/vendor");
        VendorID = get_sysfs_shex(attribute); 
    
        strcpy(attribute, pciattribute);
        strcat(attribute, "/device");
        DeviceID = get_sysfs_shex(attribute); 
    
        strcpy(attribute, pciattribute);
        strcat(attribute, "/subsystem_vendor");
        SubsysVendorID = get_sysfs_shex(attribute); 
    
        strcpy(attribute, pciattribute);
        strcat(attribute, "/subsystem_device");
        SubsysDeviceID = get_sysfs_shex(attribute);
    
        DEBUGMSGTL(("cpqnic:container:load","Vendor = %x, Device = %x, " 
                    "Subsys Vendor = %x, Subsys Device = %x\n",
                    VendorID, DeviceID,
                    SubsysVendorID, SubsysDeviceID));
        /* get model name and part number from database created struct */
        pnic_hw_db = get_nic_hw_info(VendorID, DeviceID,
                SubsysVendorID, SubsysDeviceID);
    
        strcpy(attribute, buffer);
        strcat(attribute, "/device/irq");
        entry->cpqNicIfPhysAdapterIrq = get_sysfs_int(attribute);
    
        } else {    
            pci_node* node0 = pci_root;
            char sysdev0[16];
    
            entry->PCI_Bus = node->bus;
            entry->PCI_Slot = ((node->dev & 0xFF) << 3) | (node->func & 0xFF);
            /* init values to defaults in case they are not in procfs */
            DEBUGMSGTL(("cpqnic:data", "Before2 NIC port = %d\n", entry->cpqNicIfPhysAdapterPort));
            if (!entry->cpqNicIfPhysAdapterPort)
                entry->cpqNicIfPhysAdapterPort = node->func + 1;
            DEBUGMSGTL(("cpqnic:data", "PCI Function = %d\n", node->func));
            DEBUGMSGTL(("cpqnic:data", "After2 NIC port = %d\n", entry->cpqNicIfPhysAdapterPort));
            entry->cpqNicIfPhysAdapterIrq = node->irq;
    
            memset(sysdev0, 0 , 16);
            strncpy(sysdev0, (char *)node->sysdev, 16);
            /* get PCI device and subdevice from Function 0.
             * some Devices have corrupted subsystem ID for
             * functions other than 0
             */
            sysdev0[strlen(sysdev0) - 1] = '0';
    
            while (node0 != NULL) {
                if (!strncmp((char *)node0->sysdev, sysdev0, strlen(sysdev0)))
                    break;
                else
                    node0 = node0->pci_next;
            } 
    
            if ((node0->vpd_productname) && !strncmp((char *)node0->vpd_productname,"HP ", 3)) {
                DEBUGMSGTL(("cpqnic:data", "Using VPD PN\n"));
                entry->cpqNicIfPhysAdapterName_len = snprintf(entry->cpqNicIfPhysAdapterName,
                        256, "%s", node0->vpd_productname);
            } else {
                vpd_node* vnode = node0->vpd_data;
                while (vnode != NULL){
                    if ((vnode->data != NULL) && !strncmp((char *)vnode->data, "HP ", 3)) {
                        DEBUGMSGTL(("cpqnic:data", "Using VPD other\n"));
                        entry->cpqNicIfPhysAdapterName_len = snprintf(entry->cpqNicIfPhysAdapterName,
                            256, "%s", vnode->data);
                    break;
                    } else
                        vnode = vnode->next;
                }
            }
            if (node0->vendor_id == 0x14e4) {
                vpd_node* vnode = node0->vpd_data;
                while (vnode != NULL){
                    if (!strncmp((char *)vnode->tag, "VA", 2)) {
                        entry->cpqNicIfPhysAdapterFWVersion_len = snprintf(entry->cpqNicIfPhysAdapterFWVersion,
                            256, "%s", vnode->data);
                        break;
        } else 
                        vnode = vnode->next;
                }
            }
            pnic_hw_db = get_nic_hw_info(node0->vendor_id, node0->device_id,
                    node0->subvendor_id, node0->subdevice_id);
        }
        DEBUGMSGTL(("cpqnic:data", "Using NIC DB\n"));
        if (pnic_hw_db != NULL) {
            if (strlen(pnic_hw_db->pspares_part_number))
                strcpy(entry->cpqNicIfPhysAdapterPartNumber, 
                        pnic_hw_db->pspares_part_number);
            else
                strcpy(entry->cpqNicIfPhysAdapterPartNumber, 
                        pnic_hw_db->ppca_part_number);
            entry->cpqNicIfPhysAdapterPartNumber_len = 
                strlen(entry->cpqNicIfPhysAdapterPartNumber);
            /* if product name is empty and we have an entry in the nic_db */
            if (strlen(entry->cpqNicIfPhysAdapterName) == 0) {
                strcpy(entry->cpqNicIfPhysAdapterName, pnic_hw_db->pname);
                entry->cpqNicIfPhysAdapterName_len =
                    strlen(entry->cpqNicIfPhysAdapterName);
            }
        }
    }
    return entry;
} 
/*----------------------------------------------------------------------*\
 *                                                                        
 *   Description:                                                         
 *                Load stats from procfs into provided IfPhys containert      
 *   Parameters :                                                         
 *                  container
 *   Returns    :                                                         
 *                zero on success; non-zero on failure                    
 *                                                                        
 \*----------------------------------------------------------------------*/
int netsnmp_arch_ifphys_container_load(netsnmp_container *container) 
{

    oid     Index;
    netsnmp_index tmp;
    oid oid_index[2];

    static int devcount = -1;
    struct dirent **devlist;
    int     i;
    char    mibStatus =  MIB_CONDITION_OK;

    cpqNicIfPhysAdapterTable_entry* entry;
    cpqNicIfPhysAdapterTable_entry* old;

    DEBUGMSGTL(("cpqnic:arch", "netsnmp_arch_ifphys_container_load entry\n"));

    if ((devcount = scandir("/sys/class/net/",
                    &devlist, file_select, alphasort)) <= 0) {
        cpqHoMibHealthStatusArray[CPQMIBHEALTHINDEX] = mibStatus;
        cpqHostMibStatusArray[CPQMIB].cond = mibStatus;
        return(0);
    }
    DEBUGMSGTL(("cpqnic:container:load", "devcount =%u\n",  devcount));
    for (i = 0; i < devcount; i++) {
        Index = if_nametoindex(devlist[i]->d_name);
        DEBUGMSGTL(("cpqnic:container:load", "looking for NIC =%ld\n", 
                    Index));

        oid_index[0] = Index;
        tmp.len = 1;
        tmp.oids = &oid_index[0];

        old = CONTAINER_FIND(container, &tmp);
        DEBUGMSGTL(("cpqnic:container:load", "Old nic=%p\n",old));
        if (old != NULL ) {
            DEBUGMSGTL(("cpqnic:container:load", "Re-Use old entry\n"));
            entry = old;
        } else 
            entry = netsnmp_arch_ifphys_entry_load(container, Index);

        if (entry != NULL) 
            cpqNicIfPhysAdapter_reload_entry(entry);
        if ((old == NULL) && (entry != NULL)) {
            CONTAINER_INSERT(container, entry);
            DEBUGMSGTL(("cpqnic:arch","Insert entry %s\n",
                        entry->devname));
        }
        free(devlist[i]);
    }
    numIfPhys = CONTAINER_SIZE(container);
    DEBUGMSGTL(("cpqnic:arch"," loaded %d entries\n", numIfPhys));
    free(devlist);
    return(0);
}

int get_bond_status(cpqNicIfLogMapTable_entry *row)
{
    int status;
    int i;
    int diff;
    int pstatus;

    row->cpqNicIfLogMapAdapterOKCount = 0;

    for ( i = 0; i < row->cpqNicIfLogMapAdapterCount; i++) {
        char  ifname[IF_NAMESIZE];
        char *devname;
        int ifIndex;
        ifIndex = row->cpqNicIfLogMapPhysicalAdapters[i];
        devname = if_indextoname((unsigned int) ifIndex, ifname);
        pstatus = ADAPTER_CONDITION_OTHER;
        if (devname != NULL)
            pstatus = get_if_status(devname);
        if (pstatus == ADAPTER_CONDITION_OK)
            row->cpqNicIfLogMapAdapterOKCount++;
    }
    if (!row->cpqNicIfLogMapAdapterOKCount || 
        ((status = get_if_status(row->cpqNicIfLogMapLACNumber)) != ADAPTER_CONDITION_OK)){
        row->cpqNicIfLogMapCondition = MAP_CONDITION_FAILED;
        row->cpqNicIfLogMapStatus = MAP_STATUS_GROUP_FAILED;
        return ADAPTER_CONDITION_FAILED;
    }
    diff = row->cpqNicIfLogMapAdapterCount - row->cpqNicIfLogMapAdapterOKCount;
    if (diff == 0) {
        row->cpqNicIfLogMapCondition = MAP_CONDITION_OK;
        row->cpqNicIfLogMapStatus = MAP_STATUS_OK;
    } else {
        if ((row->cpqNicIfLogMapAdapterCount - diff)  == 1) {
            row->cpqNicIfLogMapCondition = MAP_CONDITION_DEGRADED;
            row->cpqNicIfLogMapStatus = MAP_STATUS_REDUNDANCY_LOST;
        } else {
            row->cpqNicIfLogMapCondition = MAP_CONDITION_DEGRADED;
            row->cpqNicIfLogMapStatus = MAP_STATUS_REDUNDANCY_LOST;
        }
    }
    return row->cpqNicIfLogMapCondition;
}

int get_bridge_status(cpqNicIfLogMapTable_entry *row)
{
    int i;
    int diff;

    row->cpqNicIfLogMapAdapterOKCount = 0;

    for ( i = 0; i < row->cpqNicIfLogMapAdapterCount; i++) {
        char  ifname[IF_NAMESIZE];
        char *devname;
        int ifIndex;
        ifIndex = row->cpqNicIfLogMapPhysicalAdapters[i];
        devname = if_indextoname((unsigned int) ifIndex, ifname);
        if ((devname != NULL) && 
            ((get_iftype(devname) == VNIC) || 
             (get_if_status(devname) == ADAPTER_CONDITION_OK)))
            row->cpqNicIfLogMapAdapterOKCount++;
    }
/*
    if ((status = get_if_status(row->cpqNicIfLogMapLACNumber)) != ADAPTER_CONDITION_OK){
        row->cpqNicIfLogMapCondition = MAP_CONDITION_FAILED;
        row->cpqNicIfLogMapStatus = MAP_STATUS_GROUP_FAILED;
        return ADAPTER_CONDITION_FAILED;
    }
*/
    diff = row->cpqNicIfLogMapAdapterCount - row->cpqNicIfLogMapAdapterOKCount;
    if (diff == 0) {
        row->cpqNicIfLogMapCondition = MAP_CONDITION_OK;
        row->cpqNicIfLogMapStatus = MAP_STATUS_OK;
    } else {
        row->cpqNicIfLogMapCondition = MAP_CONDITION_FAILED;
        row->cpqNicIfLogMapStatus = MAP_STATUS_GROUP_FAILED;
    }
    return row->cpqNicIfLogMapCondition;
}

void cpqNicIfLogMap_reload_entry(cpqNicIfLogMapTable_entry *entry)
{
    dynamic_interface_info dyn_if_info;
    int mapStatus;
    int ifType;
    char buffer[256];
    int map = 0;
    int      j, dCount;
    struct dirent **devlist;
    int phys_index;

    netsnmp_cache  *cpqNicIfLogMapTable_cache = NULL;
    netsnmp_container *iflogmap_container;
    netsnmp_iterator  *it;
    cpqNicIfLogMapTable_entry *log_entry;
    
    cpqNicIfLogMapTable_cache = netsnmp_cache_find_by_oid(cpqNicIfLogMapTable_oid,
                                            cpqNicIfLogMapTable_oid_len);

    entry->cpqNicIfLogMapPrevCondition = entry->cpqNicIfLogMapCondition;
    entry->cpqNicIfLogMapCondition = ADAPTER_CONDITION_OTHER;
    entry->cpqNicIfLogMapPrevStatus = entry->cpqNicIfLogMapStatus;
    entry->cpqNicIfLogMapStatus = MAP_STATUS_UNKNOWN;

    DEBUGMSGTL(("cpqnic:container:reload", "\nreload cpqIfIfLogTable %s\n",
                entry->cpqNicIfLogMapLACNumber)); 
    ifType = get_iftype(entry->cpqNicIfLogMapLACNumber);
    
    /* get IPv4/6 address info */
    if (get_dynamic_interface_info(entry->cpqNicIfLogMapLACNumber, &dyn_if_info) 
        
            != -1) {
        if (dyn_if_info.ipv6_shorthand_addr != (char*)0) {
            sprintf(entry->cpqNicIfLogMapIPV6Address, "%s/%d",
                    dyn_if_info.ipv6_shorthand_addr, 
                    dyn_if_info.ipv6_prefix_length);
        } else if (dyn_if_info.ipv6_addr != (char *)0) {
            strcpy(entry->cpqNicIfLogMapIPV6Address, dyn_if_info.ipv6_addr);
        } else {
            strcpy(entry->cpqNicIfLogMapIPV6Address, "N/A");
        }
        entry->cpqNicIfLogMapIPV6Address_len = strlen(entry->cpqNicIfLogMapIPV6Address);

        free_dynamic_interface_info(&dyn_if_info);
    }

    entry->cpqNicIfLogMapSpeedMbps = get_if_speedMbps(entry->cpqNicIfLogMapLACNumber);
    if (entry->cpqNicIfLogMapSpeedMbps <= 4294 )
                entry->cpqNicIfLogMapSpeed = entry->cpqNicIfLogMapSpeedMbps * 1000000;

    switch (ifType) {
        case VBRIDGE:
        case BRIDGE:
            snprintf(buffer, 255, "/sys/class/net/%s/brif", entry->cpqNicIfLogMapLACNumber);
            DEBUGMSGTL(("cpqnic:arch", "looking at bridge interface = %s\n", entry->cpqNicIfLogMapLACNumber));
            if ((dCount = scandir(buffer, &devlist, 
                            file_select, alphasort)) > 0) {
                for (j = 0; j < dCount; j++) {
                    phys_index = name2pindx(devlist[j]->d_name);

                /* logical interface name is used for bonding/teaming */
                    entry->cpqNicIfLogMapPhysicalAdapters[j] = 
                         (char)phys_index;
                }
                entry->cpqNicIfLogMapPhysicalAdapters_len = j;
                entry->cpqNicIfLogMapPhysicalAdapters[j] = 0;
                entry->cpqNicIfLogMapAdapterCount = j;
                free(devlist[0]);
                free(devlist);
            } 

            mapStatus = get_bridge_status(entry);
            break;
        case BOND:
            map = 0;
            get_bondtype(entry->cpqNicIfLogMapLACNumber, entry);
            entry->cpqNicIfLogMapAdapterOKCount = 0;

            /* looking for /sys/class/net/<bond>/slav_* */
            snprintf(buffer, 255, "/sys/class/net/%s", entry->cpqNicIfLogMapLACNumber);
            if ((dCount = scandir(buffer, &devlist, 
                            file_select, alphasort))  != 0) {
                for (j = 0; j < dCount; j++) {
                    if (devlist[j] == NULL)
                        continue;
                    if (!strncmp(devlist[j]->d_name, "slave_", 6) || 
                        !strncmp(devlist[j]->d_name, "lower_", 6)) {
                        DEBUGMSGTL(("cpqnic:arch", "Found slave = %s\n",
                                devlist[j]->d_name));
                        /* now grab the bits after slave_ */
                        phys_index = name2pindx(&devlist[j]->d_name[6]);
    
                        entry->cpqNicIfLogMapPhysicalAdapters[map] =
                            (char)phys_index;
                        map++;
                        entry->cpqNicIfLogMapPhysicalAdapters_len = map + 1;
                    }
                    free(devlist[j]);
                }
                entry->cpqNicIfLogMapAdapterCount = map;
                free(devlist);
            }
            mapStatus = get_bond_status(entry);
            break;
        case NIC:
        case VNIC:
        case VLAN:
            phys_index = name2pindx(entry->cpqNicIfLogMapLACNumber);
            entry->cpqNicIfLogMapPhysicalAdapters[0] = 
                (char)phys_index;
            entry->cpqNicIfLogMapPhysicalAdapters[1] = 0;
            entry->cpqNicIfLogMapPhysicalAdapters_len = 1;
            entry->cpqNicIfLogMapAdapterCount = 1;
        case LOOP:
        case SIT:
        default:
            mapStatus = get_if_status(entry->cpqNicIfLogMapLACNumber);
            switch (mapStatus) {
                case ADAPTER_CONDITION_OK:
                    entry->cpqNicIfLogMapStatus = MAP_STATUS_OK;
                    break;
                case ADAPTER_CONDITION_FAILED:
                    entry->cpqNicIfLogMapStatus = MAP_STATUS_GROUP_FAILED;
                    break;
                case ADAPTER_CONDITION_OTHER:
                default:
                entry->cpqNicIfLogMapCondition = MAP_CONDITION_OTHER;
                    entry->cpqNicIfLogMapStatus = MAP_STATUS_UNKNOWN;
            }
    }

    DEBUGMSGTL(("cpqnic:arch","%s cpqNicIfLogMapCondition = %ld, cpqNicIfLogMapStatus = %ld\n", 
                entry->cpqNicIfLogMapLACNumber,
                entry->cpqNicIfLogMapCondition,
                entry->cpqNicIfLogMapStatus));
    entry->cpqNicIfLogMapCondition = MAKE_CONDITION(entry->cpqNicIfLogMapCondition, mapStatus);
    if (entry->cpqNicIfLogMapCondition !=  entry->cpqNicIfLogMapPrevCondition)
        entry->cpqNicIfLogMapLastChange = uptime();
        
    ifLogMapOverallStatus =  ADAPTER_CONDITION_OK;
    if (cpqNicIfLogMapTable_cache != NULL) {
        iflogmap_container = cpqNicIfLogMapTable_cache->magic;
        it = CONTAINER_ITERATOR(iflogmap_container);
        log_entry = ITERATOR_FIRST( it );
        while ( log_entry != NULL ) {
            ifLogMapOverallStatus = MAKE_CONDITION(ifLogMapOverallStatus, 
                             log_entry->cpqNicIfLogMapCondition);
            log_entry = ITERATOR_NEXT( it );
        }
        ITERATOR_RELEASE( it );
    }
}

cpqNicIfLogMapTable_entry* 
netsnmp_arch_iflogmap_entry_load(netsnmp_container *container, oid ifIndex)
{
    char  ifname[IF_NAMESIZE];
    char *devname;

    int ifType;
    netsnmp_index tmp;
    oid oid_index[2];

    cpqNicIfLogMapTable_entry *entry = NULL;

    devname = if_indextoname((unsigned int) ifIndex, ifname);
    if (devname == NULL)
        return NULL;

    DEBUGMSGTL(("cpqnic:container:load", "Create new Log entry\n"));
    oid_index[0] = ifIndex;
    tmp.len = 1;
    tmp.oids = &oid_index[0];

    entry = CONTAINER_FIND(container, &tmp);
    if (entry == NULL) {
        entry = cpqNicIfLogMapTable_createEntry(container, ifIndex);
        if (NULL == entry) {
            return NULL;
        }
        DEBUGMSGTL(("cpqnic:container:load", "Created new Log entry\n"));

        entry->cpqNicIfLogMapIfNumber[0] = (char)((ifIndex) & 0xff);
        entry->cpqNicIfLogMapIfNumber[1]=(char)(((ifIndex)>>8) & 0xff);
        entry->cpqNicIfLogMapIfNumber[2]=(char)(((ifIndex)>>16) & 0xff);
        entry->cpqNicIfLogMapIfNumber[3]=(char)(((ifIndex)>>24) & 0xff);
        entry->cpqNicIfLogMapIfNumber_len = 4;

        entry->cpqNicIfLogMapPhysicalAdapters[0] = (char)ifIndex;
        entry->cpqNicIfLogMapPhysicalAdapters[1] = (char)0;
        entry->cpqNicIfLogMapPhysicalAdapters_len = 1;
        entry->cpqNicIfLogMapGroupType = MAP_GROUP_TYPE_NONE;
        entry->cpqNicIfLogMapAdapterCount = 1;
        entry->cpqNicIfLogMapAdapterOKCount = 1;

        entry->cpqNicIfLogMapSwitchoverMode = SWITCHOVER_MODE_NONE;
        entry->cpqNicIfLogMapMACAddress_len = 
            setMACaddr(devname,
                    (unsigned char *)entry->cpqNicIfLogMapMACAddress); 
        DEBUGMSGTL(("cpqnic:arch", "mac address_len = %ld\n",
                    entry->cpqNicIfLogMapMACAddress_len));
        entry->cpqNicIfLogMapNumSwitchovers = 0;
        entry->cpqNicIfLogMapHwLocation[0] = '\0';
        entry->cpqNicIfLogMapHwLocation_len = 0;
        entry->cpqNicIfLogMapVlanCount = 0;
        entry->cpqNicIfLogMapVlans[0] = (char)0;

    }

    ifType = get_iftype(devname);

    entry->cpqNicIfLogMapCondition = ADAPTER_CONDITION_OTHER;
    entry->cpqNicIfLogMapPrevCondition = ADAPTER_CONDITION_OTHER;
    entry->cpqNicIfLogMapStatus = MAP_STATUS_UNKNOWN;
    entry->cpqNicIfLogMapPrevStatus = MAP_STATUS_UNKNOWN;

    entry->cpqNicIfLogMapLACNumber_len = strlen(devname);
    strncpy(entry->cpqNicIfLogMapLACNumber, devname,
            entry->cpqNicIfLogMapLACNumber_len);
    strcpy(entry->cpqNicIfLogMapDescription, MapNicType[ifType]);
    entry->cpqNicIfLogMapDescription_len = strlen(MapNicType[ifType]); 
    return entry;
}

int netsnmp_arch_iflog_container_load(netsnmp_container *container)
{

    int      i, iCount;
    struct dirent **filelist;
    int ifIndex;
    int ifType;
    netsnmp_index tmp;
    oid oid_index[2];

    cpqNicIfLogMapTable_entry *entry;
    cpqNicIfLogMapTable_entry *old = NULL;

    DEBUGMSGTL(("cpqnic:arch","netsnmp_arch_iflog_container_load entry\n"));

    ifLogMapOverallStatus = STATUS_OK;

    if ((iCount = scandir("/sys/class/net", 
                          &filelist, file_select, versionsort)) <= 0) {
        if (iCount == -1)
            DEBUGMSGTL(("cpqnic:arch","ERRNO = %d\n", errno));
        return(iCount);
    }
    DEBUGMSGTL(("cpqnic:arch","iflog_container_load %d devs\n",iCount));
    for (i = 0; i < iCount; i++) {
        if ((ifIndex = if_nametoindex(filelist[i]->d_name)) == 0) {
            free(filelist[i]);
            continue;
        }
        DEBUGMSGTL(("cpqnic:arch",
                    "iflog_container_load name = %s index = %d\n",
                    filelist[i]->d_name,
                    ifIndex));
        ifType = get_iftype(filelist[i]->d_name);
        DEBUGMSGTL(("cpqnic:arch", "iflog_container_load type = %d\n",
                    ifType));
        if (ifType  > 0 ) {
            oid_index[0] = ifIndex;
            tmp.len = 1;
            tmp.oids = &oid_index[0];

            DEBUGMSGTL(("cpqnic:container:load", "look for logical NIC =%d\n", 
                        ifIndex));
            old = CONTAINER_FIND(container, &tmp);

            DEBUGMSGTL(("cpqnic:container:load", "Old iflog=%p\n",old));
            if (old != NULL ) {
                DEBUGMSGTL(("cpqnic:container:load", "Re-Use old entry\n"));
                entry = old;
            } else 
                entry = netsnmp_arch_iflogmap_entry_load(container, ifIndex);

            if (entry != NULL) 
                cpqNicIfLogMap_reload_entry(entry);
            if (old == NULL) {
                CONTAINER_INSERT(container, entry);
                DEBUGMSGTL(("cpqnic:arch","Insert entry %s\n",
                        filelist[i]->d_name));
            }
        }
        free(filelist[i]);
    }
    free(filelist);
    numIfLog = CONTAINER_SIZE(container);
    DEBUGMSGTL(("cpqnic:arch"," loaded %d entries\n", numIfLog));

    cpqHoMibHealthStatusArray[CPQMIBHEALTHINDEX] = ifLogMapOverallStatus;

    cpqHostMibStatusArray[CPQMIB].cond = ifLogMapOverallStatus;

    DEBUGMSGTL(("cpqnic:arch","ifLogMapOverallStatus = %d\n", ifLogMapOverallStatus));
    return(0);
}

long get_cpqNicIfLogMapOverallCondition( void )
{
    return ( ifLogMapOverallStatus );
}

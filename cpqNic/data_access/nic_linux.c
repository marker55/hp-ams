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

#include "cpqHoFwVerTable.h"
#include "cpqNicIfLogMapTable.h"
#include "cpqNicIfPhysAdapterTable.h"

#include "hpHelper.h"
#include "cpqNic.h"
#include "cpqnic.h"
#include "nic_linux.h"
#include "nic_db.h"
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

#define MAX_UINT_32 0xffffffff

extern unsigned char cpqHoMibHealthStatusArray[];
extern oid       cpqHoFwVerTable_oid[];
extern size_t    cpqHoFwVerTable_oid_len;

static struct ifconf ifc;
static struct ifreq  ifr;
char    ifLogMapOverallStatus = 2;

#define SYSBUSPCISLOTS "/sys/bus/pci/slots/"
#define PROCFILE "/proc/net/if_inet6"

char *nic_names[] = {"eth",
    "em",
    "peth",
    "vmnic",
    (char *) 0};

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

#define BRIDGE 1
#define VBRIDGE 2
#define BOND 3
#define LOOP 4
#define SIT  5
#define VNIC  6
#define NIC  7

#define UNKNOWN_BOND        -1
#define ROUND_ROBIN_BOND    0
#define ACTIVE_BACKUP_BOND  1
#define XOR_BOND            2
#define BROADCAST_BOND      3
#define IEEE_8023AD_BOND    4
#define TLB_BOND            5
#define ALB_BOND            6

char* pch_nl;
#define CLOBBER_NL(pstr)                        \
{                                               \
    pch_nl = strchr(pstr,'\n');                 \
    if (pch_nl != (char*)0)                     \
    *pch_nl = '\0';                         \
}

/* added for tracking ifLastChange; rQm 235021 */
/* NOTE: reader/writer must hold thread lock   */
typedef struct tIfLastChange {  
    char *pchInterfaceName;               /* eth0, bond0, etc.               */
    unsigned long ulLastChanged;          /* sysUpTime at last status change */
    struct tIfLastChange *pstrNext; 
} IfLastChange;

IfLastChange *pstrIfLastChange = (IfLastChange*)0;
/* used as initial value of pchInterfaceName above so we don't have */
/* to check for null ptr before strcmp                              */
char *pchNoInterface = "-";
#define NUM_IF_LAST_CHANGE_ENTRIES  10

#define     LINES_TO_ALLOC   10    /* number of lines to alloc at a time */
/* for reading procfs...              */

int iSetIfLastChange(char *, unsigned long);
int32_t get_pcislot(char *);
static void GetConfSpeedDuplex(cpqNicIfPhysAdapterTable_entry *);

oid sysUpTimeOid[] = {1, 3, 6, 1, 2, 1, 1, 3, 0};

void free_ethtool_info_members(ethtool_info *); 
int32_t get_ethtool_info(char *, ethtool_info *);
static int send_cpqnic_trap(int, cpqNicIfPhysAdapterTable_entry*);

int get_bondtype(char *netdev)
{
    char buffer[256];
    char *mode;
    int ret = -1;

    sprintf(buffer, "/sys/class/net/%s/bonding/mode", netdev);

    if ((mode = get_sysfs_str(buffer)) == NULL)
        return ret;
    sscanf(mode, "%s %d", buffer, &ret);
    free(mode);
    return ret;
}

int get_iftype(char *netdev) 
{
    struct dirent **filelist;
    struct stat sb;
    char buffer[256];

    int count;
    int i=0;


    while (nic_names[i] != (char *) 0) {
        if (!strncmp(netdev, nic_names[i], strlen(nic_names[i]))) 
            return NIC;
        i++;
    }
    sprintf(buffer, "/sys/class/net/%s/brport", netdev);
    if (stat(buffer, &sb) == 0) {
        sprintf(buffer, "/sys/class/net/%s/device", netdev);
        if (stat(buffer, &sb) != 0) 
            return VNIC;
        else 
            return NIC;
    }

    sprintf(buffer, "/sys/class/net/%s/bonding", netdev);
    if (stat(buffer, &sb) == 0)
        return BOND;

    sprintf(buffer, "/sys/class/net/%s/brif/", netdev);
    if (stat(buffer, &sb) == 0) {
        if ((count = scandir(buffer, &filelist, file_select, alphasort)) > 0 ) {
            i = 0;
            while (nic_names[i] != (char *) 0) {
                if (!strncmp(filelist[0]->d_name, nic_names[i], 
                            strlen(nic_names[i]))) {
                    free(filelist[0]);
                    free(filelist);
                    return BRIDGE;
                } else
                    i++;
            }
            free(filelist[0]);
            free(filelist);
            return VBRIDGE;
        }
        else 
            return VBRIDGE;
    }
    if (!strcmp(netdev, "lo"))
        return LOOP;

    if (!strncmp(netdev, "sit", 3))
        return SIT;

    return 0;
}

int setMACaddr(char * name, unsigned char * MacAddr) {

    unsigned int MAC[6];
    char  *buffer;
    char  address[256];
    char  address_len[256];
    int len = 0;
    int j;

    sprintf(address, "/sys/class/net/%s/address", name);
    sprintf(address_len, "/sys/class/net/%s/addr_len", name);

    if ((buffer = get_sysfs_str(address)) != NULL )  {
        DEBUGMSGTL(("cpqnic:arch","MAC length file = %s\n",address_len));

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

int get_if_status(char *interface) {

    char attribute[256];
    char *value;
    int link;
    unsigned short iff;

    DEBUGMSGTL(("cpqnic:container:load", "%s = %x\n", 
                attribute, get_sysfs_shex(attribute)));

    strcpy(attribute, interface);
    strcat(attribute, "/flags");

    iff = get_sysfs_shex(attribute);
    if (iff & IFF_UP) {
        strcpy(attribute, interface);
        strcat(attribute, "/carrier");
        link = get_sysfs_int(attribute);
        if (link) 
            return ADAPTER_CONDITION_OK;

        strcpy(attribute, interface);
        strcat(attribute, "/operstate");
        if ((value = get_sysfs_str(attribute)) != NULL) {
            DEBUGMSGTL(("cpqnic:container:load", "%s = %s\n", attribute, value));
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
    } else 
        return ADAPTER_CONDITION_OTHER;
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

    char buffer[256];
    char attribute[256];
    char *value;
    int     iMacLoop;
    oid     Index;
    netsnmp_index tmp;
    oid oid_index[2];

    static int sock = -1;
    struct ifreq ifr;

    nic_hw_db *pnic_hw_db;
    ethtool_info einfo;
    static int32_t mlxport = -1;

    static int devcount = -1;
    struct dirent **devlist;
    int     i, j = 0;
    int     domain,bus,device,function;
    char    sysfsDir[1024];
    char    linkBuf[1025];
    char    mibStatus =  MIB_CONDITION_OK;
    ssize_t link_sz;
    cpqNicIfPhysAdapterTable_entry* entry;
    cpqNicIfPhysAdapterTable_entry* old;

    unsigned short VendorID, DeviceID, SubsysVendorID, SubsysDeviceID;
    int rc;

    DEBUGMSGTL(("cpqnic:arch","netsnmp_arch_ifphys_container_load entry\n"));

    if ((devcount = scandir("/sys/class/net/",
                    &devlist, file_select, alphasort)) <= 0) {
        cpqHoMibHealthStatusArray[CPQMIBHEALTHINDEX] = mibStatus;
        cpqHostMibStatusArray[CPQMIB].cond = mibStatus;
        return(0);
    }
    DEBUGMSGTL(("cpqnic:container:load", "devcount =%u\n",  devcount));
    for (i = 0; i < devcount; i++) {
        memset(buffer, 0, 256);
        sprintf(buffer, "/sys/class/net/%s", devlist[i]->d_name);

        strcpy(attribute, buffer);
        strcat(attribute, "/device");
        DEBUGMSGTL(("cpqnic:container:load", "attribute = %s\n", attribute));
        if ((link_sz = readlink(attribute, linkBuf, 1024)) < 0) {
            free(devlist[i]);
            continue; 
        }
        linkBuf[link_sz] = 0;
        Index = name2indx(devlist[i]->d_name);
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
        } else {
            entry = cpqNicIfPhysAdapterTable_createEntry(container, Index);
            if (NULL == entry) {
                free(devlist[i]);
                continue;   /* error already logged by function */
            }

            entry->cpqNicIfPhysAdapterIoBayNo = UNKNOWN_SLOT;
            entry->cpqNicIfPhysAdapterVirtualPortNumber = UNKNOWN_PORT;

            sscanf(strrchr(&linkBuf[0],'/') + 1, "%x:%x:%x.%x",
                    &domain, &bus, &device, &function);
            entry->PCI_Bus = bus;
            entry->PCI_Slot = ((device & 0xFF) << 3) | (function & 0xFF);
            /* init values to defaults in case they are not in procfs */
            entry->cpqNicIfPhysAdapterSlot = -1;
            entry->cpqNicIfPhysAdapterPort = function + 1;
            DEBUGMSGTL(("cpqnic:data", "PCI Function = %d\n", function));

            entry->cpqNicIfPhysAdapterVirtualPortNumber = -1;
            entry->cpqNicIfPhysAdapterIoBayNo = -1;
            entry->cpqNicIfPhysAdapterSpeedMbps = UNKNOWN_SPEED;
            entry->cpqNicIfPhysAdapterConfSpeedDuplex = 
                CONF_SPEED_DUPLEX_OTHER;
            entry->AdapterAutoNegotiate = 0;
            entry->cpqNicIfPhysAdapterMemAddr = 0;
            entry->cpqNicIfPhysAdapterAggregationGID = -1;
            entry->cpqNicIfPhysAdapterFWVersion[0] = '\0';
            strcpy(entry->cpqNicIfPhysAdapterPartNumber, "UNKNOWN");
            /* default "empty" per mib */
            strcpy(entry->cpqNicIfPhysAdapterName, "EMPTY"); 

            entry->cpqNicIfPhysAdapterRole = ADAPTER_ROLE_NOTAPPLICABLE;

            entry->cpqNicIfPhysAdapterIfNumber[0] = (char)(Index & 0xff);
            entry->cpqNicIfPhysAdapterIfNumber[1] = (char)((Index>>8)  & 0xff);
            entry->cpqNicIfPhysAdapterIfNumber[2] = (char)((Index>>16) & 0xff);
            entry->cpqNicIfPhysAdapterIfNumber[3] = (char)((Index>>24) & 0xff);

            entry->cpqNicIfPhysAdapterIfNumber[4] = 0;
            entry->cpqNicIfPhysAdapterIfNumber[5] = 0;
            entry->cpqNicIfPhysAdapterIfNumber[6] = 0;
            entry->cpqNicIfPhysAdapterIfNumber[7] = 0;
            entry->cpqNicIfPhysAdapterIfNumber_len = 8;

            if (sock < 0) 
                sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
            if (sock >= 0) {
                memset(&ifr,0,sizeof(ifr));
                ifr.ifr_data = (char*)0;
                if (ioctl(sock, SIOCGIFMAP, &ifr) >= 0) {
                    entry->cpqNicIfPhysAdapterIoAddr = ifr.ifr_map.base_addr;
                    entry->cpqNicIfPhysAdapterDma = ((int)ifr.ifr_map.dma == 0)?
                        -1:(int)ifr.ifr_map.dma;

                    if (ifr.ifr_map.mem_start) 
                        entry->cpqNicIfPhysAdapterMemAddr = 
                            ifr.ifr_map.mem_start;
                }
            }

            entry->cpqNicIfPhysAdapterHwLocation[0] = '\0';
            entry->cpqNicIfPhysAdapterHwLocation_len = 0;

            strcpy(attribute, buffer);
            strcat(attribute, "/device/vendor");
            VendorID = get_sysfs_shex(attribute); 

            strcpy(attribute, buffer);
            strcat(attribute, "/device/device");
            DeviceID = get_sysfs_shex(attribute); 

            strcpy(attribute, buffer);
            strcat(attribute, "/device/subsystem_vendor");
            SubsysVendorID = get_sysfs_shex(attribute); 

            strcpy(attribute, buffer);
            strcat(attribute, "/device/subsystem_device");
            SubsysDeviceID = get_sysfs_shex(attribute);

            DEBUGMSGTL(("cpqnic:container:load","Vendor = %x, Device = %x, " 
                        "Subsys Vendor = %x, Subsys Device = %x\n",
                        VendorID, DeviceID,
                        SubsysVendorID, SubsysDeviceID));
            /* get model name and part number from database created struct */
            pnic_hw_db = get_nic_hw_info(VendorID, DeviceID,
                                         SubsysVendorID, SubsysDeviceID);
            if (pnic_hw_db != NULL) {
                /* over-ride info read from procfs/hpetfe */
                strcpy(entry->cpqNicIfPhysAdapterName, pnic_hw_db->pname);
                entry->cpqNicIfPhysAdapterName_len =
                    strlen(entry->cpqNicIfPhysAdapterName);
                strcpy(entry->cpqNicIfPhysAdapterPartNumber, 
                        pnic_hw_db->ppca_part_number);
                entry->cpqNicIfPhysAdapterPartNumber_len = 
                    strlen(entry->cpqNicIfPhysAdapterPartNumber);
            }

            strcpy(attribute, buffer);
            strcat(attribute, "/device/irq");
            entry->cpqNicIfPhysAdapterIrq = get_sysfs_int(attribute);

            if (get_ethtool_info(devlist[i]->d_name, &einfo) == 0) {
                DEBUGMSGTL(("cpqnic:container:load", "Got ethtool info for %s\n", 
                            devlist[i]->d_name));  
                DEBUGMSGTL(("cpqnic:container:load", "ethtool Duplex info for %s = %d\n",
                            devlist[i]->d_name, einfo.duplex));
                entry->AdapterAutoNegotiate =  einfo.autoneg; 
                entry->cpqNicIfPhysAdapterDuplexState =  UNKNOWN_DUPLEX; 
                if (einfo.duplex == DUPLEX_FULL)  
                    entry->cpqNicIfPhysAdapterDuplexState =  FULL_DUPLEX; 
                if (einfo.duplex == DUPLEX_HALF)  
                    entry->cpqNicIfPhysAdapterDuplexState =  HALF_DUPLEX; 

                /*
                 * Mellanox Card NC542m has bus, device and function 
                 * same for both  the ports.  
                 */

                if ((VendorID == 0x15B3) && (DeviceID == 0x5A44) && 
                        (SubsysVendorID == 0x3107) && (SubsysDeviceID == 0x103C)) {
                    mlxport++;
                }		

                entry->cpqNicIfPhysAdapterSlot = getPCIslot_str(einfo.bus_info);

                if (entry->cpqNicIfPhysAdapterSlot > 0 ) {
                    DEBUGMSGTL(("cpqnic:container:load", "Got pcislot info for %s = %u\n",
                                devlist[i]->d_name,
                                entry->cpqNicIfPhysAdapterSlot));

                    if (entry->cpqNicIfPhysAdapterSlot > 0x1000)
                        entry->cpqNicIfPhysAdapterHwLocation_len =
                            snprintf(entry->cpqNicIfPhysAdapterHwLocation,
                                    256,
                                    "Blade %u, Slot %u",
                                    (entry->cpqNicIfPhysAdapterSlot>>8) & 0xF,
                                    (entry->cpqNicIfPhysAdapterSlot>>16) & 0xFF);
                    else
                        entry->cpqNicIfPhysAdapterHwLocation_len =
                            snprintf(entry->cpqNicIfPhysAdapterHwLocation,
                                    256,
                                    "Slot %u",
                                    entry->cpqNicIfPhysAdapterSlot);
                } else {
                    entry->cpqNicIfPhysAdapterPort = getChassisPort_str(einfo.bus_info);
                    entry->cpqNicIfPhysAdapterHwLocation[0] = '\0';
                    entry->cpqNicIfPhysAdapterHwLocation_len = 0;
                }

                if ( einfo.firmware_version ) {
                    strcpy(entry->cpqNicIfPhysAdapterFWVersion, 
                            einfo.firmware_version);
                    entry->cpqNicIfPhysAdapterFWVersion_len = 
                        strlen(entry->cpqNicIfPhysAdapterFWVersion);
                    nic_fw_idx = register_FW_version(-1,  /* direction */
                                                     nic_fw_idx, 
                                                     3, /* Category */
                                                     24,  /*type */
                                                     3, /*update method */
                                                     einfo.firmware_version,
                                                     entry->cpqNicIfPhysAdapterName,
                                                     entry->cpqNicIfPhysAdapterHwLocation,
                                                     "");
                }
            } else 
                DEBUGMSGTL(("cpqnic:container:load", "ethtool FAILED for %s\n",
                            devlist[i]->d_name));

            if (entry->cpqNicIfPhysAdapterSpeedMbps == UNKNOWN_SPEED) {
                entry->cpqNicIfPhysAdapterSpeedMbps = einfo.link_speed;
                if (entry->cpqNicIfPhysAdapterSpeedMbps != UNKNOWN_SPEED) {
                    if (entry->cpqNicIfPhysAdapterSpeedMbps <= 4294 )
                        entry->cpqNicIfPhysAdapterSpeed = 
                                 entry->cpqNicIfPhysAdapterSpeedMbps * 1000000;
                }
            }

            for (iMacLoop=0; iMacLoop<MAC_ADDRESS_BYTES; iMacLoop++) {
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
            DEBUGMSGTL(("cpqnic:container:load", 
                        "Finished processing  ethtool info for %s\n", 
                        devlist[i]->d_name));
        } 

        GetConfSpeedDuplex(entry);

        entry->cpqNicIfPhysAdapterPrevCondition = entry->cpqNicIfPhysAdapterCondition;
        entry->cpqNicIfPhysAdapterCondition = ADAPTER_CONDITION_OTHER;
        entry->cpqNicIfPhysAdapterPrevStatus = entry->cpqNicIfPhysAdapterStatus;
        entry->cpqNicIfPhysAdapterStatus = STATUS_UNKNOWN;

        mibStatus = get_if_status(buffer);
        entry->cpqNicIfPhysAdapterCondition = 
                MAKE_CONDITION(entry->cpqNicIfPhysAdapterCondition, mibStatus);
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
        if (old == NULL) {
            rc = CONTAINER_INSERT(container, entry);
            DEBUGMSGTL(("cpqnic:arch","Insert entry %s\n",
                        devlist[i]->d_name));
            j++;
        }
        DEBUGMSGTL(("cpqnic:arch","Check NIC trap = %s Status = %d, Prev = %d\n",
                        devlist[i]->d_name,
                        entry->cpqNicIfPhysAdapterStatus,
                        entry->cpqNicIfPhysAdapterPrevStatus));
        free(devlist[i]);

        if ((entry->cpqNicIfPhysAdapterStatus == STATUS_LINK_FAILURE) &&
            (entry->cpqNicIfPhysAdapterPrevStatus != STATUS_LINK_FAILURE))
            send_cpqnic_trap(LINK_DOWN_TRAP_OID, entry);
        else if ((entry->cpqNicIfPhysAdapterPrevStatus == STATUS_LINK_FAILURE) 
                &&
            (entry->cpqNicIfPhysAdapterStatus != STATUS_LINK_FAILURE))
            send_cpqnic_trap(LINK_UP_TRAP_OID, entry);
    }
    numIfPhys = CONTAINER_SIZE(container);
    DEBUGMSGTL(("cpqnic:arch"," loaded %d entries\n", numIfPhys));
    free(devlist);
    return(0);
}


int netsnmp_arch_iflog_container_load(netsnmp_container *container)
{

    int      i, j, k, iCount, dCount;
    struct dirent **filelist;
    struct dirent **devlist;
    char netdev[256] = "/sys/class/net/";
    int netdev_sz;
    int ifIndex;
    int ifType;
    int phys_index;
    int map = 0;
    int rc;
    char chTemp[256];
    char *pchTemp;
    int bondtype;
    int mapStatus;
    netsnmp_index tmp;
    oid oid_index[2];
    netsnmp_container *ifphys_container;
    netsnmp_iterator  *it;
    netsnmp_cache *ifphys_cache;
    cpqNicIfPhysAdapterTable_entry* ifphys_entry;

    cpqNicIfLogMapTable_entry *entry;
    cpqNicIfLogMapTable_entry *old = NULL;
    netsnmp_cache  *cpqNicIfPhysAdapterTable_cache = NULL;

    dynamic_interface_info dyn_if_info;

    DEBUGMSGTL(("cpqnic:arch","netsnmp_arch_iflog_container_load entry\n"));

    ifLogMapOverallStatus = STATUS_OK;
    cpqNicIfPhysAdapterTable_cache =
                     netsnmp_cache_find_by_oid(cpqNicIfPhysAdapterTable_oid,
                                            cpqNicIfPhysAdapterTable_oid_len);

    netdev_sz = strlen(netdev);
    if ((iCount = scandir(netdev, &filelist, file_select, versionsort))
            == 0)
        return(iCount);
    DEBUGMSGTL(("cpqnic:arch","iflog_container_load %d devs\n",iCount));
    for (i = 0; i < iCount; i++) {
        if ((ifIndex = name2indx(filelist[i]->d_name)) == -1) {
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
            } else {
                entry = cpqNicIfLogMapTable_createEntry(container, ifIndex); 
                if (entry == NULL) {
                    free(filelist[i]);
                    continue;
                }
                entry->cpqNicIfLogMapIfNumber[0] = (char)((ifIndex) & 0xff);
                entry->cpqNicIfLogMapIfNumber[1]=(char)(((ifIndex)>>8) & 0xff);
                entry->cpqNicIfLogMapIfNumber[2]=(char)(((ifIndex)>>16) & 0xff);
                entry->cpqNicIfLogMapIfNumber[3]=(char)(((ifIndex)>>24) & 0xff);
                entry->cpqNicIfLogMapIfNumber_len = 4;

                entry->cpqNicIfLogMapPhysicalAdapters[0] = (char)ifIndex;
                entry->cpqNicIfLogMapPhysicalAdapters_len = 1;
                entry->cpqNicIfLogMapGroupType = MAP_GROUP_TYPE_NONE;
                entry->cpqNicIfLogMapAdapterCount = 1;
                entry->cpqNicIfLogMapAdapterOKCount = 1;

                entry->cpqNicIfLogMapSwitchoverMode = SWITCHOVER_MODE_NONE;
                entry->cpqNicIfLogMapMACAddress_len = 
                    setMACaddr(filelist[i]->d_name,
                            (unsigned char *)entry->cpqNicIfLogMapMACAddress); 
                DEBUGMSGTL(("cpqnic:arch", "mac address_len = %ld\n",
                            entry->cpqNicIfLogMapMACAddress_len));
                entry->cpqNicIfLogMapNumSwitchovers = 0;
                entry->cpqNicIfLogMapHwLocation[0] = '\0';
                entry->cpqNicIfLogMapVlanCount = 0;
                entry->cpqNicIfLogMapVlans[0] = (char)0;

            }
            entry->cpqNicIfLogMapCondition = ADAPTER_CONDITION_OTHER;
            entry->cpqNicIfLogMapStatus = MAP_STATUS_UNKNOWN;
            sprintf(chTemp, "/sys/class/net/%s", filelist[i]->d_name);
            mapStatus = get_if_status(chTemp);
            entry->cpqNicIfLogMapCondition = MAKE_CONDITION(entry->cpqNicIfLogMapCondition, mapStatus);
            entry->cpqNicIfLogMapStatus = entry->cpqNicIfLogMapCondition;

            bondtype = -1;
            entry->cpqNicIfLogMapDescription[0] = (char)0;
            entry->cpqNicIfLogMapDescription_len = 0; 

            entry->cpqNicIfLogMapLACNumber_len = strlen(filelist[i]->d_name);
            strncpy(entry->cpqNicIfLogMapLACNumber, filelist[i]->d_name,
                    entry->cpqNicIfLogMapLACNumber_len);
            switch (ifType) {
                case LOOP:
                case SIT:
                    break;
                case NIC:
                case VNIC:
                    phys_index = name2indx(filelist[i]->d_name);
                    entry->cpqNicIfLogMapPhysicalAdapters[0] = 
                        (char)phys_index;
                    entry->cpqNicIfLogMapPhysicalAdapters[1] =0;
                    entry->cpqNicIfLogMapPhysicalAdapters_len = 1;
                    entry->cpqNicIfLogMapAdapterCount = 1;
                    break;
                case VBRIDGE:
                case BRIDGE:
                    strcpy(&netdev[netdev_sz], filelist[i]->d_name);
                    strcat(netdev, "/brif");
                    if ((dCount = scandir(netdev, &devlist, 
                                    file_select, alphasort)) == 1) {
                        phys_index = name2indx(devlist[0]->d_name);

                        /* logical interface name is used for bonding/teaming */
                        entry->cpqNicIfLogMapPhysicalAdapters[0] = 
                            (char)phys_index;
                        entry->cpqNicIfLogMapPhysicalAdapters_len = 1;
                        entry->cpqNicIfLogMapPhysicalAdapters[1] =0;
                        entry->cpqNicIfLogMapAdapterCount = 1;
                        free(devlist[0]);
                        free(devlist);
                    } 
                    break;
                case BOND:
                    bondtype = get_bondtype(filelist[i]->d_name);
                    if (bondtype < 7){
                        strcpy(entry->cpqNicIfLogMapDescription,
                                MapGroupType[bondtype + 1]);
                        entry->cpqNicIfLogMapDescription_len = 
                            strlen(MapGroupType[bondtype + 1]);
                    } 
                    switch (bondtype) {
                        case ACTIVE_BACKUP_BOND:
                            entry->cpqNicIfLogMapGroupType = MAP_GROUP_TYPE_NFT;
                            entry->cpqNicIfLogMapSwitchoverMode =
                                SWITCHOVER_MODE_SWITCH_ON_FAIL;
                            break;

                        case ROUND_ROBIN_BOND:
                        case XOR_BOND:
                        case BROADCAST_BOND:
                            entry->cpqNicIfLogMapGroupType = MAP_GROUP_TYPE_SLB;

                            /* due to a lack of other values for this 
                               mib object, we must assign the switchover mode
                               to the "best fit" value of switch on fail.
                               */
                            entry->cpqNicIfLogMapSwitchoverMode =
                                SWITCHOVER_MODE_SWITCH_ON_FAIL;
                            break;

                        case IEEE_8023AD_BOND:
                            entry->cpqNicIfLogMapGroupType = MAP_GROUP_TYPE_AD;
                            entry->cpqNicIfLogMapSwitchoverMode =
                                SWITCHOVER_MODE_SWITCH_ON_FAIL;
                            break;

                        case TLB_BOND:
                            entry->cpqNicIfLogMapGroupType = MAP_GROUP_TYPE_TLB;
                            entry->cpqNicIfLogMapSwitchoverMode =
                                SWITCHOVER_MODE_SWITCH_ON_FAIL;
                            break;

                        case ALB_BOND:
                            entry->cpqNicIfLogMapGroupType = MAP_GROUP_TYPE_ALB;
                            entry->cpqNicIfLogMapSwitchoverMode =
                                SWITCHOVER_MODE_SWITCH_ON_FAIL;
                            break;

                        case UNKNOWN_BOND:
                            entry->cpqNicIfLogMapGroupType = 
                                MAP_GROUP_TYPE_NOT_IDENTIFIABLE;
                            entry->cpqNicIfLogMapSwitchoverMode =
                                SWITCHOVER_MODE_UNKNOWN;
                            break;
                        default:
                            DEBUGMSGTL(("cpqnic:arch",
                                        "ERROR: cmanic, unknown bonding mode "
                                        "for %s\n",
                                        filelist[i]->d_name));
                            break;
                    }

                    strcpy(&netdev[netdev_sz], filelist[i]->d_name);
                    if ((dCount = scandir(netdev, &devlist, 
                                    file_select, alphasort))  != 0) {
                        for (j = 0; j < dCount; j++) {
                            int if_status = ADAPTER_CONDITION_OTHER;
                            if (devlist[j] == NULL)
                                continue;
                            if (strncmp(devlist[j]->d_name, "slave_", 6)) {
                                free(devlist[j]);
                                continue;
                            }
                            phys_index = name2indx(&devlist[j]->d_name[6]);
                            entry->cpqNicIfLogMapPhysicalAdapters[map] =
                                (char)phys_index;
                            entry->cpqNicIfLogMapPhysicalAdapters_len = map + 1;
                            map++;
                            sprintf(chTemp, "/sys/class/net/%s", devlist[j]->d_name);
                            if_status = get_if_status(chTemp);
                            if ((if_status == ADAPTER_CONDITION_OK) && 
                                (entry->cpqNicIfLogMapCondition == ADAPTER_CONDITION_OK)) {
                                entry->cpqNicIfLogMapCondition = ADAPTER_CONDITION_OK;
                                entry->cpqNicIfLogMapStatus = STATUS_OK;

                            } else if ((if_status == ADAPTER_CONDITION_OK) &&
                                ((entry->cpqNicIfLogMapCondition == ADAPTER_CONDITION_DEGRADED) ||
                                 (entry->cpqNicIfLogMapCondition == ADAPTER_CONDITION_FAILED))) {
                                entry->cpqNicIfLogMapCondition = ADAPTER_CONDITION_DEGRADED;
                                entry->cpqNicIfLogMapStatus = STATUS_GENERAL_FAILURE;
                            }  else if ((if_status == ADAPTER_CONDITION_FAILED) &&
                                ((entry->cpqNicIfLogMapCondition == ADAPTER_CONDITION_DEGRADED) ||
                                 (entry->cpqNicIfLogMapCondition == ADAPTER_CONDITION_OK))) {
                                entry->cpqNicIfLogMapCondition = ADAPTER_CONDITION_DEGRADED;
                                entry->cpqNicIfLogMapStatus = STATUS_GENERAL_FAILURE;
                            }
                            free(devlist[j]);
                        }
                        entry->cpqNicIfLogMapAdapterCount = map;
                        free(devlist);
                    }
                    if (entry->cpqNicIfLogMapCondition == ADAPTER_CONDITION_OK)
                        entry->cpqNicIfLogMapStatus = MAP_STATUS_OK;
                    else if (entry->cpqNicIfLogMapCondition == 
                            ADAPTER_CONDITION_FAILED)
                        entry->cpqNicIfLogMapStatus = MAP_STATUS_GROUP_FAILED;
                    else
                        entry->cpqNicIfLogMapStatus = MAP_STATUS_UNKNOWN;

                    break;
            } 

            map = 0;
            /* get IPv4/6 address info */
            if (get_dynamic_interface_info(filelist[i]->d_name, &dyn_if_info) 
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

            entry->cpqNicIfLogMapSpeedMbps = UNKNOWN_SPEED;
            entry->cpqNicIfLogMapSpeed = UNKNOWN_SPEED;
            /* calculate interface speeds from cpqNicIfPhysAdapterTable */
            if (cpqNicIfPhysAdapterTable_cache != NULL) {
                ifphys_container = cpqNicIfPhysAdapterTable_cache->magic;
                it = CONTAINER_ITERATOR( ifphys_container );
                ifphys_entry = ITERATOR_FIRST( it );
                for (k = 0; k < entry->cpqNicIfLogMapAdapterCount; k++) {
                    while ( ifphys_entry != NULL ) {
                        if (entry->cpqNicIfLogMapPhysicalAdapters[0] ==
                            (char) ifphys_entry->cpqNicIfPhysAdapterIndex) {
                            entry->cpqNicIfLogMapSpeedMbps += 
                                ifphys_entry->cpqNicIfPhysAdapterSpeedMbps;
                        }
                        ifphys_entry = ITERATOR_NEXT( it );
                    }
                }
                if (entry->cpqNicIfLogMapSpeedMbps != UNKNOWN_SPEED) {
                    if (entry->cpqNicIfLogMapSpeedMbps <= 4294 )
                        entry->cpqNicIfLogMapSpeed = 
                                    entry->cpqNicIfLogMapSpeedMbps * 1000000;
                }
                ITERATOR_RELEASE( it );
            }

            ifLogMapOverallStatus = MAKE_CONDITION(ifLogMapOverallStatus, 
                                                   entry->cpqNicIfLogMapCondition);
            /* logical interface name is used for bonding/teaming */
            if (entry->cpqNicIfLogMapCondition == ADAPTER_CONDITION_OK)
                entry->cpqNicIfLogMapStatus = MAP_STATUS_OK;
            if (old == NULL) {
                rc = CONTAINER_INSERT(container, entry);
                DEBUGMSGTL(("cpqnic:arch",
                            "Inserted cpqNicIfLogMapTable entry %s rc = %d\n",
                            filelist[i]->d_name, rc));
                j++;
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
 * |               Send the cpqnic linkup (18005) or
 * |               linkdown (18006) trap to iLO                             |
 * |  Returns    :                                                          |
 * |               zero on success; non-zero on failure                     |
 * |                                                                        |
 * \*----------------------------------------------------------------------*/
static int send_cpqnic_trap(int specific_type,     
                            cpqNicIfPhysAdapterTable_entry* nic)
{

    int fd;
    register struct sockaddr_in *sin;
    int intVal = 0;
    char *ipDigit;
    struct utsname sys_name;
    char cpqSiServerSystemId[SYSTEM_ID_LEN + 14]; /* +14 for "Not Available" */
    char cpqSePciSlotBoardName[BOARD_NAME_LEN];
    char cpqNicIfPhysAdapterPartNumber[PART_NUMBER_LEN];
    char ipAdEntAddr[IPADDR_LEN]; /* IPv4 address of failed/restored link */
    unsigned char ipAddr[4];
    char cpqNicIfLogMapIPV6Address[IPADDR_LEN]; /* IPv6 address of link */
    FILE           *in;
    char            line[80], addr[40];

    unsigned char *pRecord = NULL;
    PCQSMBIOS_SERV_SYS_ID pServSysId;
    int recCount = 0, iLoop;
    cpqNicIfLogical_t *pIfLog;
    netsnmp_variable_list *var_list = NULL;

    static oid compaq[] = { 1, 3, 6, 1, 4, 1, 232 };
    static oid compaq_len = 7;

    /* add trap indexes to each object if possible */
    static oid sysNameOid[] = {1, 3, 6, 1, 2, 1, 1, 5, 0}; /* true index */
    static oid cpqHoTrapFlagsOid[]
        = {1, 3, 6, 1, 4, 1, 232, 11, 2, 11, 1, 0}; /* true index */
    static oid cpqNicIfPhysAdapterSlotOid[]
        = {1, 3, 6, 1, 4, 1, 232, 18, 2, 3, 1, 1, 5, 255};  /*index holder*/
    static oid cpqNicIfPhysAdapterPortOid[]
        = {1, 3, 6, 1, 4, 1, 232, 18, 2, 3, 1, 1, 10, 255}; /*index holder*/
    static oid cpqSiServerSystemIdOid[]
        = {1, 3, 6, 1, 4, 1, 232, 2, 2, 4, 17, 0}; /* true index */
    static oid cpqNicIfPhysAdapterStatusOid[]
        = {1, 3, 6, 1, 4, 1, 232, 18, 2, 3, 1, 1, 14, 255}; /*index holder*/
    static oid cpqSePciSlotBoardNameOid[]
        = {1, 3, 6, 1, 4, 1, 232, 1, 2, 13, 1, 1, 5, 255,255}; /*idxholder*/
    static oid cpqNicIfPhysAdapterPartNumberOid[]
        = {1, 3, 6, 1, 4, 1, 232, 18, 2, 3, 1, 1, 32, 255}; /*index holder*/
    static oid ipAdEntAddrOid[]
        = {1, 3, 6, 1, 2, 1, 4, 20, 1, 1, 0, 0, 0, 0}; /*index holder*/
    static oid cpqNicIfLogMapIPV6AddressOid[]
        = {1, 3, 6, 1, 4, 1, 232, 18, 2, 4, 1, 1, 6, 255}; /*index holder*/



    strcpy(cpqSiServerSystemId, "Not Available");
    strcpy(cpqSePciSlotBoardName, "EMPTY"); /* default "empty" per mib */
    strcpy(cpqNicIfPhysAdapterPartNumber, "Not Available");

    /* find the slot and port associated with this interface */
    if ( nic->cpqNicIfPhysAdapterIndex >= 0 ) {
        /* Get SiServerSystemId from SmBIOS  */
        SmbGetRecordByType(SMBIOS_HPOEM_SYSID, recCount, (void *)&pRecord);
        pServSysId = (PCQSMBIOS_SERV_SYS_ID)pRecord;
        strcpy(cpqSiServerSystemId, (char *)pServSysId->serverSystemIdStr);

        /* get IPv4/6 address associated with this interface */
        strcpy(ipAdEntAddr, "0.0.0.0");
        if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0) {
            memset(&ifr, 0, sizeof(struct ifreq));
            if (if_indextoname(nic->cpqNicIfPhysAdapterIndex, ifr.ifr_name) 
                        != NULL) {
                DEBUGMSGTL(("cpqNic:ethtool",
                        "name = %s\n",
                        ifr.ifr_name));

                if (ioctl(fd, SIOCGIFADDR, &ifr) >= 0) {
                    sin = (struct sockaddr_in *) &ifr.ifr_addr;
                    strcpy(ipAdEntAddr,
                           inet_ntoa(sin->sin_addr));
		}
            }
            close(fd);
        }

        strcpy(cpqNicIfLogMapIPV6Address, "0.0.0.0");
        if ((in = fopen(PROCFILE, "r"))) {
            while (fgets(line, sizeof(line), in)) {
                int if_index, pfx_len, scope, flags, rc = 0;
                char if_name[17];
                rc = sscanf(line, "%39s %08x %08x %04x %02x %16s\n",
                    addr, &if_index, &pfx_len, &scope, &flags, if_name);
                if ((rc = 6) && (nic->cpqNicIfPhysAdapterIndex == if_index)) {
                    sprintf(cpqNicIfLogMapIPV6Address, 
                            "%.4s:%.4s:%.4s:%.4s:%.4s:%.4s:%.4s:%.4s",
                            &addr[0],  &addr[4],  &addr[8],  &addr[12],
                            &addr[16], &addr[20], &addr[24], &addr[28]);

                }
            }
            fclose(in);
        }
    }
    else {
        snmp_log(LOG_ERR, "ERROR: send_cpqnic_trap(): physNicIndex is < 0!!!\n");
        return(1);
    }

    cpqNicIfPhysAdapterSlotOid[
        OID_LENGTH(cpqNicIfPhysAdapterSlotOid)-1] = nic->cpqNicIfPhysAdapterIndex;
    cpqNicIfPhysAdapterPortOid[
        OID_LENGTH(cpqNicIfPhysAdapterPortOid)-1] = nic->cpqNicIfPhysAdapterIndex;
    cpqNicIfPhysAdapterStatusOid[
        OID_LENGTH(cpqNicIfPhysAdapterStatusOid)-1] = nic->cpqNicIfPhysAdapterIndex;
    cpqSePciSlotBoardNameOid[OID_LENGTH(cpqSePciSlotBoardNameOid)-2] = 
                                                                nic->PCI_Bus;
    cpqSePciSlotBoardNameOid[OID_LENGTH(cpqSePciSlotBoardNameOid)-1] = 
                                                                nic->PCI_Slot;
    cpqNicIfPhysAdapterPartNumberOid[
        OID_LENGTH(cpqNicIfPhysAdapterPartNumberOid)-1] = 
                                                nic->cpqNicIfPhysAdapterIndex;

    /* skip over "." (dots) in the ip address */
    /* TODO: this is IPv4 specific...         */
    DEBUGMSGTL(("cpqnic:arch",
                            "ipAdEntAddr %s\n",
                            ipAdEntAddr));
    DEBUGMSGTL(("cpqnic:arch",
                            "cpqNicIfLogMapIPV6Address %s\n",
                            cpqNicIfLogMapIPV6Address));

    ipDigit = ipAdEntAddr;
    ipAddr[0] = ipAdEntAddrOid[OID_LENGTH(ipAdEntAddrOid)-4] = atoi(ipDigit);

    ipDigit = strchr(ipDigit, '.');
    if (ipDigit != (char*)0)
        ipDigit++;
    ipAddr[1] = ipAdEntAddrOid[OID_LENGTH(ipAdEntAddrOid)-3] = atoi(ipDigit);

    ipDigit = strchr(ipDigit, '.');
    if (ipDigit != (char*)0)
        ipDigit++;
    ipAddr[2] = ipAdEntAddrOid[OID_LENGTH(ipAdEntAddrOid)-2] = atoi(ipDigit);

    ipDigit = strchr(ipDigit, '.');
    if (ipDigit != (char*)0)
        ipDigit++;
    ipAddr[3] = ipAdEntAddrOid[OID_LENGTH(ipAdEntAddrOid)-1] = atoi(ipDigit);

    cpqNicIfLogMapIPV6AddressOid[OID_LENGTH(cpqNicIfLogMapIPV6AddressOid)-1] = 
                                                  nic->cpqNicIfPhysAdapterIndex;

    uname(&sys_name);   /* get sysName */

    snmp_varlist_add_variable(&var_list, sysNameOid,
            sizeof(sysNameOid) / sizeof(oid),
            ASN_OCTET_STR,
            (u_char *) sys_name.nodename,
            strlen(sys_name.nodename));

    snmp_varlist_add_variable(&var_list, cpqHoTrapFlagsOid,
            sizeof(cpqHoTrapFlagsOid) / sizeof(oid),
            ASN_INTEGER, &intVal,
            sizeof(ASN_INTEGER));

    snmp_varlist_add_variable(&var_list, cpqNicIfPhysAdapterSlotOid,
            sizeof(cpqNicIfPhysAdapterSlotOid) / sizeof(oid),
            ASN_INTEGER, (u_char *) &nic->cpqNicIfPhysAdapterSlot,
            sizeof(ASN_INTEGER));

    snmp_varlist_add_variable(&var_list, cpqNicIfPhysAdapterPortOid,
            sizeof(cpqNicIfPhysAdapterPortOid) / sizeof(oid),
            ASN_INTEGER, (u_char *) &nic->cpqNicIfPhysAdapterPort,
            sizeof(ASN_INTEGER));

    snmp_varlist_add_variable(&var_list, cpqSiServerSystemIdOid,
            sizeof(cpqSiServerSystemIdOid) / sizeof(oid),
            ASN_OCTET_STR,
            (u_char *) cpqSiServerSystemId,
            strlen(cpqSiServerSystemId));

    snmp_varlist_add_variable(&var_list, cpqNicIfPhysAdapterStatusOid,
            sizeof(cpqNicIfPhysAdapterStatusOid) / sizeof(oid),
            ASN_INTEGER, (u_char *) &nic->cpqNicIfPhysAdapterStatus,
            sizeof(ASN_INTEGER));

    snmp_varlist_add_variable(&var_list, cpqSePciSlotBoardNameOid,
            sizeof(cpqSePciSlotBoardNameOid) / sizeof(oid),
            ASN_OCTET_STR,
            (u_char *) nic->cpqNicIfPhysAdapterName,
            strlen(nic->cpqNicIfPhysAdapterName));

    snmp_varlist_add_variable(&var_list, cpqNicIfPhysAdapterPartNumberOid,
            sizeof(cpqNicIfPhysAdapterPartNumberOid) / sizeof(oid),
            ASN_OCTET_STR,
            (u_char *) nic->cpqNicIfPhysAdapterPartNumber,
            strlen(nic->cpqNicIfPhysAdapterPartNumber));

    snmp_varlist_add_variable(&var_list, ipAdEntAddrOid,
            sizeof(ipAdEntAddrOid) / sizeof(oid),
            ASN_IPADDRESS,
            (u_char *) ipAddr,
            sizeof(ASN_IPADDRESS));

    snmp_varlist_add_variable(&var_list, cpqNicIfLogMapIPV6AddressOid,
            sizeof(cpqNicIfLogMapIPV6AddressOid) / sizeof(oid),
            ASN_OCTET_STR,
            (u_char *) cpqNicIfLogMapIPV6Address,
            strlen(cpqNicIfLogMapIPV6Address));


    DEBUGMSGTL(("cpqNic","calling netsnmp_send_trap...\n"));

    netsnmp_send_traps(SNMP_TRAP_ENTERPRISESPECIFIC,
            specific_type,
            compaq,
            compaq_len,
            var_list,
            NULL,
            0);

    DEBUGMSGTL(("cpqNic","returned from netsnmp_send_trap; call snmp_free_varbind()....\n"));

    snmp_free_varbind(var_list);

    DEBUGMSGTL(("cpqNic","done snmp_free_varbind().\n"));

#ifdef notdef
    /* this is not a team, so send link fault */
    switch (specific_type) {
        case (LINK_DOWN_TRAP_OID):
            /* non-teamed nic went down */
            iml_log_link_down( (unsigned char)nic_slot, (unsigned char)nic_port);
            break;

        case (LINK_UP_TRAP_OID):
            /* non-teamed nic came up */
            iml_log_link_up(
                    (unsigned char)nic_slot,
                    (unsigned char)nic_port);
            break;

        default:
            fprintf(stderr, "cmanic ERROR: bad trap type %#x, %s,%d\n",
                    specific_type, __FILE__, __LINE__);
            break;
    }
#endif
    return (0);
}


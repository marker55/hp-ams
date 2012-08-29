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
#include <sys/stat.h>
#include <net/if.h>
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
#include "cpqHoFwVerTable.h"

#include "hpHelper.h"
#include "cpqNic.h"
#include "cpqnic.h"
#include "nic_linux.h"
#include "nic_db.h"
#include "utils.h"

extern unsigned char cpqHoMibHealthStatusArray[];
extern oid       cpqHoFwVerTable_oid[];
extern size_t    cpqHoFwVerTable_oid_len;
extern int       FWidx;

#define SYSBUSPCISLOTS "/sys/bus/pci/slots/"

char *nic_names[] = {"eth",
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

    int     iMacLoop;
    oid     Index;
    char*   pchTemp;
    char    achTemp[TEMP_LEN];  
    netsnmp_container *fw_container = NULL;
    netsnmp_cache *fw_cache = NULL;
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
    char    linkBuf[1024];
    ssize_t link_sz;
    cpqNicIfPhysAdapterTable_entry* entry;
    cpqNicIfPhysAdapterTable_entry* old;

    unsigned short VendorID, DeviceID, SubsysVendorID, SubsysDeviceID;
    int rc;

    DEBUGMSGTL(("cpqnic:arch","netsnmp_arch_ifphys_container_load entry\n"));

    /*
     * find  the cpqHoFwVerTable  container.
     */

    fw_cache = netsnmp_cache_find_by_oid(cpqHoFwVerTable_oid,
                                            cpqHoFwVerTable_oid_len);
    if (fw_cache != NULL) {
        fw_container = fw_cache->magic;
    }
    DEBUGMSGTL(("cpqHoFwVerTable:init",
                "cpqHoFwVerTable cache = %p\n", fw_cache));

    DEBUGMSGTL(("cpqHoFwVerTable:init",
                "cpqHoFwVerTable container = %p\n", fw_container));


    if ((devcount = scandir("/sys/class/net/",
                    &devlist, file_select, alphasort)) > 0) {
        for (i = 0; i < devcount; i++) {
            snprintf(sysfsDir, 1024, "/sys/class/net/%s/device", 
                    devlist[i]->d_name);
            if ((link_sz = readlink(sysfsDir, linkBuf, 1024)) < 0) {
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

                sscanf(strrchr(&linkBuf[0],'/') + 1, "%x:%x:%x.%x",
                        &domain, &bus, &device, &function);

                /* init values to defaults in case they are not in procfs */
                entry->cpqNicIfPhysAdapterSlot = -1;
                entry->cpqNicIfPhysAdapterPort = -1;
                entry->cpqNicIfPhysAdapterVirtualPortNumber = -1;
                entry->cpqNicIfPhysAdapterIoBayNo = -1;
                entry->cpqNicIfPhysAdapterSpeedMbps = UNKNOWN_SPEED;
                entry->cpqNicIfPhysAdapterConfSpeedDuplex = 
                    CONF_SPEED_DUPLEX_OTHER;
                entry->AdapterAutoNegotiate = 0;
                entry->cpqNicIfPhysAdapterMemAddr = 0;
                entry->cpqNicIfPhysAdapterFWVersion[0] = '\0';
                strcpy(entry->cpqNicIfPhysAdapterPartNumber, "UNKNOWN");
                /* default "empty" per mib */
                strcpy(entry->cpqNicIfPhysAdapterName, "EMPTY"); 
                entry->cpqHoFwVerIndex = 0;
            }

            if (entry->cpqNicIfPhysAdapterPort != -1) {
                /* adapter port is one based....if we found the port (bus) in */
                /* the procfs bump it by one.                                 */
                entry->cpqNicIfPhysAdapterPort++;
            }

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

            sprintf(achTemp, "/sys/class/net/%s/device/vendor", 
                    devlist[i]->d_name);
            VendorID = get_sysfs_shex(achTemp); 

            sprintf(achTemp, "/sys/class/net/%s/device/device", 
                    devlist[i]->d_name);
            DeviceID = get_sysfs_shex(achTemp);

            sprintf(achTemp, "/sys/class/net/%s/device/subsystem_vendor", 
                    devlist[i]->d_name);
            SubsysVendorID = get_sysfs_shex(achTemp);

            sprintf(achTemp, "/sys/class/net/%s/device/subsystem_device", 
                    devlist[i]->d_name);
            SubsysDeviceID = get_sysfs_shex(achTemp);

            DEBUGMSGTL(("cpqNic","Vendor = %x, Device = %x, " 
                        "Subsys Vendor = %x, Subsys Device = %x\n",
                        VendorID, DeviceID,
                        SubsysVendorID, SubsysDeviceID));
            /* get model name and part number from database created struct */
            if ((pnic_hw_db = 
                        get_nic_hw_info(VendorID, DeviceID,
                            SubsysVendorID, SubsysDeviceID))
                    != NULL) {
                /* over-ride info read from procfs/hpetfe */
                strcpy(entry->cpqNicIfPhysAdapterName, pnic_hw_db->pname);
                entry->cpqNicIfPhysAdapterName_len =
                    strlen(entry->cpqNicIfPhysAdapterName);
                strcpy(entry->cpqNicIfPhysAdapterPartNumber, 
                        pnic_hw_db->ppca_part_number);
                entry->cpqNicIfPhysAdapterPartNumber_len = 
                    strlen(entry->cpqNicIfPhysAdapterPartNumber);
            }

            sprintf(achTemp, "/sys/class/net/%s/device/irq",
                    devlist[i]->d_name);
            entry->cpqNicIfPhysAdapterIrq = get_sysfs_int(achTemp);

            if (get_ethtool_info(devlist[i]->d_name, &einfo) == 0) {
                DEBUGMSGTL(("cpqNic", "Got ethtool info for %s\n", 
                            devlist[i]->d_name));  
                entry->AdapterAutoNegotiate =  einfo.autoneg; 
                if (einfo.duplex == DUPLEX_FULL)  
                    entry->cpqNicIfPhysAdapterDuplexState =  FULL_DUPLEX; 
                else
                    entry->cpqNicIfPhysAdapterDuplexState =  HALF_DUPLEX; 

                /*
                 * Mellanox Card NC542m has bus, device and function 
                 * same for both  the ports.  
                 */

                if ((VendorID == 0x15B3) && (DeviceID == 0x5A44) && 
                    (SubsysVendorID == 0x3107) && (SubsysDeviceID == 0x103C)) {
                    mlxport++;
                }		
                entry->cpqNicIfPhysAdapterIoBayNo = UNKNOWN_SLOT;
                entry->cpqNicIfPhysAdapterPort = UNKNOWN_PORT;
                entry->cpqNicIfPhysAdapterVirtualPortNumber = UNKNOWN_PORT;

                entry->cpqNicIfPhysAdapterSlot = get_pcislot(einfo.bus_info);
                DEBUGMSGTL(("cpqNic", "Got pcislot info for %s = %ld\n", 
                            devlist[i]->d_name,
                            entry->cpqNicIfPhysAdapterSlot));  

                if ( einfo.firmware_version ) {
                    int idx = 0;
                    cpqHoFwVerTable_entry *fw_entry;

                    strcpy(entry->cpqNicIfPhysAdapterFWVersion, 
                            einfo.firmware_version);
                    entry->cpqNicIfPhysAdapterFWVersion_len = 
                        strlen(entry->cpqNicIfPhysAdapterFWVersion);
                    if (FWidx != -1) 
                        idx = unregister_int_index(cpqHoFwVerTable_oid, 
                                                   cpqHoFwVerTable_oid_len,
                                                   FWidx);

                    DEBUGMSGTL(("cpqHoFwVerTable:init",
                                "cpqHoFwVerTable Unregister NIC idx %d = %d\n",
                                FWidx, idx));
                    idx = register_int_index(cpqHoFwVerTable_oid, 
                                             cpqHoFwVerTable_oid_len,
                                             FWidx);
                    DEBUGMSGTL(("cpqHoFwVerTable:init",
                                "cpqHoFwVerTable Register NIC idx %d = %d\n", 
                                FWidx, idx));
                    if (idx != -1)
                        FWidx = idx;
                    DEBUGMSGTL(("cpqHoFwVerTable:init",
                        "cpqHoFwVerTable NIC FWidx = %d before\n", FWidx));
		    if (fw_container != NULL) {
			fw_entry = 
				cpqHoFwVerTable_createEntry(fw_container, (oid)FWidx++);
			if (fw_entry) {
				DEBUGMSGTL(("cpqHoFwVerTable:init",
				"cpqHoFwVerTable entry = %p\n", fw_entry));
	
				DEBUGMSGTL(("cpqHoFwVerTable:init",
					"cpqHoFwVerTable FWidx = %d after\n", 
					FWidx));
				fw_entry->cpqHoFwVerCategory = 3;
				fw_entry->cpqHoFwVerDeviceType = 24;
	
				strcpy(fw_entry->cpqHoFwVerVersion, 
				einfo.firmware_version);
				fw_entry->cpqHoFwVerVersion_len = 
				strlen(fw_entry->cpqHoFwVerVersion);
	
	
				strcpy(fw_entry->cpqHoFwVerDisplayName,
					entry->cpqNicIfPhysAdapterName);
				fw_entry->cpqHoFwVerDisplayName_len = 
					entry->cpqNicIfPhysAdapterName_len;
				CONTAINER_INSERT(fw_container, fw_entry);
			}
		    }
                }

                if (entry->cpqNicIfPhysAdapterSpeedMbps == UNKNOWN_SPEED) {
                    entry->cpqNicIfPhysAdapterSpeedMbps = einfo.link_speed;
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
                DEBUGMSGTL(("cpqNic", 
                            "Finished processing  ethtool info for %s\n", 
                            devlist[i]->d_name));
            } 

            GetConfSpeedDuplex(entry);
            sprintf(achTemp, "/sys/class/net/%s/operstate", devlist[i]->d_name);
            pchTemp = get_sysfs_str(achTemp);
            if (pchTemp != NULL) {
                if (strcmp(pchTemp, "up") == 0) {
                    entry->cpqNicIfPhysAdapterCondition = ADAPTER_CONDITION_OK;
                    entry->cpqNicIfPhysAdapterStatus = STATUS_OK;
                } else if (strcmp(pchTemp, "down") == 0) {
                    cpqHoMibHealthStatusArray[CPQMIBHEALTHINDEX] =
                                                MIB_CONDITION_FAILED;
                    cpqHostMibStatusArray[CPQMIB].cond = 
                                                MIB_CONDITION_FAILED;
                    entry->cpqNicIfPhysAdapterCondition = 
                        ADAPTER_CONDITION_FAILED;
                    entry->cpqNicIfPhysAdapterStatus = STATUS_LINK_FAILURE;
                } else {
                    if (cpqHoMibHealthStatusArray[CPQMIBHEALTHINDEX] !=
                                                MIB_CONDITION_FAILED ) {
                        cpqHoMibHealthStatusArray[CPQMIBHEALTHINDEX] =
                                                MIB_CONDITION_DEGRADED;  
                        cpqHostMibStatusArray[CPQMIB].cond = 
                                                MIB_CONDITION_DEGRADED;  
                    }
                    entry->cpqNicIfPhysAdapterCondition =
                        ADAPTER_CONDITION_OTHER;
                    entry->cpqNicIfPhysAdapterStatus = STATUS_UNKNOWN;
                }
                free(pchTemp);
            }

            if (entry->cpqNicIfPhysAdapterStatus == STATUS_OK)
                entry->cpqNicIfPhysAdapterState = ADAPTER_STATE_ACTIVE;
            else if (entry->cpqNicIfPhysAdapterStatus == STATUS_LINK_FAILURE)
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

            sprintf(achTemp, "/sys/class/net/%s/statistics/rx_bytes",
                    devlist[i]->d_name);
            entry->cpqNicIfPhysAdapterInOctets = get_sysfs_int(achTemp);
            sprintf(achTemp, "/sys/class/net/%s/statistics/tx_bytes",
                    devlist[i]->d_name);
            entry->cpqNicIfPhysAdapterOutOctets = get_sysfs_int(achTemp);
            DEBUGMSGTL(("cpqNic", "rx_bytes= %ld tx_bytes = %ld\n", 
                        entry->cpqNicIfPhysAdapterInOctets,
                        entry->cpqNicIfPhysAdapterOutOctets));

            sprintf(achTemp, "/sys/class/net/%s/statistics/rx_packets",
                    devlist[i]->d_name);
            entry->cpqNicIfPhysAdapterGoodReceives = get_sysfs_int(achTemp);
            sprintf(achTemp, "/sys/class/net/%s/statistics/tx_packets",
                    devlist[i]->d_name);
            entry->cpqNicIfPhysAdapterGoodTransmits = get_sysfs_int(achTemp);
            DEBUGMSGTL(("cpqNic", "rx_packets= %ld tx_packets = %ld\n", 
                        entry->cpqNicIfPhysAdapterGoodReceives,
                        entry->cpqNicIfPhysAdapterGoodTransmits));
            if (old == NULL) {
                rc = CONTAINER_INSERT(container, entry);
                DEBUGMSGTL(("cpqnic:arch","Insert entry %s\n",
                            devlist[i]->d_name));
                j++;
            }
            free(devlist[i]);
        }
        numIfPhys = CONTAINER_SIZE(container);
        DEBUGMSGTL(("cpqnic:arch"," loaded %d entries\n", numIfPhys));
        free(devlist);
    }
    return(0);
}


int netsnmp_arch_iflog_container_load(netsnmp_container *container)
{

    int      i, j, iCount, dCount;
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
    netsnmp_index tmp;
    oid oid_index[2];

    cpqNicIfLogMapTable_entry *entry;
    cpqNicIfLogMapTable_entry *old = NULL;

    dynamic_interface_info dyn_if_info;

    DEBUGMSGTL(("cpqnic:arch","netsnmp_arch_iflog_container_load entry\n"));

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
                entry->cpqNicIfLogMapCondition = ADAPTER_CONDITION_OTHER;
                entry->cpqNicIfLogMapStatus = MAP_STATUS_UNKNOWN;
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
            sprintf(chTemp, "/sys/class/net/%s/operstate", filelist[i]->d_name);
            pchTemp = get_sysfs_str(chTemp);
            if (pchTemp != NULL) {
                if (strcmp(pchTemp, "up") == 0) {
                    entry->cpqNicIfLogMapCondition = ADAPTER_CONDITION_OK;
                    entry->cpqNicIfLogMapStatus = STATUS_OK;
                } else if (strcmp(pchTemp, "down") == 0) {
                    entry->cpqNicIfLogMapCondition = ADAPTER_CONDITION_FAILED;
                    entry->cpqNicIfLogMapStatus = STATUS_LINK_FAILURE;
                } else {
                    entry->cpqNicIfLogMapCondition = ADAPTER_CONDITION_OTHER;
                    entry->cpqNicIfLogMapStatus = STATUS_UNKNOWN;
                }
                free(pchTemp);
            }

            bondtype = -1;
            entry->cpqNicIfLogMapDescription[0] = (char)0;
            entry->cpqNicIfLogMapDescription_len = 0; 

            entry->cpqNicIfLogMapLACNumber_len = strlen(filelist[i]->d_name);
            strncpy(entry->cpqNicIfLogMapLACNumber, filelist[i]->d_name,
                            entry->cpqNicIfLogMapLACNumber_len);
            switch (ifType) {
                case LOOP:
                case SIT:
                    entry->cpqNicIfLogMapSpeedMbps = 0;
                    break;
                case NIC:
                case VNIC:
                    phys_index = name2indx(filelist[i]->d_name);
                    entry->cpqNicIfLogMapPhysicalAdapters[0] = 
                            (char)phys_index;
                    entry->cpqNicIfLogMapPhysicalAdapters[1] =0;
                    entry->cpqNicIfLogMapPhysicalAdapters_len = 1;
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
                            sprintf(chTemp, "/sys/class/net/%s/operstate", 
                                    devlist[j]->d_name);
                            pchTemp = get_sysfs_str(chTemp);
                            if (pchTemp != NULL) {
                                if (strcmp(pchTemp, "up") == 0) {
                                    entry->cpqNicIfLogMapCondition = 
                                                        ADAPTER_CONDITION_OK;
                                    entry->cpqNicIfLogMapStatus = STATUS_OK;
                                } else if (strcmp(pchTemp, "down") == 0) {
                                    entry->cpqNicIfLogMapCondition = 
                                                    ADAPTER_CONDITION_FAILED;
                                    entry->cpqNicIfLogMapStatus = 
                                                    STATUS_LINK_FAILURE;
                                } else {
                                    entry->cpqNicIfLogMapCondition = 
                                                    ADAPTER_CONDITION_OTHER;
                                    entry->cpqNicIfLogMapStatus = 
                                                            STATUS_UNKNOWN;
                                }
                                free(pchTemp);
                            }
                            free(devlist[j]);
                        }
                        entry->cpqNicIfLogMapAdapterCount = map;
                        free(devlist);
                    }
                    /* logical interface name is used for bonding/teaming */
                    if (entry->cpqNicIfLogMapCondition == ADAPTER_CONDITION_OK)
                        entry->cpqNicIfLogMapStatus = MAP_STATUS_OK;
                    else if (entry->cpqNicIfLogMapCondition == 
                                                    ADAPTER_CONDITION_FAILED)
                        entry->cpqNicIfLogMapStatus = MAP_STATUS_GROUP_FAILED;
                    else
                        entry->cpqNicIfLogMapStatus = MAP_STATUS_UNKNOWN;

                    entry->cpqNicIfLogMapSpeedMbps = UNKNOWN_SPEED;
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
                } else 
                    strcpy(entry->cpqNicIfLogMapIPV6Address, "N/A");

                free_dynamic_interface_info(&dyn_if_info);
            }


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
    return(0);
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

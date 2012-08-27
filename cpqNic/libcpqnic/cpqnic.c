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

#include "common_utilities.h"

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

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <net-snmp/library/system.h>
#include <net-snmp/output_api.h>

#include "cpqnic.h"
#include "utils.h"
#include "cpqNic.h"

extern unsigned char cpqHoMibHealthStatusArray[];

static const char *hplog_command = (char*)0;
static const char *hplog_sbin  = "/sbin/hplog";

static time_t  iTimeToRecache = 0;

static cpqNic_t NicMibVars;
typedef struct _hplog_t {
    int  iBusNum;
    int  iDeviceNum;
    int  iFunctionNum;
    int  iSlotNumber;
    struct _hplog_t* next;
} hplog_t;

hplog_t * hplog_entry;
hplog_t * hplog_start;
/* NicMibVars                    */

static int      iMinPollFrequency = 20;  /* if caller does not set min */


#define SYS_CLASS_NET "/sys/class/net/"
char *nic_names[] = {"eth",
    "peth",
    "vmnic",
    (char *) 0};
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

#ifndef NIC_DEBUG
#ifdef UNIT_TEST_GET_STATS
#define NIC_DEBUG 1
#else /* UNIT_TEST_GET_STATS */
#define NIC_DEBUG 0
#endif /* UNIT_TEST_GET_STATS */
#endif

#define UNKNOWN_BOND        0
#define ACTIVE_BACKUP_BOND  1
#define ROUND_ROBIN_BOND    2
#define XOR_BOND            3
#define BROADCAST_BOND      4
#define IEEE_8023AD_BOND    5
#define TLB_BOND            6
#define ALB_BOND            7

/* below array corresponds to above _BOND defines*/
char *(apchMapGroupType[]) = 
{
    "unknown team type",
    "Network Fault Tolerance (active backup)",
    "Switch-assisted Load Balancing (round-robin)",
    "Switch-assisted Load Balancing (xor)",
    "Switch-assisted Load Balancing (Broadcast)",
    "IEEE 802.3ad Link Aggregation",
    "Transmit Load Balancing",
    "Adaptive Load Balancing"
};

#define MII_STATUS_UP       1
#define MII_STATUS_DOWN     2

#define MAX_BOND_IFACE_LEN  32  /* max interface name length, i.e. eth0 */
#define MAX_BOND_DIR_LEN   128  /* max directory name len to bond dir   */
/* i.e. "/proc/net/bond0"               */

char* pch_nl;
#define CLOBBER_NL(pstr)                        \
{                                               \
    pch_nl = strchr(pstr,'\n');                 \
    if (pch_nl != (char*)0)                     \
    *pch_nl = '\0';                         \
}


typedef struct tBondDevice 
{
    char achInterfaceName[MAX_BOND_IFACE_LEN];
    int  iMIIStatus;
    int  iLinkFailureCount;
    int  iSNMPIndex; /* snmp index for this bond member interface */
    struct tBondDevice* pstrNext;
} BondDevice;

/* struct below holds contents of /proc/net/bondX/info files */
typedef struct tBondInterface 
{
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

typedef struct tVlanRecord 
{
    char achDeviceName[MAX_INTERFACE_NAME];    /* i.e. eth0.4092  */
    char achId[MAX_VLAN_ID_LEN];               /* i.e. 4092       */
    char achInterfaceName[MAX_INTERFACE_NAME]; /* i.e. eth0       */
    int  iSNMPIndex; /* snmp index for interface this vlan is on */
    int  iMapping; /* assigned when VlanMapTable is filled in */
    struct tVlanRecord* pstrNext;
} VlanRecord;

typedef struct tNicTag 
{
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

/* added for tracking ifLastChange; rQm 235021 */
/* NOTE: reader/writer must hold thread lock   */
typedef struct tIfLastChange 
{  
    char *pchInterfaceName;               /* eth0, bond0, etc.               */
    unsigned long ulLastChanged;          /* sysUpTime at last status change */
    struct tIfLastChange *pstrNext; 
} IfLastChange;

IfLastChange *pstrIfLastChange = (IfLastChange*)0;
/* used as initial value of pchInterfaceName above so we don't have */
/* to check for null ptr before strcmp                              */
char *pchNoInterface = "-";
#define NUM_IF_LAST_CHANGE_ENTRIES  10

static const cpqNicIfPhysical_t strPhysIf;  /* defined here for struct below */

/* one of these structs exist for each driver we detect in the procfs */
typedef struct tNicPort 
{
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

#define     LINES_TO_ALLOC   10    /* number of lines to alloc at a time */
/* for reading procfs...              */

static int iLoadPhysStruct(int, NicPort *, cpqNicIfPhysical_t *);
int iSetIfLastChange(char *, unsigned long);

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

static int iLibNotInitialized = 1;
static int iAllowSnmpSets = 1;

pid_t main_pid;                         /* required by obj.c                 */
char *sysUpTimeOid="1.3.6.1.2.1.1.3.0"; /* used to populate ifLastChange     */

/******************************************************************************
 *   Description: Dump the contents of the physical interface             
 *   Parameters : 1) physical interface                                   
 *   Returns    : void                                                    
 ******************************************************************************/
static void vPrintNicPhysInfo(cpqNicIfPhysical_t *pstrIfPhys)
{
    int iLoop;

    fprintf(stderr, "             VendorID : %#x\n", pstrIfPhys->VendorID);
    fprintf(stderr, "             DeviceID : %#x\n", pstrIfPhys->DeviceID);
    fprintf(stderr, "       SubsysVendorID : %#x\n", pstrIfPhys->SubsysVendorID);
    fprintf(stderr, "       SubsysDeviceID : %#x\n", pstrIfPhys->SubsysDeviceID);
    fprintf(stderr, "        InterfaceName : %s\n", pstrIfPhys->InterfaceName);

    fprintf(stderr, "         AdapterIndex : %d\n", pstrIfPhys->AdapterIndex);

    fprintf(stderr, "      AdapterIfNumber : ");
    for (iLoop=0;
            ((pstrIfPhys->AdapterIfNumber[iLoop] != 0) ||
             (pstrIfPhys->AdapterIfNumber[1+iLoop] != 0) ||
             (pstrIfPhys->AdapterIfNumber[2+iLoop] != 0) ||
             (pstrIfPhys->AdapterIfNumber[3+iLoop] != 0));
            iLoop+=4) {
        fprintf(stderr, "%02X%02X%02X%02X  ",
                pstrIfPhys->AdapterIfNumber[iLoop],
                pstrIfPhys->AdapterIfNumber[1+iLoop],
                pstrIfPhys->AdapterIfNumber[2+iLoop],
                pstrIfPhys->AdapterIfNumber[3+iLoop]);
    }
    fprintf(stderr, "\n");

    fprintf(stderr, "          AdapterRole : %d\n",
            pstrIfPhys->AdapterRole);
    fprintf(stderr, "    AdapterMACAddress : %02X:%02X:%02X:%02X:%02X:%02X\n",
            pstrIfPhys->AdapterMACAddress[0]&0xff,
            pstrIfPhys->AdapterMACAddress[1]&0xff,
            pstrIfPhys->AdapterMACAddress[2]&0xff,
            pstrIfPhys->AdapterMACAddress[3]&0xff,
            pstrIfPhys->AdapterMACAddress[4]&0xff,
            pstrIfPhys->AdapterMACAddress[5]&0xff);

    fprintf(stderr, "   SoftwareMACAddress : %02X:%02X:%02X:%02X:%02X:%02X\n",
            pstrIfPhys->SoftwareMACAddress[0]&0xff,
            pstrIfPhys->SoftwareMACAddress[1]&0xff,
            pstrIfPhys->SoftwareMACAddress[2]&0xff,
            pstrIfPhys->SoftwareMACAddress[3]&0xff,
            pstrIfPhys->SoftwareMACAddress[4]&0xff,
            pstrIfPhys->SoftwareMACAddress[5]&0xff);



    fprintf(stderr, "          AdapterSlot : %d\n",
            pstrIfPhys->AdapterSlot);
    fprintf(stderr, "        AdapterIoAddr : %#x\n",
            pstrIfPhys->AdapterIoAddr);
    fprintf(stderr, "           AdapterIrq : %d\n",
            pstrIfPhys->AdapterIrq);
    fprintf(stderr, "           AdapterDma : %#x\n",
            pstrIfPhys->AdapterDma);
    fprintf(stderr, "       AdapterMemAddr : %#x\n",
            pstrIfPhys->AdapterMemAddr);
    fprintf(stderr, "          AdapterPort : %d\n",
            pstrIfPhys->AdapterPort);
    fprintf(stderr, "        AdapterVirtualPort : %d\n",
            pstrIfPhys->AdapterVirtualPort);
    fprintf(stderr, "   AdapterDuplexState : %d\n",
                pstrIfPhys->AdapterDuplexState);
    fprintf(stderr, "     AdapterCondition : %d\n",
            pstrIfPhys->AdapterCondition);
    fprintf(stderr, "         AdapterState : %d\n",
            pstrIfPhys->AdapterState);
    fprintf(stderr, "        AdapterStatus : %d\n",
            pstrIfPhys->AdapterStatus);
    fprintf(stderr, "    AdapterStatsValid : %d\n",
            pstrIfPhys->AdapterStatsValid);
    fprintf(stderr, " AdapterGoodTransmits : %d\n",
            pstrIfPhys->AdapterGoodTransmits);
    fprintf(stderr, "  AdapterGoodReceives : %d\n",
            pstrIfPhys->AdapterGoodReceives);
    fprintf(stderr, "  AdapterBadTransmits : %d\n",
            pstrIfPhys->AdapterBadTransmits);
    fprintf(stderr, "   AdapterBadReceives : %d\n",
            pstrIfPhys->AdapterBadReceives);
    fprintf(stderr, " AdapterAlignmentErrors : %d\n",
            pstrIfPhys->AdapterAlignmentErrors);
    fprintf(stderr, "       AdapterFCSErrors : %d\n",
            pstrIfPhys->AdapterFCSErrors);
    fprintf(stderr, "  AdapterSingleCollisionFrames : %d\n",
            pstrIfPhys->AdapterSingleCollisionFrames);
    fprintf(stderr, "AdapterMultipleCollisionFrames : %d\n",
            pstrIfPhys->AdapterMultipleCollisionFrames);
    fprintf(stderr, "  AdapterDeferredTransmissions : %d\n",
            pstrIfPhys->AdapterDeferredTransmissions);
    fprintf(stderr, "         AdapterLateCollisions : %d\n",
            pstrIfPhys->AdapterLateCollisions);
    fprintf(stderr, "    AdapterExcessiveCollisions : %d\n",
            pstrIfPhys->AdapterExcessiveCollisions);
    fprintf(stderr, "AdapterInternalMacTransmitErrors : %d\n",
            pstrIfPhys->AdapterInternalMacTransmitErrors);
    fprintf(stderr, "       AdapterCarrierSenseErrors : %d\n",
            pstrIfPhys->AdapterCarrierSenseErrors);
    fprintf(stderr, "            AdapterFrameTooLongs : %d\n",
            pstrIfPhys->AdapterFrameTooLongs);
    fprintf(stderr, " AdapterInternalMacReceiveErrors : %d\n",
            pstrIfPhys->AdapterInternalMacReceiveErrors);
    fprintf(stderr, " AdapterHwLocation : %s\n",
            pstrIfPhys->AdapterHwLocation);
    fprintf(stderr, " AdapterPartNumber : %s\n",
            pstrIfPhys->AdapterPartNumber);

    return;
} 


/******************************************************************************
 *   Description: Prints every interface we were able to decode/parse     
 *   Parameters : 1) linked list of nic port structures                   
 *   Returns    : void                                                    
 ******************************************************************************/
#if NIC_DEBUG 
static void vPrintNicPortStructs(NicPort *pNicPorts)
{
    int iPhysIndex = 0;
    cpqNicIfPhysical_t strIfPhys;
    int do_remap = 1;

    while (pNicPorts != (NicPort *)0) {
        fprintf(stderr, "===================================================\n");
        fprintf(stderr, "Interface number: %d\n",
                pNicPorts->iInterfaceNumber);
        fprintf(stderr, "Procfs file: %s\n",
                pNicPorts->pchNicProcFile);

        memset(&strIfPhys, 0, sizeof(cpqNicIfPhysical_t));
        if (iLoadPhysStruct(do_remap, pNicPorts, &strIfPhys)) {
            fprintf(stderr,
                    "--could not load physical interface information\n");
            pNicPorts = pNicPorts->next;
            continue;
        }
        /* fill in AdpaterIndex here to fix rQm 211783     */
        strIfPhys.AdapterIndex = iPhysIndex + 1;
        iPhysIndex++;


        vPrintNicPhysInfo(&strIfPhys);

        pNicPorts = pNicPorts->next;
    }
    return;
}
#endif /* NIC_DEBUG  */

/******************************************************************************
 *   Description: Print the complete cpqNic structure that is returned by 
 *                this library                                            
 *   Parameters : 1) the compaq nic structure (cpqNic)                    
 *   Returns    : void                                                    
 ******************************************************************************/
void vPrintCqpNic(cpqNic_t *pstrCpqNic)
{

    int iLoop;
    int iLoop2;
    cpqNicIfLogical_t  *pIfLog;
    cpqNicIfVlan_t     *pVlan;

    fprintf(stderr,
            "################################################################\n");

    if (pstrCpqNic->NumPhysNics == 0) {
        fprintf(stderr, 
                "No NICs detected that support procfs reporting\n");
        return;
    }

    /* we have at least one nic that recognizes our ioctls */

    fprintf(stderr, "         MibRevMajor : %d\n",
            pstrCpqNic->MibRevMajor);
    fprintf(stderr, "         MibRevMinor : %d\n",
            pstrCpqNic->MibRevMinor);
    fprintf(stderr, "        MibCondition : %d\n",
            pstrCpqNic->MibCondition);
    fprintf(stderr, "    OsCommonPollFreq : %d\n",
            pstrCpqNic->OsCommonPollFreq);
    fprintf(stderr, " MapOverallCondition : %d\n",
            pstrCpqNic->MapOverallCondition);

    for (iLoop=0; iLoop<pstrCpqNic->NumLogNics; iLoop++) {
        fprintf(stderr,
                "=============================================================\n");
        pIfLog = &pstrCpqNic->pIfLog[iLoop];
        fprintf(stderr, "      Interface Name : %s\n",
                pIfLog->InterfaceName);
        fprintf(stderr, "            MapIndex : %d\n",
                pIfLog->MapIndex);

        fprintf(stderr, "         MapIfNumber : ");
        for (iLoop2=0; 
                ((pIfLog->MapIfNumber[iLoop2] != 0) ||
                 (pIfLog->MapIfNumber[1+iLoop2] != 0) ||
                 (pIfLog->MapIfNumber[2+iLoop2] != 0) ||
                 (pIfLog->MapIfNumber[3+iLoop2] != 0));
                iLoop2+=4)
        {
            fprintf(stderr, "%02X%02X%02X%02X  ",
                    pIfLog->MapIfNumber[iLoop2],
                    pIfLog->MapIfNumber[1+iLoop2],
                    pIfLog->MapIfNumber[2+iLoop2],
                    pIfLog->MapIfNumber[3+iLoop2]);
        }
        fprintf(stderr, "\n");


        fprintf(stderr, "      MapDescription : %s\n",
                pIfLog->MapDescription);
        fprintf(stderr, "        MapGroupType : %d\n",
                pIfLog->MapGroupType);
        fprintf(stderr, "     MapAdapterCount : %d\n",
                pIfLog->MapAdapterCount);
        fprintf(stderr, "   MapAdapterOKCount : %d\n",
                pIfLog->MapAdapterOKCount);

        fprintf(stderr, "      MapIPV4Address : %s\n",
                pIfLog->MapIPV4Address);
        fprintf(stderr, "      MapIPV6Address : %s\n",
                pIfLog->MapIPV6Address);

        fprintf(stderr, " MapPhysicalAdapters : ");
        for (iLoop2=0; iLoop2<pIfLog->MapAdapterCount; iLoop2++) {
            fprintf(stderr, "%02X ",
                    pIfLog->MapPhysicalAdapters[iLoop2]);
        }
        fprintf(stderr, "\n");

        fprintf(stderr, "       MapMACAddress : %02X:%02X:%02X:%02X:%02X:%02X\n",
                pIfLog->MapMACAddress[0]&0xff,
                pIfLog->MapMACAddress[1]&0xff,
                pIfLog->MapMACAddress[2]&0xff,
                pIfLog->MapMACAddress[3]&0xff,
                pIfLog->MapMACAddress[4]&0xff,
                pIfLog->MapMACAddress[5]&0xff);
        fprintf(stderr, "   MapSwitchoverMode : %d\n",
                pIfLog->MapSwitchoverMode);
        fprintf(stderr, "        MapCondition : %d\n",
                pIfLog->MapCondition);
        fprintf(stderr, "           MapStatus : %d\n",
                pIfLog->MapStatus);
        fprintf(stderr, "   MapNumSwitchovers : %d\n",
                pIfLog->MapNumSwitchovers);
        fprintf(stderr, "   MapHwLocation : %s\n",
                pIfLog->MapHwLocation);
        fprintf(stderr, "   MapSpeedMbps : %d\n",
                pIfLog->MapSpeedMbps);
        fprintf(stderr, "  VlanCount : %d\n",
                pIfLog->VlanCount);

        if (pIfLog->VlanCount) {
            fprintf(stderr, "   MapVlans : ");

            for (iLoop2=0; iLoop2<pIfLog->VlanCount; iLoop2++) {
                fprintf(stderr, "%02X ",
                        pIfLog->MapVlans[iLoop2]);
            }
            fprintf(stderr, "\n");
        }
    }

    for (iLoop=0; iLoop<pstrCpqNic->NumPhysNics; iLoop++) {
        fprintf(stderr, 
                "-------------------------------------------------------------\n");
        vPrintNicPhysInfo(&pstrCpqNic->pIfPhys[iLoop]);
    }

    if (pstrCpqNic->NumVlans) {
        fprintf(stderr, 
                "-----------------------VLAN-------------------------------\n");
        for (iLoop=0; iLoop<pstrCpqNic->NumVlans; iLoop++) {
            pVlan = &pstrCpqNic->pIfVlan[iLoop];
            fprintf(stderr, "============================\n");

            fprintf(stderr, "MapIndex    : %d\n",
                    pVlan->MapIndex);
            fprintf(stderr, "MapLogIndex : %d\n",
                    pVlan->MapLogIndex);
            fprintf(stderr, "MapIfIndex  : %d\n",
                    pVlan->MapIfIndex);
            fprintf(stderr, "MapVlanId   : %d\n",
                    pVlan->MapVlanId);
            fprintf(stderr, "Name        : %s\n",
                    pVlan->Name);
            fprintf(stderr, "IPV6Address : %s\n",
                    pVlan->IPV6Address);
        }
        fprintf(stderr, "\n");
    }

    return;
} 


/******************************************************************************
 *   Description: Get the MIB-II interface index from the interface name. 
 *                The index is associated with the index of               * 
 *                interfaces.ifTable.ifEntry.ifDescr.x for example.       
 *                                                                        
 *   Parameters : pointer to string, i.e. "eth0", "bond0", etc.           
 *                                                                        
 *   Returns    : Index on success, -1 on failure                         
 ******************************************************************************/
int iGetInterfaceIndexFromName(char *pachName)
{
    int  iIfIndex = 0;
    static int procnetdev = -1;
    int readsize = 1024;
    int readcount;
    char *readbuf;
    char *end;
    char *next;
    char *colon;
    char *names[128];
    char *lines[128];

    if (procnetdev == -1) {
        if ((procnetdev = open("/proc/net/dev",O_RDONLY))  == -1) {
            return (-1);
        }
    }
    if (lseek(procnetdev, 0, SEEK_SET) != 0) {
        return -1;
    }
    readbuf = malloc(readsize);
    readcount = read(procnetdev,readbuf,readsize);
    while (readcount == readsize) {
        free(readbuf);
        if (lseek(procnetdev, 0, SEEK_SET) != 0)
            return -1;
        readsize = 2* readsize;
        readbuf = malloc(readsize);
        readcount = read(procnetdev,readbuf,readsize);
    }
    if (readcount <= 0)
        return -1;
    next = readbuf;
    while (next < readbuf + readcount) {
        end = strchr(next,'\n');
        *end = 0;
        if ((colon = strchr(next,':')) != NULL) {
            *colon = 0;
            lines[iIfIndex] = colon+1;
            while (*next == ' ')
                next++;
            names[iIfIndex++] = next;
            names[iIfIndex] = NULL;
        }
        next = end +1;
    }

    iIfIndex = 0;
    while (names[iIfIndex] != NULL) {
        if (strcmp(pachName, names[iIfIndex]) == 0) {
#if NIC_DEBUG 
            fprintf(stderr, "Index for %s is %d\n", 
                    pachName, iIfIndex+1);
#endif
            return (iIfIndex+1);
        }
        iIfIndex++;
    }

    return(-1);
}


/******************************************************************************
 *   Description: Initialize the nic library; this must be called in each 
 *                entry point of the library to insure the memory is set  
 *                up and cpqNic structure is initialized...               
 *   Parameters : 1) void                                                 
 *   Returns    : non-zero on error                                       
 ******************************************************************************/
int InitLibCpqNic(void)
{
    int   i;

    if (iLibNotInitialized) {
        iLibNotInitialized = 0;
        main_pid = getpid();
        //init_snmp_appname("cmanic"); /* for init_snmp_connection call */

        /* thread mutex init never fails... */

        memset(&NicMibVars, 0, sizeof(cpqNic_t));

        /* set the value of the mib we support, this is the MIB VERSION */
        NicMibVars.MibRevMajor = 1;
        NicMibVars.MibRevMinor = 13;

        NicMibVars.MibCondition = MIB_CONDITION_OK;
        cpqHoMibHealthStatusArray[CPQMIBHEALTHINDEX] = MIB_CONDITION_OK;

        NicMibVars.OsCommonPollFreq = OS_COMMON_POLL_FREQ;
        NicMibVars.MapOverallCondition = MAP_OVERALL_CONDITION_OK;
        NicMibVars.NumLogNics = 0;
        NicMibVars.NumPhysNics = 0;
        NicMibVars.NumVlans = 0;
        NicMibVars.pIfLog = (cpqNicIfLogical_t*)0;
        NicMibVars.pIfPhys = (cpqNicIfPhysical_t*)0;
        NicMibVars.pIfVlan = (cpqNicIfVlan_t*)0;

        pstrIfLastChange = (IfLastChange*)malloc(sizeof(IfLastChange)*
                NUM_IF_LAST_CHANGE_ENTRIES);
        if (pstrIfLastChange == (IfLastChange*)0) {
            fprintf(stderr, 
                    "ERROR: cpqnic could not alloc memory\n");
            return(1);
        }
        memset(pstrIfLastChange, 0, sizeof(IfLastChange)*
                NUM_IF_LAST_CHANGE_ENTRIES);

        for (i=0; i<NUM_IF_LAST_CHANGE_ENTRIES; i++) {
            pstrIfLastChange[i].pchInterfaceName = pchNoInterface;
            pstrIfLastChange[i].ulLastChanged = 0;
            pstrIfLastChange[i].pstrNext = (IfLastChange*)0;
        }

        /* create ethX.info files */
    }
    return(0);
}

/*----------------------------------------------------------------------*\
 *                                                                        
 *   Description:                                                         
 *                Set the ifLastChange value associated with the          
 *                given interface to the given value. If the interface    
 *                is not present, add an entry for it.                    
 *   Parameters :                                                         
 *                1) interface name (eth0, bond0, etc)                    
 *                2) uptime value to assign                               
 *   Returns    :                                                         
 *                non-zero on error                                       
 *                                                                        
 \*----------------------------------------------------------------------*/
int iSetIfLastChange(char *pchInterfaceName, unsigned long ulValue)
{
    int i;
    IfLastChange *pstrTemp;

#if NIC_DEBUG 
    fprintf(stderr, "in iSetIfLastChange for %s,%lu\n", 
            pchInterfaceName, ulValue);
#endif /* NIC_DEBUG  */

    /* the last char is used to index into the list to search */
    i = strlen(pchInterfaceName)-1;
    if (i<0) {
        /* this should never happen! so, no need to throttle */
        fprintf(stderr, 
                "ERROR: NULL interface name given\n");
        return(1);
    }

    i = ((int)pchInterfaceName[i])%NUM_IF_LAST_CHANGE_ENTRIES;
    pstrTemp = &pstrIfLastChange[i];

    /* fine entry to update */
    do {
        if (strcmp(pstrTemp->pchInterfaceName, pchInterfaceName) == 0) {
            /* we have a match */
            pstrTemp->ulLastChanged = ulValue;
            return(0);
        }

        /* check then advance in test below, this is so we */
        /* exit with an non-null ptr                       */
    } while ((pstrTemp->pstrNext != (IfLastChange*)0) &&
            ((pstrTemp = pstrTemp->pstrNext) != (IfLastChange*)0));

    /* no match found, add the interface */
    if (pstrTemp->pchInterfaceName != pchNoInterface) {
        /* root entry is not available since interface name is not null, add */
        /* a list element                                                    */
        pstrTemp->pstrNext = (IfLastChange*)malloc(sizeof(IfLastChange));
        if (pstrTemp->pstrNext == (IfLastChange*)0) {
            fprintf(stderr, 
                    "ERROR: cpqnic could not alloc memory\n");
            return(1);
        }
        memset(pstrTemp->pstrNext, 0 , sizeof(IfLastChange));
        pstrTemp = pstrTemp->pstrNext;
        pstrTemp->pstrNext = (IfLastChange*)0;
    }

    pstrTemp->pchInterfaceName = strdup(pchInterfaceName);
    if (pstrTemp->pchInterfaceName == (char*)0) {
        fprintf(stderr, 
                "ERROR: cpqnic could not alloc memory\n");
        return(1);
    }

    pstrTemp->ulLastChanged = ulValue;

    return(0);
}

#ifdef NOT_NEEDED
/*----------------------------------------------------------------------*\
 *                                                                        
 *   Description:                                                         
 *                Update the ifLastChange value associated with the       
 *                given interface. If the interface is not present, add   
 *                an entry for it with the current uptime.                
 *   Parameters :                                                         
 *                1) char* interface name (eth0, bond0, etc)              
 *   Returns    :                                                         
 *                non-zero on error                                       
 *                                                                        
 \*----------------------------------------------------------------------*/
int iUpdateIfLastChange(char *pchInterfaceName)
{
    return(iSetIfLastChange(pchInterfaceName, get_uptime()));
}
#endif

/*----------------------------------------------------------------------*\
 *                                                                        
 *   Description:                                                         
 *                Get the ifLastChange value associated with the          
 *                given interface.                                        
 *   Parameters :                                                         
 *                1) char* interface name (eth0, bond0, etc)              
 *   Returns    :                                                         
 *                ifLastChange associated with given interface            
 *                                                                        
 \*----------------------------------------------------------------------*/
unsigned long iGetIfLastChange(char *pchInterfaceName)
{
    int i;
    unsigned long ulUpTime;
    IfLastChange *pstrTemp;

    /* If the interface is not present, we return zero. */

#if NIC_DEBUG 
    fprintf(stderr, "in iGetIfLastChange for %s\n", pchInterfaceName);
#endif /* NIC_DEBUG  */

    /* the last char is used to index into the list to search */
    i = strlen(pchInterfaceName)-1;
    if (i<0) {
        /* this should never happen! so, no need to throttle */
        fprintf(stderr, 
                "ERROR: NULL interface name given\n");
        return(1);
    }

    i = ((int)pchInterfaceName[i])%NUM_IF_LAST_CHANGE_ENTRIES;
    pstrTemp = &pstrIfLastChange[i];

    /* fine entry to update */
    do {
        if (strcmp(pstrTemp->pchInterfaceName, pchInterfaceName) == 0) {
            /* we found the interface */
            ulUpTime = pstrTemp->ulLastChanged;

#if NIC_DEBUG 
            fprintf(stderr, "%s last change is %lu\n", 
                    pchInterfaceName, ulUpTime);
#endif /* NIC_DEBUG  */

            return(ulUpTime);
        }
    } while ((pstrTemp = pstrTemp->pstrNext) != (IfLastChange*)0);

#if NIC_DEBUG 
    fprintf(stderr, "%s last change is 0\n", 
            pchInterfaceName);
#endif /* NIC_DEBUG  */
    return(0);
}


/*----------------------------------------------------------------------*\
 *                                                                        
 *   Description:                                                         
 *                Returns the physical slot number associated with the    
 *                given pci bus and device/slot number...               
 *   Parameters :                                                         
 *                1) pci bus number                                       
 *                2) pci device/slot number                               
 *   Returns    :                                                         
 *                physical slot number card is in; -1 on error            
 *                                                                        
 \*----------------------------------------------------------------------*/
static int iGetSlotNumber(int iPciBus, int iPciDevice)
{
    int i;
    FILE*  phPipe;
    struct stat stat_buf;
    char   achTemp[TEMP_LEN];  

    /* vars for parsing "hplog -i" */
    int  iBusNum;
    int  iDeviceNum;
    int  iFunctionNum;
    int  iLinkIntA;
    int  iIrqBitMapIntA;
    int  iSlotNumber;
    struct timeval strCurrentTime;
    static struct timeval strTimeToLog1 = {0};
    static struct timeval strTimeToLog2 = {0};
    struct timezone strTimeZone; /* we don't care about the time zone */
    char * dummy;

    if (hplog_entry == NULL) {
        if (hplog_command == (char*)0) {
            if (stat(hplog_sbin, &stat_buf) != -1) {
                hplog_command = hplog_sbin;
            } else {
                static int log_error=1;
                if (log_error--) {
                    /*fprintf(stderr, 
                            "ERROR: could not find hplog utility\n"); */
                }
                return(-1);
            }
#if NIC_DEBUG 
            fprintf(stderr, "cmanic using %s\n", hplog_command);
#endif
        }
    
        sprintf(achTemp, "%s -i 2>/dev/null", hplog_command);
        phPipe = popen(achTemp, "r");
        if (phPipe != (FILE *)NULL) {
            /* read the first blank line */
            dummy = fgets(achTemp, TEMP_LEN, phPipe);
    
            /* read past the header  "BusNu...." */
            dummy = fgets(achTemp, TEMP_LEN, phPipe);
    
            while (fgets(achTemp, TEMP_LEN, phPipe) != NULL) {
                i = sscanf(achTemp, 
                        "%x %x %x %x %x %d", 
                        &iBusNum,
                        &iDeviceNum,
                        &iFunctionNum,
                        &iLinkIntA,
                        &iIrqBitMapIntA,
                        &iSlotNumber);

                if (i != NUM_CPQ_PCI_ITEMS) {
                    /* skip this line... log message for debug if not a blank line... */
                    if (i > 0) {
                        gettimeofday(&strCurrentTime, &strTimeZone);
                        if (strCurrentTime.tv_sec > strTimeToLog1.tv_sec) {
                            gettimeofday(&strTimeToLog1, &strTimeZone);
                            strTimeToLog1.tv_sec +=  3600; /* log only once an hour */
    
                            fprintf(stderr, 
                                    "ERROR: cpqnic short line from "
                                    "\"%s\", (%d,%d) line=\"%s\"\n",
                                    hplog_command, 
                                    i, NUM_CPQ_PCI_ITEMS,
                                    achTemp);
                        }
                    }
                    continue;
                }
                if (hplog_entry == NULL) {
                    hplog_entry = malloc (sizeof(hplog_t));
                    hplog_start = hplog_entry;
                } else {
                    hplog_entry->next = malloc (sizeof(hplog_t));
                    hplog_entry = hplog_entry->next;
                }
                hplog_entry->next = NULL;
                hplog_entry->iBusNum = iBusNum;
                hplog_entry->iDeviceNum = iDeviceNum;
                hplog_entry->iFunctionNum = iFunctionNum;
                hplog_entry->iSlotNumber = iSlotNumber;
            }

            pclose(phPipe);
        } else {
            gettimeofday(&strCurrentTime, &strTimeZone);
            if (strCurrentTime.tv_sec > strTimeToLog2.tv_sec) {
                gettimeofday(&strTimeToLog2, &strTimeZone);
                strTimeToLog2.tv_sec +=  3600; /* log only once an hour */
                fprintf(stderr, 
                        "ERROR: cpqnicd \"%s\" didn't run correctly, errno=%d\n",
                        hplog_command, errno);
            }
    
        }
    }
    for (hplog_entry = hplog_start; hplog_entry != NULL;) 
        if ((hplog_entry->iBusNum == iPciBus) && 
            (hplog_entry->iDeviceNum == iPciDevice)) {
              /* we found a match */
#if NIC_DEBUG 
            fprintf(stderr, "Found slot %d for pci bus/device %d,%d\n", 
                    iSlotNumber, iPciBus, iPciDevice);
#endif
                return(hplog_entry->iSlotNumber);
        } else 
            hplog_entry = hplog_entry->next;
    
#if NIC_DEBUG 
        fprintf(stderr,
                "ERROR: did not find match for pci bus/device %d,%d\n",
                iPciBus, iPciDevice);
#endif

    return(-1);
}

#ifdef NOT_NEEDED
/*----------------------------------------------------------------------*\
 *                                                                        
 *   Description:                                                         
 *                Prevent snmp sets; this is required to conform to the   
 *                peer architecture command line options...               
 *   Parameters :                                                         
 *                1) void                                                 
 *   Returns    :                                                         
 *                void                                                    
 *                                                                        
 \*----------------------------------------------------------------------*/
void vPreventSnmpSets(void)
{
    iAllowSnmpSets = 0;
    return;
}
#endif

/*----------------------------------------------------------------------*\
 *                                                                        
 *   Description:                                                         
 *                Determines the number of nic proc files; does not       
 *                validate contents of file; only determines number of    
 *                ethN.info files in the predefined directories           
 *   Parameters :                                                         
 *                1) void                                                 
 *   Returns    :                                                         
 *                number of ethN.info files found in predefined dirs      
 *                                                                        
 \*----------------------------------------------------------------------*/
    static int
iGetNicProcFileCount(void)
{
    int      i,iCount;
    struct dirent **filelist;

    int nic_select ( const struct dirent *d );

    iCount = scandir(SYS_CLASS_NET, &filelist, nic_select, alphasort);
    i=iCount;
    while(i--) 
        free(filelist[i]);
    free(filelist);

    return(iCount);
}

int nic_select(const struct dirent *entry)
{
    int      i = 0;
    while (nic_names[i] != (char *) 0) {
        if (strncmp(entry->d_name,nic_names[i],strlen(nic_names[i])) == 0)
            return(1);
        i++;
    }
    return 0;
}

/*----------------------------------------------------------------------*\
 *   Description:                                                         
 *                Free's nic port linked list of structs; safe to call    
 *                with null ptr                                           
 *   Parameters :                                                         
 *                1) nic ports                                            
 *   Returns    :                                                         
 *                void                                                    
 \*----------------------------------------------------------------------*/
static void vFreeNicPorts(NicPort **ppNicPorts)
{
    /* recursive free linked list */
    if (*ppNicPorts != (NicPort*)0) {
        vFreeNicPorts(&((*ppNicPorts)->next));

        /* free allocated members in structure */
        free((*ppNicPorts)->pchNicProcFile);
        free((*ppNicPorts)->pchDriverName);
        free((*ppNicPorts)->pchDriverVersion);

        free(*ppNicPorts);
        *ppNicPorts = (NicPort*)0;
    }
    return;
}


/*----------------------------------------------------------------------*\
 *                                                                        
 *   Description:                                                         
 *                Creates ordered linked list of ethN.info files found in 
 *                predefined procfs directories                           
 *   Parameters :                                                         
 *                1) nic ports structure                                  
 *   Returns    :                                                         
 *                number of elements procfs files loaded into linked list;
 *                negative number of failure                              
 *                                                                        
 \*----------------------------------------------------------------------*/
static int iGetNicProcFiles(NicPort **ppNicPorts)
{
    int      iNumProcFilesFound;
    char*    pchTemp;
    NicPort* pNicPort;
    NicPort* pNicPrevPort;
    NicPort* pNicCurrentPort;

    *ppNicPorts = (NicPort*)0;
    pNicPort   = (NicPort*)0;
    iNumProcFilesFound = 0;

    int      i,n,iCount;
    struct dirent **filelist;

    int nic_select ( const struct dirent *d );

    iCount=scandir(SYS_CLASS_NET, &filelist, nic_select, alphasort);
    if (iCount == 0)
        return(iCount);
    for (i = 0; i < iCount; i++) {

        if ((pNicPort = (NicPort*) malloc(sizeof(NicPort))) == (NicPort*)0) {
            fprintf(stderr, "ERROR: cpqnic out of memory\n");
            return(OUT_OF_MEMORY);
        }

        memset(pNicPort, 0, sizeof(NicPort));
        pNicPort->iStructIndex = i;

        /* init all ptrs in case of free */
        n = strlen(SYS_CLASS_NET) + strlen(filelist[i]->d_name) + 2;
        pNicPort->pchNicProcFile = malloc(n);
        memset(pNicPort->pchNicProcFile, 0, n);
        strcpy(pNicPort->pchNicProcFile, SYS_CLASS_NET);
        strcat(pNicPort->pchNicProcFile, filelist[i]->d_name);
        pNicPort->pchDriverName = (char*)0;
        pNicPort->pchDriverVersion = (char*)0;
        pNicPort->iProcfsStatus = PROCFS_OK;

        n=0;
        while (nic_names[n] != (char *)0) {
            if ((pchTemp = strstr(filelist[i]->d_name,nic_names[n]))!=(char*)0)
                pNicPort->iInterfaceNumber = atoi(pchTemp);
            n++;
        }

        pNicPort->next = (NicPort*)0;

        /* insert new element in sorted list */
        if (*ppNicPorts == (NicPort*)0) {
            /* first time through */
            *ppNicPorts = pNicPort;
        } else {
            /* find location to insert new element */
            pNicCurrentPort = *ppNicPorts;
            pNicPrevPort = pNicCurrentPort;
            while (pNicCurrentPort != (NicPort*)0) {
                if (pNicPort->iInterfaceNumber <
                        pNicCurrentPort->iInterfaceNumber) {                                                        
                    if (pNicPrevPort == pNicCurrentPort) {
                        /* insert before first element in list */
                        pNicPort->next = *ppNicPorts;
                        *ppNicPorts = pNicPort;
                        break;
                    } else {
                        /* insert between two elements */
                        pNicPrevPort->next = pNicPort;
                        pNicPort->next = pNicCurrentPort;
                        break;
                    }
                }                                                        
                pNicPrevPort = pNicCurrentPort;
                pNicCurrentPort = pNicCurrentPort->next;
            }

            if (pNicCurrentPort == (NicPort*)0) {
                /* insert at end of list */
                pNicPrevPort->next = pNicPort;
            }
        }
        free(filelist[i]);
    }
    free(filelist);

    return(iCount);
}

/******************************************************************************
 *   Description: Load the logical structure...                           
 *                                                                        
 *                NOTE: later we need to pass in a linked list of phys    
 *                      devices in that are associated with a logical     
 *                      team... OR maybe, this can be called with the     
 *                      same logical struct for each physical struct...   
 *                                                                        
 *                NOTE: This must be re-written when we have teaming....  
 *                      I can't look at the teaming code so I don't       
 *                      know the architecture to write code with this     
 *                      in mind...                                        
 *   Parameters : 1) physical interface to update logical interface struct
 *                   with                                                 
 *                2) logical interface to update                          
 *                3) logical map index                                    
 *                4) logical interface map to physical adapters           
 *   Returns    : positive value on success; negative value on failure    
 ******************************************************************************/
static int iGetLogicalInfo(cpqNicIfPhysical_t *pIfPhys, 
                           cpqNicIfLogical_t  *pIfLog, 
                           int                map_index,
                           int                map_phys_adapters)
{
    dynamic_interface_info_t dyn_if_info;

    /* begin cpqNicIfLogMapTable table variables */
    pIfLog->MapIndex = map_index;

    pIfLog->MapIfNumber[0] = pIfPhys->AdapterIfNumber[0];
    pIfLog->MapIfNumber[1] = pIfPhys->AdapterIfNumber[1];
    pIfLog->MapIfNumber[2] = pIfPhys->AdapterIfNumber[2];
    pIfLog->MapIfNumber[3] = pIfPhys->AdapterIfNumber[3];

    pIfLog->MapIfNumber[4] = 0;
    pIfLog->MapIfNumber[5] = 0;
    pIfLog->MapIfNumber[6] = 0;
    pIfLog->MapIfNumber[7] = 0;

    /* logical interface name is used for bonding/teaming */
    strcpy(pIfLog->InterfaceName, pIfPhys->InterfaceName);

    pIfLog->MapPhysicalAdapters[0] = (char)map_phys_adapters;

    pIfLog->MapDescription[0] = '\0';
    pIfLog->MapGroupType = MAP_GROUP_TYPE_NONE;
    pIfLog->MapAdapterCount = 1;
    pIfLog->MapAdapterOKCount = (pIfPhys->AdapterCondition == 
            ADAPTER_CONDITION_OK)?1:0;
    memcpy(pIfLog->MapMACAddress, pIfPhys->SoftwareMACAddress,
            IFLOGMAPMACADDRESS_LEN);


    /* get IPv4/6 address info */
    if (get_dynamic_interface_info(pIfLog->InterfaceName, &dyn_if_info) != -1) {
        if (dyn_if_info.pipv6_shorthand_addr != (char*)0) {
            sprintf(pIfLog->MapIPV6Address, "%s/%d",
                    dyn_if_info.pipv6_shorthand_addr, 
                    dyn_if_info.pipv6_refix_length);
        } else {
            strcpy(pIfLog->MapIPV6Address, "N/A");
        }
        STRCPY_NN(pIfLog->MapIPV4Address, dyn_if_info.pip_addr);
        free_dynamic_interface_info(&dyn_if_info);
    }

    pIfLog->MapSwitchoverMode = SWITCHOVER_MODE_NONE;
    pIfLog->MapCondition = pIfPhys->AdapterCondition;

    if (pIfPhys->AdapterCondition == ADAPTER_CONDITION_OK)
        pIfLog->MapStatus = MAP_STATUS_OK;
    else if (pIfPhys->AdapterCondition == ADAPTER_CONDITION_FAILED)
        pIfLog->MapStatus = MAP_STATUS_GROUP_FAILED;
    else
        pIfLog->MapStatus = MAP_STATUS_UNKNOWN;

    pIfLog->MapNumSwitchovers = 0;
    pIfLog->MapHwLocation[0] = '\0';
    pIfLog->MapSpeedMbps = pIfPhys->AdapterMapSpeedMbps;
    pIfLog->VlanCount = 0;
    pIfLog->MapVlans[0] = (char)0;

    return(0);
}


/******************************************************************************
 *   Description: Load the logical loopback structure                     
 *   Parameters : 1) logical interface to load with loopback info         
 *                2) logical map index                                    
 *                3) logical map interface number                         
 *   Returns    : X                                                       
 ******************************************************************************/
static void vLoadLogLoopback(cpqNicIfLogical_t  *pIfLog,
                             int                map_index,
                             int                map_if_number)
{

    int i;
    dynamic_interface_info_t dyn_if_info;

    pIfLog->MapIndex = map_index;
    strcpy(pIfLog->InterfaceName, "lo");

    pIfLog->MapIfNumber[0] = (char)((map_if_number) & 0xff);
    pIfLog->MapIfNumber[1] = (char)(((map_if_number)>>8)  & 0xff);
    pIfLog->MapIfNumber[2] = (char)(((map_if_number)>>16) & 0xff);
    pIfLog->MapIfNumber[3] = (char)(((map_if_number)>>24) & 0xff);

    pIfLog->MapIfNumber[4] = 0;
    pIfLog->MapIfNumber[5] = 0;
    pIfLog->MapIfNumber[6] = 0;
    pIfLog->MapIfNumber[7] = 0;

    pIfLog->MapPhysicalAdapters[0] = (char)0;

    pIfLog->MapGroupType = MAP_GROUP_TYPE_UNKNOWN;
    pIfLog->MapAdapterCount = 0;
    pIfLog->MapAdapterOKCount = 0;
    pIfLog->MapSwitchoverMode = SWITCHOVER_MODE_UNKNOWN;
    pIfLog->MapCondition = ADAPTER_CONDITION_OTHER;
    pIfLog->MapStatus = MAP_STATUS_UNKNOWN;

    pIfLog->MapDescription[0] = '\0';


    for (i=0; i<IFLOGMAPMACADDRESS_LEN; i++) {
        pIfLog->MapMACAddress[i] = '\0';
    }

    /* get IPv4/6 address info */
    if (get_dynamic_interface_info(pIfLog->InterfaceName, &dyn_if_info) != -1) {
        if (dyn_if_info.pipv6_shorthand_addr != (char*)0) {
            sprintf(pIfLog->MapIPV6Address, "%s/%d",
                    dyn_if_info.pipv6_shorthand_addr, 
                    dyn_if_info.pipv6_refix_length);
        } else {
            strcpy(pIfLog->MapIPV6Address, "N/A");
        }
        STRCPY_NN(pIfLog->MapIPV4Address, dyn_if_info.pip_addr);

        if (dyn_if_info.if_status == link_up)
            pIfLog->MapCondition = ADAPTER_CONDITION_OK;
        else if (dyn_if_info.if_status == link_down)
            pIfLog->MapCondition = ADAPTER_CONDITION_FAILED;

        free_dynamic_interface_info(&dyn_if_info);
    }

    if (pIfLog->MapCondition == ADAPTER_CONDITION_OK)
        pIfLog->MapStatus = MAP_STATUS_OK;

    pIfLog->MapNumSwitchovers = 0;
    pIfLog->MapHwLocation[0] = '\0';
    pIfLog->MapSpeedMbps = 0;
    pIfLog->VlanCount = 0;
    pIfLog->MapVlans[0] = (char)0;

}

/******************************************************************************
 *   Description: Determine advertised speed/duplex, for                  
 *                cpqNicIfPhysAdapterConfSpeedDuplex MIB object           
 *   Parameters : ptr to physical NIC struct                           
 *   Returns    : void                                                    
 ******************************************************************************/
static void vGetConfiguredSpeedDuplex(cpqNicIfPhysical_t *pIfPhys)
{
    /* TODO: if the link is not up we can't determine the configured  */
    /*       speed/duplex. there is no clean way to get it today if   */
    /*       the link is not up.                                      */
    if (pIfPhys->AdapterAutoNegotiate) {
        pIfPhys->AdapterConfiguredSpeedDuplex = CONF_SPEED_DUPLEX_AUTO;
        return;
    }

    if ((UNKNOWN_SPEED == pIfPhys->AdapterMapSpeedMbps) ||
            (UNKNOWN_DUPLEX == pIfPhys->AdapterDuplexState)) {
        pIfPhys->AdapterConfiguredSpeedDuplex = CONF_SPEED_DUPLEX_OTHER;
        return;
    }

    if (FULL_DUPLEX == pIfPhys->AdapterDuplexState) {
        switch (pIfPhys->AdapterMapSpeedMbps) {

            case 10000:  /* 10Gb   */
                pIfPhys->AdapterConfiguredSpeedDuplex = 
                    CONF_SPEED_DUPLEX_10GIG_ENET_FULL;
                break;

            case 1000:   /* 1Gb   */
                pIfPhys->AdapterConfiguredSpeedDuplex = 
                    CONF_SPEED_DUPLEX_GIG_ENET_FULL;
                break;

            case 100:    /* 100 Mb */
                pIfPhys->AdapterConfiguredSpeedDuplex = 
                    CONF_SPEED_DUPLEX_FAST_ENET_FULL;
                break;

            case 10:     /* 10 Mb   */
                pIfPhys->AdapterConfiguredSpeedDuplex = 
                    CONF_SPEED_DUPLEX_ENET_FULL;
                break;

            default:
                pIfPhys->AdapterConfiguredSpeedDuplex = CONF_SPEED_DUPLEX_OTHER;
                break;
        }
    } else if (HALF_DUPLEX == pIfPhys->AdapterDuplexState) {
        switch (pIfPhys->AdapterMapSpeedMbps) {

            case 100: /* 100 Mb */
                pIfPhys->AdapterConfiguredSpeedDuplex = 
                    CONF_SPEED_DUPLEX_FAST_ENET_HALF;
                break;

            case 10: /* 10Mb   */
                pIfPhys->AdapterConfiguredSpeedDuplex = 
                    CONF_SPEED_DUPLEX_ENET_HALF;
                break;

            default:
                pIfPhys->AdapterConfiguredSpeedDuplex = CONF_SPEED_DUPLEX_OTHER;
                break;
        }
    } else {
        pIfPhys->AdapterConfiguredSpeedDuplex = CONF_SPEED_DUPLEX_OTHER;
    }
}



/*----------------------------------------------------------------------*\
 *                                                                        
 *   Description:                                                         
 *                Load stats from procfs into provided IfPhys struct      
 *   Parameters :                                                         
 *                1) nic port to load contents from                       
 *                2) physical interface struct to load                    
 *   Returns    :                                                         
 *                zero on success; non-zero on failure                    
 *                                                                        
 \*----------------------------------------------------------------------*/
static int iLoadPhysStruct(int do_remap, NicPort *pNicPort, cpqNicIfPhysical_t *pIfPhys)
{

    int     iMacLoop;
    int     iIndex;
    char*   pchTemp;
    char    achTemp[TEMP_LEN];  

    static int sock = -1;
    struct ifreq ifr;

    nic_hw_db_t *pnic_hw_db;
    ethtool_info einfo;
    int32_t slot;
    int32_t port;
    int32_t bay_slot;
    int32_t bay_port;
    int32_t virtual_port;
    static int32_t mlxport = -1;

    int     domain,bus,device,function;
    char    sysfsDir[1024];
    char    devBuf[1024];
    char    driverBuf[1024];
    int     tmpLen;
    int     tmpFd;
    int     dummy;


    if (sock < 0) {
        sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
        if (sock < 0) {
#if NIC_DEBUG 
            fprintf(stderr, "socket() failed: %d(%s)\n",
                    errno, strerror(errno));
#endif /* NIC_DEBUG  */
            return(PROCFS_REMAP_NEEDED);
        }
    }
    ifr.ifr_data = (char*)0;
    if (do_remap == 1) {
        /* Initializae sys dir to "/sys/class/net/eth?" */
        strcpy(&sysfsDir[0],pNicPort->pchNicProcFile);
        tmpLen = strlen(&sysfsDir[0]);
    
        /* Get Driver name from link name */
        strcat(&sysfsDir[0],"/device");
        dummy = readlink(&sysfsDir[0],&devBuf[0], 1024);
        sscanf(strrchr(&devBuf[0],'/')+1, "%x:%x:%x.%x",
                        &domain, &bus, &device, &function);

        strcat(&sysfsDir[0],"/driver");
        dummy = readlink(&sysfsDir[0],&driverBuf[0], 1024);
        pNicPort->pchDriverName = strdup(strrchr(&driverBuf[0],'/'));
        /* Get Driver Version */
        strcat(&sysfsDir[0],"/module/version");
        if ((tmpFd= open(&sysfsDir[0],O_RDONLY)) < 0) 
            fprintf(stderr,"Could not open %s\n",&sysfsDir[0]);
        else 
            if (read(tmpFd,&driverBuf[0],1024) >=0 ){
                pNicPort->pchDriverVersion = strdup(&driverBuf[0]);
                close(tmpFd);
            }
        sysfsDir[tmpLen] = 0;

        /* init values to defaults in case they are not in procfs */
        pIfPhys->AdapterSlot = -1;
        pIfPhys->AdapterPort = -1;
        pIfPhys->AdapterVirtualPort = -1;
        pIfPhys->AdapterIoBayNo = -1;
        pIfPhys->PCI_Bus = bus;
        pIfPhys->AdapterMapSpeedMbps = UNKNOWN_SPEED;
        pIfPhys->AdapterConfiguredSpeedDuplex = CONF_SPEED_DUPLEX_OTHER;
        pIfPhys->AdapterAutoNegotiate = 0;
        pIfPhys->AdapterMemAddr = 0;
        pIfPhys->AdapterFWVersion[0] = '\0';
        strcpy(pIfPhys->AdapterPartNumber, "UNKNOWN");
        strcpy(pIfPhys->AdapterName, "EMPTY");        /* default "empty" per mib */
    
        if (pIfPhys->AdapterPort != -1) {
            /* adapter port is one based.... if we found the port (bus) in */
            /* the procfs bump it by one.                                  */
            pIfPhys->AdapterPort++;
        }

        /***************************************************************/
        /* now fill the members that we can't get form the procfs file */
        /***************************************************************/
        /* comment out and fill in later to fix rQm 211783     */
        /* pIfPhys->AdapterIndex = pNicPort->iStructIndex + 1; */
    
        /*************************************************************
        * the interfaces portion of the mib looks like
        *   interfaces.ifTable.ifEntry.ifIndex.1 = 1
        *   interfaces.ifTable.ifEntry.ifIndex.2 = 2
        *   interfaces.ifTable.ifEntry.ifIndex.3 = 3
        *   interfaces.ifTable.ifEntry.ifDescr.1 = lo0
        *   interfaces.ifTable.ifEntry.ifDescr.2 = eth0
        *   interfaces.ifTable.ifEntry.ifDescr.3 = eth1
        * so, we need to add two to our interface number to get the 
        * correct mib-ii interface number...                          
        *************************************************************/
    

        pIfPhys->AdapterRole = ADAPTER_ROLE_NOTAPPLICABLE;

        /* if the driver did not provide the interface name, use the info file */
        /* name from the procfs file... ethX.info, use ethX.                   */
        strcpy(&pIfPhys->InterfaceName[0],
                strrchr(pNicPort->pchNicProcFile,'/') + 1);
        iIndex = iGetInterfaceIndexFromName(pIfPhys->InterfaceName);
        if (iIndex < 0) {
            fprintf(stderr, 
                    "ERROR: cpqnic could not get index for "
                    "interface %s\n",
                    pIfPhys->InterfaceName);
            iIndex = 0;  /* fail-safe... */
        }
    
        pIfPhys->AdapterIfNumber[0] = (char)(iIndex & 0xff);
        pIfPhys->AdapterIfNumber[1] = (char)((iIndex>>8)  & 0xff);
        pIfPhys->AdapterIfNumber[2] = (char)((iIndex>>16) & 0xff);
        pIfPhys->AdapterIfNumber[3] = (char)((iIndex>>24) & 0xff);
    
        pIfPhys->AdapterIfNumber[4] = 0;
        pIfPhys->AdapterIfNumber[5] = 0;
        pIfPhys->AdapterIfNumber[6] = 0;
        pIfPhys->AdapterIfNumber[7] = 0;
    
    
        strcpy(ifr.ifr_name, pIfPhys->InterfaceName);
    
        if (ioctl(sock, SIOCGIFMAP, &ifr) < 0) {
#if NIC_DEBUG 
            fprintf(stderr, "ioctl(SIOCGIFMAP): %s\n", 
                    strerror(errno));
#endif /* NIC_DEBUG  */
            return(PROCFS_REMAP_NEEDED);
        }

        pIfPhys->AdapterIoAddr = ifr.ifr_map.base_addr;
        pIfPhys->AdapterDma = ((int)ifr.ifr_map.dma == 0)?-1:(int)ifr.ifr_map.dma;

        if (ifr.ifr_map.mem_start) {
            pIfPhys->AdapterMemAddr = ifr.ifr_map.mem_start;
        }

        pIfPhys->AdapterHwLocation[0] = '\0';
    
        sprintf(achTemp, "/sys/class/net/%s/device/vendor", 
                         pIfPhys->InterfaceName);
        pIfPhys->VendorID = get_sysfs_hex(achTemp); 

        sprintf(achTemp, "/sys/class/net/%s/device/device", 
                         pIfPhys->InterfaceName);
        pIfPhys->DeviceID = get_sysfs_hex(achTemp);

        sprintf(achTemp, "/sys/class/net/%s/device/subsystem_vendor", 
                         pIfPhys->InterfaceName);
        pIfPhys->SubsysVendorID = get_sysfs_hex(achTemp);

        sprintf(achTemp, "/sys/class/net/%s/device/subsystem_device", 
                         pIfPhys->InterfaceName);
        pIfPhys->SubsysDeviceID = get_sysfs_hex(achTemp);

        DEBUGMSGTL(("cpqnic","Vendor = %x, Device = %x, " 
                            "Subsys Vendor = %x, Subsys Device = %x\n",
                           pIfPhys->VendorID, pIfPhys->DeviceID,
                           pIfPhys->SubsysVendorID, pIfPhys->SubsysDeviceID));
        /* get model name and part number from database created struct */
        if ((pnic_hw_db = 
                    get_nic_hw_info( pIfPhys->VendorID, pIfPhys->DeviceID,
                           pIfPhys->SubsysVendorID, pIfPhys->SubsysDeviceID))
                 != NULL) {
            /* over-ride info read from procfs/hpetfe */
            strcpy(pIfPhys->AdapterName, pnic_hw_db->pname);
            strcpy(pIfPhys->AdapterPartNumber, pnic_hw_db->ppca_part_number);
        }
    
        sprintf(achTemp, "/sys/class/net/%s/device/irq",
                         pIfPhys->InterfaceName);
        pIfPhys->AdapterIrq = get_sysfs_int(achTemp);
    
        pIfPhys->PCI_Slot = 0;

        if (get_ethtool_info(pIfPhys->InterfaceName, &einfo) == 0) {
            pIfPhys->AdapterAutoNegotiate =  einfo.autoneg; 
            if (einfo.duplex == DUPLEX_FULL)  
                pIfPhys->AdapterDuplexState =  FULL_DUPLEX; 
            else
                pIfPhys->AdapterDuplexState =  HALF_DUPLEX; 
                
            /*
            * Mellanox Card NC542m has bus, device and function same for both 
            * the ports.  
            */

            if ((pIfPhys->VendorID == 0x15B3) && 
                (pIfPhys->DeviceID == 0x5A44) && 
                (pIfPhys->SubsysVendorID == 0x3107) && 
                (pIfPhys->SubsysDeviceID == 0x103C)) {
                mlxport++;
            }		

            if (get_slot_info(einfo.pbus_info, &slot, &port, 
                &bay_slot, &bay_port, &virtual_port, mlxport) == 0) {
                if ((bay_slot != UNKNOWN_SLOT) && (bay_port != UNKNOWN_PORT)) {
                    pIfPhys->AdapterIoBayNo = bay_slot;
                    pIfPhys->AdapterPort = bay_port;
                    pIfPhys->AdapterVirtualPort = virtual_port;
                } 

            }

            if ( einfo.pfirmware_version ) 
                strcpy(pIfPhys->AdapterFWVersion, einfo.pfirmware_version);

            if (pIfPhys->AdapterMapSpeedMbps == UNKNOWN_SPEED) {
                pIfPhys->AdapterMapSpeedMbps = einfo.link_speed;
            }

            for (iMacLoop=0; iMacLoop<MAC_ADDRESS_BYTES; iMacLoop++) {
                /* If einfo.perm_addr is all 0's, the ioctl must've failed */
                if (einfo.perm_addr[iMacLoop]) {
                    memcpy(pIfPhys->AdapterMACAddress, einfo.perm_addr, 
                            sizeof(pIfPhys->AdapterMACAddress));
                    break;
                }
            }

            free_ethtool_info_members(&einfo);
        } else {
            /* we should not be here, something has changed about this 
            interface below our feet. */ 	
            return (-1);
        }

        vGetConfiguredSpeedDuplex(pIfPhys);
    }
    sprintf(achTemp,"/sys/class/net/%s/operstate",pIfPhys->InterfaceName);
    pchTemp = get_sysfs_str(achTemp);
    if (pchTemp != NULL) {
        if (strcmp(pchTemp, "up") == 0) {
            pIfPhys->AdapterCondition = ADAPTER_CONDITION_OK;
            pIfPhys->AdapterStatus = STATUS_OK;
        } else if (strcmp(pchTemp, "down") == 0) {
            pIfPhys->AdapterCondition = ADAPTER_CONDITION_FAILED;
            pIfPhys->AdapterStatus = STATUS_LINK_FAILURE;
        } else {
            pIfPhys->AdapterCondition = ADAPTER_CONDITION_OTHER;
            pIfPhys->AdapterStatus = STATUS_UNKNOWN;
        }
        free(pchTemp);
    }
    
    if (pIfPhys->AdapterStatus == STATUS_OK)
        pIfPhys->AdapterState = ADAPTER_STATE_ACTIVE;
    else if (pIfPhys->AdapterStatus == STATUS_LINK_FAILURE)
        pIfPhys->AdapterState = ADAPTER_STATE_FAILED;
    else 
        pIfPhys->AdapterState = ADAPTER_STATE_UNKNOWN;

    pIfPhys->AdapterStatsValid = ADAPTER_STATS_VALID_TRUE;

    pIfPhys->AdapterBadTransmits =
        pIfPhys->AdapterDeferredTransmissions +
        pIfPhys->AdapterLateCollisions +
        pIfPhys->AdapterCarrierSenseErrors +
        pIfPhys->AdapterInternalMacTransmitErrors;

    pIfPhys->AdapterBadReceives  =
        pIfPhys->AdapterAlignmentErrors +
        pIfPhys->AdapterFCSErrors +
        pIfPhys->AdapterFrameTooLongs +
        pIfPhys->AdapterInternalMacReceiveErrors;

    sprintf(achTemp, "/sys/class/net/%s/statistics/rx_bytes",
                     pIfPhys->InterfaceName);
    pIfPhys->AdapterInOctets = get_sysfs_int(achTemp);
    sprintf(achTemp, "/sys/class/net/%s/statistics/tx_bytes",
                     pIfPhys->InterfaceName);
    pIfPhys->AdapterOutOctets = get_sysfs_int(achTemp);
    DEBUGMSGTL(("cpqNic", "rx_bytes= %d tx_bytes = %d\n", 
                            pIfPhys->AdapterInOctets,
                            pIfPhys->AdapterOutOctets));

    sprintf(achTemp, "/sys/class/net/%s/statistics/rx_packets",
                     pIfPhys->InterfaceName);
    pIfPhys->AdapterGoodReceives = get_sysfs_int(achTemp);
    sprintf(achTemp, "/sys/class/net/%s/statistics/tx_packets",
                     pIfPhys->InterfaceName);
    pIfPhys->AdapterGoodTransmits = get_sysfs_int(achTemp);
    DEBUGMSGTL(("cpqNic", "rx_packets= %d tx_packets = %d\n", 
                            pIfPhys->AdapterGoodReceives,
                            pIfPhys->AdapterGoodTransmits));

    return(0);
}


/******************************************************************************
 *   Description: This is to be used in conjunction with                  
 *                set_common_poll_freq; the snmp agents need a way to     
 *                insure the value passed to set_common_poll_freq will    
 *                succeed for the snmp multi-stage-commit feature; this   
 *                function give the snmp agent a means to conform to      
 *                that requirement                                         | 
 *   Parameters : 1) new poll frequency                                   
 *   Returns    : zero on success; non-zero on failure                    
 ******************************************************************************/
int iCheckCommonPollFreq(int new_poll_freq)
{
    if (0 == NicMibVars.OsCommonPollFreq) {
        /* mib states poll freq of zero can't be changed */
        return(-1);
    }

    if (0 >= new_poll_freq) {
        /* mib states setting poll freq to zero will     */
        /* always fail                                   */
        return(-1);
    }

    return(0);
} 

#ifdef NOT_NEEDED
/******************************************************************************
 *   Description: Sets the minimum poll frequency.                        
 *   Parameters : 1) minimum poll frequency                               
 *   Returns    : void                                                    
 ******************************************************************************/
void vSetMinPollFreq(int new_min_poll_freq)
{
    iMinPollFrequency = new_min_poll_freq;
    return;
}

/******************************************************************************
 *   Description: Force the MIB structures to be re-read.                 
 *   Parameters : 1) void                                                 
 *   Returns    : none                                                    
 ******************************************************************************/
void vForceVariableRecache(void)
{
    iTimeToRecache = 0;
}

/******************************************************************************
 *   Description: Just an abstractoin layer - provided to give snmp agent 
 *                a way to indirectly the modify structure; we don't want 
 *                the agent directly modifying any structure; this is an  
 *                interface for the outside world, not the inside...      
 *   Parameters : 1) new poll frequency                                   
 *   Returns    : zero on success; non-zero on failure                    
 ******************************************************************************/
int iSetCommonPollFreq(int new_poll_freq)
{
    int rvalue;

    if (!iAllowSnmpSets) {
        return(-1);
    }

    if (new_poll_freq < iMinPollFrequency) {
        /* value less than caller specified value */
        return(-1);
    }

    /* the calling app should have checked this, but... */
    if (0 != (rvalue = iCheckCommonPollFreq(new_poll_freq))) {
        return(rvalue);
    }

    /* reset the re-cache time so the new poll_freq will be into effect */
    iTimeToRecache = 0;
    NicMibVars.OsCommonPollFreq = new_poll_freq;
    return(0);
}
#endif

#ifdef BONDING
/******************************************************************************
 *   Description: Free list created by pstrBuildBondInterfaceList function.|
 *   Parameters : pointer to pointer to BondDevice                        
 *   Returns    : nothing                                                 
 ******************************************************************************/
void vFreeBondDeviceList(BondDevice** ppBondDevice)
{
    if (*ppBondDevice != (BondDevice*)0) {
        vFreeBondDeviceList(&((*ppBondDevice)->pstrNext));
        free(*ppBondDevice);
        *ppBondDevice = (BondDevice*)0;
    }
    return;
}


/******************************************************************************
 *   Description: Free list created by pstrBuildBondInterfaceList function.|
 *   Parameters : pointer to pointer to BondInterface                     
 *   Returns    : nothing                                                 
 ******************************************************************************/
void vFreeBondInterfaceList(BondInterface** ppBondList)
{
    /* recursive free */
    if (*ppBondList != (BondInterface*)0) {
        vFreeBondInterfaceList(&((*ppBondList)->pstrNext));

        /* free allocated members in structure */
        vFreeBondDeviceList(&((*ppBondList)->pstrBondDevice));
        free(*ppBondList);
        *ppBondList = (BondInterface*)0;
    }

    return;
}


/******************************************************************************
 *   Description: Print the bond interface list and all devices           
 *   Parameters : Pointer to BondInterface struct                         
 *   Returns    : none                                                    
 ******************************************************************************/
void vPrintBondInterfaceList(BondInterface* pstrBond)
{
    BondDevice* pstrBondDevice;

    if (pstrBond == (BondInterface*)0) {
        return;
    }

    fprintf(stderr, "\n");
    fprintf(stderr, 
            "################################################################\n");

    fprintf(stderr, 
            "Current Bonding Interfaces\n");

    while (pstrBond != (BondInterface*)0) {
        fprintf(stderr, "------------------------------------\n");
        fprintf(stderr, "name:               %s\n", 
                pstrBond->achBondName);
        fprintf(stderr, "directory:          %s\n", 
                pstrBond->achBondDir);

        fprintf(stderr, "mode:               ");
        if (pstrBond->iBondingMode == ROUND_ROBIN_BOND) {
            fprintf(stderr, "load balancing (round-robin)\n");
        } else if (pstrBond->iBondingMode == XOR_BOND) {
            fprintf(stderr, "load balancing (xor)\n");
        } else if (pstrBond->iBondingMode == ACTIVE_BACKUP_BOND) {
            fprintf(stderr, "fault-tolerance (active-backup); "
                    "active interface %s\n",
                    pstrBond->achActiveInterface);
        } else if (pstrBond->iBondingMode == BROADCAST_BOND) {
            fprintf(stderr, "fault-tolerance (broadcast)\n");
        } else if (pstrBond->iBondingMode == IEEE_8023AD_BOND) {
            fprintf(stderr, "IEEE 802.3ad Dynamic link aggregation\n");
        } else if (pstrBond->iBondingMode == TLB_BOND) {
            fprintf(stderr, "transmit load balancing\n");
        } else if (pstrBond->iBondingMode == ALB_BOND) {
            fprintf(stderr, "adaptive load balancing\n");
        } else if (pstrBond->iBondingMode == UNKNOWN_BOND) {
            fprintf(stderr, "unknown bonding mode\n");
        } else {
            fprintf(stderr, "ERROR: UNKNOWN bonding mode\n");
        }

        fprintf(stderr, "MII Status:         ");
        if (pstrBond->iMIIStatus == MII_STATUS_UP) {
            fprintf(stderr, "up\n");
        } else {
            fprintf(stderr, "down\n");
        }

        fprintf(stderr, "Link failure count: %d\n", pstrBond->iLinkFailureCount);
        fprintf(stderr, "# of interfaces:    %d\n", pstrBond->iNumInterfaces);
        fprintf(stderr, "# of link ok count: %d\n", pstrBond->iLinkOKCount);
        fprintf(stderr, "SNMP index:         %d\n", pstrBond->iSNMPIndex);


        /* now go through each device belonging to this bond */
        pstrBondDevice = pstrBond->pstrBondDevice;
        while (pstrBondDevice != (BondDevice*)0) {
            fprintf(stderr, "\n");

            fprintf(stderr, "\tInterface name: %s\n",
                    pstrBondDevice->achInterfaceName);

            fprintf(stderr, "\tMII Status:     ");
            if (pstrBondDevice->iMIIStatus == MII_STATUS_UP) {
                fprintf(stderr, "up\n");
            } else {
                fprintf(stderr, "down\n");
            }

            fprintf(stderr, "\tFailure count:  %d\n",
                    pstrBondDevice->iLinkFailureCount);

            fprintf(stderr, "\tSNMP Index:     %d\n",
                    pstrBondDevice->iSNMPIndex);

            pstrBondDevice = pstrBondDevice->pstrNext;
        }

        fprintf(stderr, "\n");
        pstrBond = pstrBond->pstrNext;
    }
}


/******************************************************************************
 *   Description: Build a list that represents the current bonding        
 *                interfaces - pre 802.3ad changes; returned struct must  
 *                be freed by calling function.                           
 *   Parameters : none                                                    
 *   Returns    : pointer to BondInterface structure                      
 ******************************************************************************/
BondInterface* pstrBuildOldBondInterfaceList(void)
{
    FILE* phPipe;
    FILE* phBondInfo;
    char* pchTemp;
    BondInterface* pstrBondHead = (BondInterface*)0;
    BondInterface* pstrBondTail;
    BondInterface* pstrBond;
    BondDevice* pstrBondDevice;
    BondDevice* pstrLastBondDevice;
    static int iLogError = 25;   /* log the first 25 errors */
    char           achTemp[TEMP_LEN];  

#if NIC_DEBUG 
    fprintf(stderr, "cmanic: checking for old bonding driver\n");
#endif /* NIC_DEBUG  */

    /* if bonding is not running, do nothing */
    /* list one directory per line */
    sprintf(achTemp, "ls -d1 /proc/net/bond* 2>/dev/null");
    phPipe = popen(achTemp, "r");
    if (phPipe == (FILE *)NULL) {
        /* no bonds found */
#if NIC_DEBUG 
        fprintf(stderr, "cmanic: No bonds found\n");
#endif /* NIC_DEBUG  */
        return((BondInterface*)0);
    }

    /* for each bond/team */
    while (fgets(achTemp, TEMP_LEN, phPipe) != NULL) {
        CLOBBER_NL(achTemp);
        /* allocate struct */
        pstrBond = (BondInterface*) malloc(sizeof(BondInterface));

        if (pstrBond == (BondInterface*)0) {
            vFreeBondInterfaceList(&pstrBondHead);
            if (iLogError > 0) {
                fprintf(stderr, 
                        "ERROR: cpqnic can't allocated memory\n");
                iLogError--;
            }
            return((BondInterface*)0);
        }

        pstrBond->iLinkFailureCount = 0;
        pstrBond->iNumInterfaces = 0;
        pstrBond->iLinkOKCount = 0;
        pstrBond->uiMapSpeedMbps = 0;
        pstrBond->pstrBondDevice = (BondDevice*)0;
        pstrBond->pstrNext = (BondInterface*)0;

        strncpy(pstrBond->achBondDir, achTemp, MAX_BOND_DIR_LEN);
        pstrBond->achBondDir[MAX_BOND_DIR_LEN-1] = '\0';

        /* bond name is directory name, i.e. bond0 */
        pchTemp = strrchr(pstrBond->achBondDir, '/');

        if (pchTemp == (char*)0) {
            /* something is screwed up */
#if NIC_DEBUG 
            fprintf(stderr, 
                    "cmanic: bond name not found in \"%s\"\n", 
                    pstrBond->achBondDir);
            fprintf(stderr, 
                    "cmanic: bond interface will not be "
                    "added agent list\n");
#endif /* NIC_DEBUG  */

            free(pstrBond);
            pstrBond = (BondInterface*)0;
            continue;
        }


        pchTemp += 1;
        strcpy(pstrBond->achBondName, pchTemp);

        /* link the struct to our list */
        if (pstrBondHead == (BondInterface*)0) {
            /* first time through */
            pstrBondHead = pstrBond;
            pstrBondTail = pstrBond;
        } else {
            pstrBondTail->pstrNext = pstrBond;
            pstrBondTail = pstrBond;
        }
    }
    pclose(phPipe);


    /* read contents of each bondX/info file */
    for (pstrBond = pstrBondHead;
            pstrBond != (BondInterface*)0;
            pstrBond = pstrBond->pstrNext) {
        sprintf(achTemp, "%s/info", pstrBond->achBondDir);
        phBondInfo = fopen(achTemp, "r");
        if (phBondInfo == NULL) {
            if (iLogError > 0) {
                fprintf(stderr, "ERROR: cpqnic could not open %s\n",
                        achTemp);
                iLogError--;
            }
            vFreeBondInterfaceList(&pstrBondHead);
            return((BondInterface*)0);
        }

        /**********************************************************************/
        /* the /proc/net/bondX/info file looks like this                      */
        /*                                                                    */
        /*      Bonding Mode: load balancing (round-robin)                    */
        /*      Currently Active Slave: eth0 <<<<< only in NFT if it is up!   */
        /*      MII Status: up                                                */
        /*      MII Polling Interval (ms): 1000                               */
        /*      Up Delay (ms): 0                                              */
        /*      Down Delay (ms): 0                                            */
        /*      Multicast Mode: all slaves                                    */
        /*                                                                    */
        /*      Slave Interface: eth1                                         */
        /*      MII Status: down                                              */
        /*      Link Failure Count: 4513                                      */
        /*                                                                    */
        /*      Slave Interface: eth0                                         */
        /*      MII Status: up                                                */
        /*      Link Failure Count: 2                                         */
        /**********************************************************************/

        /* we can try to make this locate the correct string or */
        /* kinda hard code things... we will hard code things   */
        /* and if the file changes log an error - too much      */
        /* over-head in trying to locate the correct string...  */

        /* read content of file into struct */

        /* parse line like "Bonding Mode: load balancing (round-robin)" */
        fgets(achTemp, TEMP_LEN, phBondInfo);
        CLOBBER_NL(achTemp);

        if (strstr(achTemp, "round-robin") != (char*)0) {
            pstrBond->iBondingMode = ROUND_ROBIN_BOND;
        }
        else if (strstr(achTemp, "xor") != (char*)0) {
            pstrBond->iBondingMode = XOR_BOND;
        }
        else if (strstr(achTemp, "active-backup") != (char*)0) {
            pstrBond->iBondingMode = ACTIVE_BACKUP_BOND;
        }
        else if (strstr(achTemp, "broadcast") != (char*)0) {
            pstrBond->iBondingMode = BROADCAST_BOND;
        } else {
            if (iLogError > 0) {
                fprintf(stderr, 
                        "ERROR: cpqnic could not parse bonding mode "
                        "token from \"%s\"\n",
                        achTemp);
                fprintf(stderr, 
                        "       ensure you are using a supported bonding "
                        "driver\n");
                iLogError--;
            }
            fclose(phBondInfo);
            vFreeBondInterfaceList(&pstrBondHead);
            return((BondInterface*)0);
        }

#if NIC_DEBUG 
        fprintf(stderr, 
                "cmanic: bonding mode = %d\n", 
                pstrBond->iBondingMode);
#endif /* NIC_DEBUG  */

        if (pstrBond->iBondingMode == ACTIVE_BACKUP_BOND) {
            /**********************************/
            /* "Currently Active Slave: eth1" */
            /**********************************/
            fgets(achTemp, TEMP_LEN, phBondInfo);
            CLOBBER_NL(achTemp);

            /* if no interfaces are assocated with the bond, there will not be */
            /* a "Currently Active Slave.." line; this is also true if all     */
            /* slaves are down; we must check for it and prevent reading past  */
            /* eof in this case.                                               */
            if (strstr(achTemp, "Currently Active Slave:") == (char*)0) {
#if NIC_DEBUG 
                fprintf(stderr, 
                        "cmanic: %s found without ethX interfaces\n", 
                        pstrBond->achBondName);
#endif /* NIC_DEBUG  */
                strcpy(pstrBond->achActiveInterface, "none");
            } else {
                pchTemp = strrchr(achTemp, ':');
                if ((pchTemp == (char*)0) || (strlen(pchTemp) < 3)) {
                    if (iLogError > 0) {
                        fprintf(stderr, 
                                "ERROR: cpqnic could not parse token "
                                "from \"%s\"\n",
                                achTemp);
                        fprintf(stderr, 
                                "       ensure you are using a supported bonding "
                                "driver\n");
                        iLogError--;
                    }
                    fclose(phBondInfo);
                    vFreeBondInterfaceList(&pstrBondHead);
                    return((BondInterface*)0);
                }

                pchTemp += 2;
                strncpy(pstrBond->achActiveInterface, pchTemp, MAX_BOND_IFACE_LEN);
                pstrBond->achActiveInterface[MAX_BOND_IFACE_LEN-1] = '\0';
            }
        }

        /* get bond status "MII Status: up" */
        if (strcmp(pstrBond->achActiveInterface, "none") != 0) {
            /* if we read the line above, don't advance past the line */
            /* we need to process...                                  */
            fgets(achTemp, TEMP_LEN, phBondInfo);
            CLOBBER_NL(achTemp);
        }

        if (strstr(achTemp, "up") == (char*)0) {
            pstrBond->iMIIStatus = MII_STATUS_DOWN;
        } else {
            pstrBond->iMIIStatus = MII_STATUS_UP;
        }

#if NIC_DEBUG 
        fprintf(stderr, "MIIStatus = %d\n", 
                pstrBond->iMIIStatus);
#endif /* NIC_DEBUG  */

        pstrBond->iSNMPIndex = iGetInterfaceIndexFromName(pstrBond->achBondName);


        /* now parse the members of the bond */
        /* first line before each device entry is blank; */
        /* if no interfaces associated with this bond,   */
        /* we will not enter the below while loop.       */
        while (fgets(achTemp, TEMP_LEN, phBondInfo) != NULL) {
            CLOBBER_NL(achTemp);

            /* ignore next three or four un-used lines depending on the */
            /* version of the bonding driver we have...                 */
            if ((strstr(achTemp, "MII Polling Interval") != (char*)0) ||
                    (strstr(achTemp, "Up Delay") != (char*)0) ||
                    (strstr(achTemp, "Down Delay") != (char*)0) ||
                    (strstr(achTemp, "Multicast Mode") != (char*)0)) {
                continue;
            }

            pstrBondDevice = (BondDevice*) malloc(sizeof(BondDevice));
            if (pstrBondDevice == (BondDevice*)0) {
                if (iLogError > 0) {
                    fprintf(stderr, 
                            "ERROR: cpqnic can't allocated memory\n");
                    iLogError--;
                }
                fclose(phBondInfo);
                vFreeBondInterfaceList(&pstrBondHead);
                return((BondInterface*)0);
            }
            pstrBondDevice->pstrNext = (BondDevice*)0;

            if (pstrBond->pstrBondDevice == (BondDevice*)0) {
                /* first entry on this device list */
                pstrBond->pstrBondDevice = pstrBondDevice;
            } else {
                /* link device to existing device list */
                pstrLastBondDevice->pstrNext = pstrBondDevice;
            }
            pstrLastBondDevice = pstrBondDevice;

            /***************************/
            /* "Slave Interface: eth1" */
            /***************************/
            fgets(achTemp, TEMP_LEN, phBondInfo);
            CLOBBER_NL(achTemp);
            pchTemp = strrchr(achTemp, ':');
            if ((pchTemp == (char*)0) || (strlen(pchTemp) < 3)) {
                if (iLogError > 0) {
                    fprintf(stderr, 
                            "ERROR: cpqnic could not parse token from "
                            "\"%s\"\n",
                            achTemp);
                    fprintf(stderr, 
                            "       ensure you are using a supported bonding "
                            "driver\n");
                    iLogError--;
                }
                fclose(phBondInfo);
                vFreeBondInterfaceList(&pstrBondHead);
                return((BondInterface*)0);
            }
            pchTemp += 2;
            strncpy(pstrBondDevice->achInterfaceName, pchTemp, MAX_BOND_IFACE_LEN);
            pstrBondDevice->achInterfaceName[MAX_BOND_IFACE_LEN-1] = '\0';

            /********************/
            /* "MII Status: up" */
            /********************/
            fgets(achTemp, TEMP_LEN, phBondInfo);
            CLOBBER_NL(achTemp);
            pchTemp = strrchr(achTemp, ':');
            if ((pchTemp == (char*)0) || (strlen(pchTemp) < 3)) {
                if (iLogError > 0) {
                    fprintf(stderr, 
                            "ERROR: cpqnic could not parse token from \"%s\", "
                            "%s,%d\n",
                            achTemp);
                    fprintf(stderr, 
                            "       ensure you are using a supported bonding "
                            "driver\n");
                    iLogError--;
                }
                fclose(phBondInfo);
                vFreeBondInterfaceList(&pstrBondHead);
                return((BondInterface*)0);
            }
            pchTemp += 2;

            if (strstr(pchTemp, "up") == (char*)0) {
                pstrBondDevice->iMIIStatus = MII_STATUS_DOWN;
            } else {
                pstrBondDevice->iMIIStatus = MII_STATUS_UP;
                pstrBond->iLinkOKCount++;
            }

            /******************************/
            /* "Link Failure Count: 4513" */
            /******************************/
            fgets(achTemp, TEMP_LEN, phBondInfo);
            CLOBBER_NL(achTemp);
            pchTemp = strrchr(achTemp, ':');
            if ((pchTemp == (char*)0) || (strlen(pchTemp) < 3)) {
                if (iLogError > 0) {
                    fprintf(stderr, 
                            "ERROR: cpqnic could not parse token from \"%s\", "
                            "%s,%d\n",
                            achTemp);
                    fprintf(stderr, 
                            "       ensure you are using a supported bonding "
                            "driver\n");
                    iLogError--;
                }
                fclose(phBondInfo);
                vFreeBondInterfaceList(&pstrBondHead);
                return((BondInterface*)0);
            }
            pchTemp += 2;

            pstrBondDevice->iLinkFailureCount = atoi(pchTemp);
            pstrBond->iLinkFailureCount += pstrBondDevice->iLinkFailureCount;
            pstrBond->iNumInterfaces++;

            pstrBondDevice->iSNMPIndex = 
                iGetInterfaceIndexFromName(pstrBondDevice->achInterfaceName);
        }

        fclose(phBondInfo);
    }

#if NIC_DEBUG 
    if (pstrBondHead == (BondInterface*)0) {
        fprintf(stderr, "cmanic: No bonds found\n");
    }
#endif /* NIC_DEBUG  */


    return(pstrBondHead);
}


/******************************************************************************
 *   Description: Build a list that represents the current bonding        
 *                interfaces; returned struct must be freed by calling    
 *                function.                                               
 *   Parameters : none                                                    
 *   Returns    : pointer to BondInterface structure                      
 ******************************************************************************/
BondInterface* pstrBuildBondInterfaceList(void)
{
    FILE* phPipe;
    FILE* phBondInfo;
    char* pchTemp;
    BondInterface* pstrBondHead = (BondInterface*)0;
    BondInterface* pstrBondTail;
    BondInterface* pstrBond;
    BondDevice* pstrBondDevice;
    BondDevice* pstrLastBondDevice;
    static int iLogError = 5;   /* log the first 5 errors */
    char           achTemp[TEMP_LEN];  

#if NIC_DEBUG 
    fprintf(stderr, "cmanic: checking for bonds\n");
#endif /* NIC_DEBUG  */

    /* if the 802.3ad bonding driver is not running, check for the old */
    /* bonding driver                                                  */
    sprintf(achTemp, "ls -d1 /proc/net/bonding/* 2>/dev/null");
    phPipe = popen(achTemp, "r");
    if (phPipe == (FILE *)NULL) {
        /* no bonds found */
        /* see if the old (non-802.3ad) bonding driver is running */
        return(pstrBuildOldBondInterfaceList());
    }

    /* for each bond/team create a structure */
    while (fgets(achTemp, TEMP_LEN, phPipe) != NULL) {
        CLOBBER_NL(achTemp);
        /* allocate struct */
        pstrBond = (BondInterface*) malloc(sizeof(BondInterface));

        if (pstrBond == (BondInterface*)0) {
            vFreeBondInterfaceList(&pstrBondHead);
            if (iLogError > 0) {
                fprintf(stderr, 
                        "ERROR: cpqnic can't allocated memory\n");
                iLogError--;
            }
            return((BondInterface*)0);
        }

        pstrBond->iLinkFailureCount = 0;
        pstrBond->iNumInterfaces = 0;
        pstrBond->iLinkOKCount = 0;
        pstrBond->uiMapSpeedMbps = 0;
        pstrBond->pstrBondDevice = (BondDevice*)0;
        pstrBond->pstrNext = (BondInterface*)0;
        strcpy(pstrBond->achActiveInterface, "none");

        /* TODO: change achBondDir to achBondFile when we drop the */
        /*       old bonding driver support (SS7.0 and later)      */
        strncpy(pstrBond->achBondDir, achTemp, MAX_BOND_DIR_LEN);
        pstrBond->achBondDir[MAX_BOND_DIR_LEN-1] = '\0';

        /* bond name is file name, i.e. bond0 */
        pchTemp = strrchr(pstrBond->achBondDir, '/');

        if (pchTemp == (char*)0) {
            /* something is screwed up */
            if (iLogError > 0) {
                fprintf(stderr, 
                        "cmanic: bond name not found in \"%s\"\n", 
                        pstrBond->achBondDir);
                iLogError--;
            }
#if NIC_DEBUG 
            fprintf(stderr, 
                    "cmanic: bond interface will not be "
                    "added agent list\n");
#endif /* NIC_DEBUG  */
            free(pstrBond);
            pstrBond = (BondInterface*)0;
            continue;
        }

        pchTemp += 1;
        strcpy(pstrBond->achBondName, pchTemp);

        /* link the struct to our list */
        if (pstrBondHead == (BondInterface*)0) {
            /* first time through */
            pstrBondHead = pstrBond;
            pstrBondTail = pstrBond;
        } else {
            pstrBondTail->pstrNext = pstrBond;
            pstrBondTail = pstrBond;
        }
    }
    plcose(phPipe);


    /* read contents of each bondX file */
    for (pstrBond = pstrBondHead;
            pstrBond != (BondInterface*)0;
            pstrBond = pstrBond->pstrNext) {
        sprintf(achTemp, "%s", pstrBond->achBondDir);
        phBondInfo = fopen(achTemp, "r");
        if (phBondInfo == NULL) {
            if (iLogError > 0) {
                fprintf(stderr, "ERROR: cpqnic could not open %s\n",
                        achTemp);
                iLogError--;
            }
            vFreeBondInterfaceList(&pstrBondHead);
            return((BondInterface*)0);
        }

#if NIC_DEBUG 
        fprintf(stderr, "parse %s\n", 
                pstrBond->achBondName);
#endif /* NIC_DEBUG  */

        /**********************************************************************/
        /* the /proc/net/bonding/bondX file looks something like this         */
        /*                                                                    */
        /*      Ethernet Channel Bonding Driver: v2.5.0 (December 1, 2003)    */
        /*      Bonding Mode: load balancing (round-robin)                    */
        /*      Currently Active Slave: eth0 <<<<< only in NFT, TLB, & ALB    */
        /*      MII Status: up                                                */
        /*      MII Polling Interval (ms): 1000                               */
        /*      Up Delay (ms): 0                                              */
        /*      Down Delay (ms): 0                                            */
        /*                                                                    */
        /*      802.3ad info                                                  */
        /*      Active Aggregator Info:                                       */
        /*           Aggregator ID: 1                                         */
        /*           Number of ports: 2                                       */
        /*           Actor Key: 2                                             */
        /*           Partner Key: 3                                           */
        /*           Partner Mac Address: 00:11:22:33:44:55                   */
        /*                                                                    */
        /*      Slave Interface: eth1                                         */
        /*      MII Status: down                                              */
        /*      Link Failure Count: 4513                                      */
        /*      Permanent HW addr: 00:11:22:33:44:55                          */
        /*      Aggregator ID: N/A                                            */
        /*                                                                    */
        /*      Slave Interface: eth0                                         */
        /*      MII Status: up                                                */
        /*      Link Failure Count: 2                                         */
        /*      Permanent HW addr: 00:11:22:33:44:55                          */
        /*      Aggregator ID: N/A                                            */
        /**********************************************************************/

        /* read content of file into struct */

        while (fgets(achTemp, TEMP_LEN, phBondInfo) != NULL) {
            CLOBBER_NL(achTemp);

            if (strstr(achTemp, "Bonding Mode:") != (char*)0) {
                if (strstr(achTemp, "(round-robin") != (char*)0) {
                    pstrBond->iBondingMode = ROUND_ROBIN_BOND;
                }
                else if (strstr(achTemp, "(xor") != (char*)0) {
                    /* catches (xor) and (xor-ip) */
                    pstrBond->iBondingMode = XOR_BOND;
                }
                else if (strstr(achTemp, "(active-backup") != (char*)0) {
                    pstrBond->iBondingMode = ACTIVE_BACKUP_BOND;
                }
                else if (strstr(achTemp, "(broadcast") != (char*)0) {
                    pstrBond->iBondingMode = BROADCAST_BOND;
                }
                else if (strstr(achTemp, "IEEE 802.3ad") != (char*)0) {
                    pstrBond->iBondingMode = IEEE_8023AD_BOND;
                }
                else if (strstr(achTemp, "transmit load balancing") != (char*)0) {
                    pstrBond->iBondingMode = TLB_BOND;
                }
                else if (strstr(achTemp, "adaptive load balancing") != (char*)0) {
                    pstrBond->iBondingMode = ALB_BOND;
                } else {
                    pstrBond->iBondingMode = UNKNOWN_BOND;
                    if (iLogError > 0) {
                        fprintf(stderr, 
                                "ERROR: cpqnic could not parse bonding mode "
                                "token from \"%s\"\n",
                                achTemp);
                        fprintf(stderr, 
                                "       ensure you are using a supported bonding "
                                "driver\n");
                        iLogError--;
                    }
                }
                pstrBond->iSNMPIndex = 
                    iGetInterfaceIndexFromName(pstrBond->achBondName);

#if NIC_DEBUG 
                fprintf(stderr, 
                        "cmanic: bonding mode = %d\n", 
                        pstrBond->iBondingMode);
#endif /* NIC_DEBUG  */

                /* parse the bond information */
                while (fgets(achTemp, TEMP_LEN, phBondInfo) != NULL) {
                    CLOBBER_NL(achTemp);

                    if (strstr(achTemp, "Currently Active Slave:") != (char*)0) {
                        pchTemp = strrchr(achTemp, ':');
                        if ((pchTemp == (char*)0) || (strlen(pchTemp) < 3)) {
                            if (iLogError > 0) {
                                fprintf(stderr, 
                                        "ERROR: cpqnic could not parse token "
                                        "from \"%s\"\n",
                                        achTemp);
                                fprintf(stderr, 
                                        "       ensure you are using a supported "
                                        "bonding driver\n");
                                iLogError--;
                            }
                            fclose(phBondInfo);
                            vFreeBondInterfaceList(&pstrBondHead);
                            return((BondInterface*)0);
                        }

                        pchTemp += 2;
                        strncpy(pstrBond->achActiveInterface, pchTemp, MAX_BOND_IFACE_LEN);
                        pstrBond->achActiveInterface[MAX_BOND_IFACE_LEN-1] = '\0';
#if NIC_DEBUG 
                        fprintf(stderr, "Active slave = %s\n", 
                                pstrBond->achActiveInterface);
#endif /* NIC_DEBUG  */
                        continue;
                    }

                    if (strstr(achTemp, "MII Status:") != (char*)0) {
                        /* get bond status "MII Status: up" */
                        if (strstr(achTemp, "up") == (char*)0) {
                            pstrBond->iMIIStatus = MII_STATUS_DOWN;
                        } else {
                            pstrBond->iMIIStatus = MII_STATUS_UP;
                        }
#if NIC_DEBUG 
                        fprintf(stderr, "MIIStatus = %d\n", 
                                pstrBond->iMIIStatus);
#endif /* NIC_DEBUG  */
                        continue;
                    }

                    if (strstr(achTemp, "Slave Interface:") != (char*)0) {
                        break; /* time to parse slave info */
                    }
#if NIC_DEBUG 
                    fprintf(stderr, "ignore line = \"%s\"\n", 
                            achTemp);
#endif /* NIC_DEBUG  */
                }
            }


            /* if no interfaces associated with this bond,   */
            /* we will not enter the below while loop.       */
            while (strstr(achTemp, "Slave Interface:") != (char*)0) {
                /* parse the members of the bond */
                pstrBondDevice = (BondDevice*) malloc(sizeof(BondDevice));
                if (pstrBondDevice == (BondDevice*)0) {
                    if (iLogError > 0) {
                        fprintf(stderr, 
                                "ERROR: cpqnic can't allocated memory\n");
                        iLogError--;
                    }
                    fclose(phBondInfo);
                    vFreeBondInterfaceList(&pstrBondHead);
                    return((BondInterface*)0);
                }
                pstrBondDevice->iMIIStatus = MII_STATUS_DOWN;
                pstrBondDevice->iLinkFailureCount = 0;
                pstrBondDevice->pstrNext = (BondDevice*)0;

                if (pstrBond->pstrBondDevice == (BondDevice*)0) {
                    /* first entry on this device list */
                    pstrBond->pstrBondDevice = pstrBondDevice;
                } else {
                    /* link device to existing device list */
                    pstrLastBondDevice->pstrNext = pstrBondDevice;
                }
                pstrLastBondDevice = pstrBondDevice;

                /*********************************/
                /* parse "Slave Interface: eth1" */
                /*********************************/
                pchTemp = strrchr(achTemp, ':');
                if ((pchTemp == (char*)0) || (strlen(pchTemp) < 3)) {
                    if (iLogError > 0) {
                        fprintf(stderr, 
                                "ERROR: cpqnic could not parse token from "
                                "\"%s\"\n",
                                achTemp);
                        fprintf(stderr, 
                                "       ensure you are using a supported bonding "
                                "driver\n");
                        iLogError--;
                    }
                    fclose(phBondInfo);
                    vFreeBondInterfaceList(&pstrBondHead);
                    return((BondInterface*)0);
                }
                pchTemp += 2;
                strncpy(pstrBondDevice->achInterfaceName, pchTemp, 
                        MAX_BOND_IFACE_LEN);
                pstrBondDevice->achInterfaceName[MAX_BOND_IFACE_LEN-1] = '\0';

                pstrBond->iNumInterfaces++;
                pstrBondDevice->iSNMPIndex = 
                    iGetInterfaceIndexFromName(pstrBondDevice->achInterfaceName);

                /* parse the slave information */
                while (fgets(achTemp, TEMP_LEN, phBondInfo) != NULL) {
                    CLOBBER_NL(achTemp);
                    if (strstr(achTemp, "Slave Interface:") != (char*)0) {
                        break; /* a new slave to parse */
                    }

                    /********************/
                    /* "MII Status: up" */
                    /********************/
                    if (strstr(achTemp, "MII Status:") != (char*)0) {
                        if (strstr(achTemp, "up") == (char*)0) {
                            pstrBondDevice->iMIIStatus = MII_STATUS_DOWN;
                        } else {
                            pstrBondDevice->iMIIStatus = MII_STATUS_UP;
                            pstrBond->iLinkOKCount++;
                        }
                        continue;
                    }

                    /******************************/
                    /* "Link Failure Count: 4513" */
                    /******************************/
                    if (strstr(achTemp, "Link Failure Count:") != (char*)0) {
                        pchTemp = strrchr(achTemp, ':');
                        if ((pchTemp == (char*)0) || (strlen(pchTemp) < 3)) {
                            if (iLogError > 0) {
                                fprintf(stderr, 
                                        "ERROR: cpqnic could not parse token from "
                                        "\"%s\"\n",
                                        achTemp);
                                fprintf(stderr, 
                                        "       ensure you are using a supported "
                                        "bonding driver\n");
                                iLogError--;
                            }
                            fclose(phBondInfo);
                            vFreeBondInterfaceList(&pstrBondHead);
                            return((BondInterface*)0);
                        }
                        pchTemp += 2;
                        pstrBondDevice->iLinkFailureCount = atoi(pchTemp);
                        pstrBond->iLinkFailureCount += 
                            pstrBondDevice->iLinkFailureCount;
                        continue;
                    }
#if NIC_DEBUG 
                    fprintf(stderr, "ignore line = \"%s\"\n", 
                            achTemp);
#endif /* NIC_DEBUG  */
                }
            }
        }

        fclose(phBondInfo);
    }

#if NIC_DEBUG 
    if (pstrBondHead == (BondInterface*)0) {
        fprintf(stderr, "cmanic: No bonds found\n");
    }
#endif /* NIC_DEBUG  */


    return(pstrBondHead);
}


/******************************************************************************
 *   Description: Update the cpqNic_t structure to reflect any teams/bonds
 *                that are currntly running...                            
 *   Parameters : 1) pointer to cpqNic_t                                  
 *   Returns    : nothing                                                 
 ******************************************************************************/
void vPopulateBondingMibVariables(cpqNic_t* pstrNic)
{
    int iLoop;
    int iLoop2;
    int iLoop3;
    int iIndex;
    BondInterface* pstrBond;
    BondInterface* pstrBondHead;
    BondDevice*    pstrBondDevice;
    BondDevice*    pstrBondDevice2;
    cpqNicIfPhysical_t* pstrIfPhys;
    cpqNicIfLogical_t*  pstrIfLog;
    static int iLogError = 1;


    /* load bonding info struct */
    pstrBondHead = pstrBuildBondInterfaceList();
    if (pstrBondHead == (BondInterface*)0) {
        return;
    }

#if NIC_DEBUG 
    vPrintBondInterfaceList(pstrBondHead);
#endif /* NIC_DEBUG */

    /*******************************/
    /* adjust physical mib entries */
    /*******************************/
    /* NOTE: the number of physical device entries will not change.           */
    /* scan interface list for match to populate cpqNicIfPhysAdapterIfNumber, */
    /* cpqNicIfPhysAdapterRole, and cpqNicIfPhysAdapterState.                 */
    for (iLoop=0;
            iLoop<pstrNic->NumPhysNics;
            iLoop++) {
        pstrIfPhys = &pstrNic->pIfPhys[iLoop];

        for (pstrBond = pstrBondHead;
                pstrBond != (BondInterface*)0;
                pstrBond = pstrBond->pstrNext) {

            for (pstrBondDevice = pstrBond->pstrBondDevice;
                    pstrBondDevice != (BondDevice*)0;
                    pstrBondDevice = pstrBondDevice->pstrNext) {
                if (strcmp(pstrBondDevice->achInterfaceName,
                            pstrIfPhys->InterfaceName) == 0) {
#if NIC_DEBUG 
                    fprintf(stderr, 
                            "cmanic: %s is a member of %s (%d)\n", 
                            pstrIfPhys->InterfaceName, 
                            pstrBond->achBondName, 
                            pstrBond->iSNMPIndex);
#endif /* NIC_DEBUG */

                    /* since this device is part of a team the ethX */
                    /* interface is no longer available for         */
                    /* communication purposes. so, just clobber the */
                    /* ethX number with the bondX number.           */
                    iIndex = pstrBond->iSNMPIndex;
                    pstrIfPhys->AdapterIfNumber[0] = (char)(iIndex & 0xff);
                    pstrIfPhys->AdapterIfNumber[1] = (char)((iIndex>>8)  & 0xff);
                    pstrIfPhys->AdapterIfNumber[2] = (char)((iIndex>>16) & 0xff);
                    pstrIfPhys->AdapterIfNumber[3] = (char)((iIndex>>24) & 0xff);
                    pstrIfPhys->AdapterIfNumber[4] = 0;
                    pstrIfPhys->AdapterIfNumber[5] = 0;
                    pstrIfPhys->AdapterIfNumber[6] = 0;
                    pstrIfPhys->AdapterIfNumber[7] = 0;

                    switch (pstrBond->iBondingMode) {
                        case ACTIVE_BACKUP_BOND:

                        case TLB_BOND:

                        case ALB_BOND:
                            if (strcmp(pstrBond->achActiveInterface,
                                        pstrIfPhys->InterfaceName) == 0) {
                                pstrIfPhys->AdapterRole = ADAPTER_ROLE_PRIMARY;
                            } else {
                                pstrIfPhys->AdapterRole = ADAPTER_ROLE_SECONDARY;
                                /* the AdapterState is correct for all conditions */
                                /* teamed or un-teamed except for this one...     */
                                /* make it correct.                               */
                                if (pstrIfPhys->AdapterStatus == STATUS_OK) {
                                    pstrIfPhys->AdapterState = ADAPTER_STATE_STANDBY;
                                }
                            }
                            break;

                        case ROUND_ROBIN_BOND:

                        case XOR_BOND:

                        case BROADCAST_BOND:

                        case IEEE_8023AD_BOND:
                            pstrIfPhys->AdapterRole = ADAPTER_ROLE_MEMBER;
                            break;

                        case UNKNOWN_BOND:
#if NIC_DEBUG 
                            fprintf(stderr, 
                                    "cmanic: Unknown bonding mode (%d)\n", 
                                    pstrBond->iBondingMode);
#endif /* NIC_DEBUG */
                            pstrIfPhys->AdapterRole = ADAPTER_ROLE_UNKNOWN;
                            break;

                        default:
                            if (iLogError) {
                                fprintf(stderr, 
                                        "ERROR: cmanic, unknown bonding mode "
                                        "%d for %s\n", 
                                        pstrBond->iBondingMode, 
                                        pstrBond->achBondName);
                                iLogError = 0;
                            }
                            break;
                    }

                    /* force outer for loop exit for performance reasons */
                    pstrBond = (BondInterface*)0;
                    break;
                }
            }

            /* for loop exit was forced */
            if (pstrBond == (BondInterface*)0) {
                break;
            }
        }
    }


    /******************************/
    /* adjust logical mib entries */
    /******************************/
    /* NOTE: the number of logical interface entries will not increase,   */
    /*       but may decrease.                                            */
    /* dont assume loopback interface is the first interface              */
    /* YES, we could have done this in the above loop since there is a    */
    /* 1-to-1 mapping at this piont between the logical and physical      */
    /* entries... this adds a bit of performance over-head but I hope     */
    /* it will make things a bit clearer and easier to read at the        */
    /* expense of a little performance hit...                             */
    for (iLoop=0;
            iLoop<pstrNic->NumLogNics;
            iLoop++) {
        pstrIfLog = &pstrNic->pIfLog[iLoop];

        for (pstrBond = pstrBondHead;
                pstrBond != (BondInterface*)0;
                pstrBond = pstrBond->pstrNext) {

            for (pstrBondDevice = pstrBond->pstrBondDevice;
                    pstrBondDevice != (BondDevice*)0;
                    pstrBondDevice = pstrBondDevice->pstrNext) {
                if (strcmp(pstrBondDevice->achInterfaceName,
                            pstrIfLog->InterfaceName) == 0) {
#if NIC_DEBUG 
                    fprintf(stderr, 
                            "cmanic: %s is a LOGICAL member of %s (%d)\n", 
                            pstrIfLog->InterfaceName, 
                            pstrBond->achBondName, 
                            pstrBond->iSNMPIndex);
#endif /* NIC_DEBUG */
                    /* if not an NFT team, take the highest speed of all */
                    /* available NICs                                    */
                    if ((pstrBond->iBondingMode != ACTIVE_BACKUP_BOND) &&
                            (pstrBondDevice->iMIIStatus == MII_STATUS_UP)  &&
                            (pstrBond->uiMapSpeedMbps < pstrIfLog->MapSpeedMbps)) {
                        pstrBond->uiMapSpeedMbps = pstrIfLog->MapSpeedMbps;
                    }
                    else if ((pstrBond->iBondingMode == ACTIVE_BACKUP_BOND) &&
                            (strcmp(pstrBond->achActiveInterface,
                                    pstrIfLog->InterfaceName) == 0)         &&
                            (pstrBondDevice->iMIIStatus == MII_STATUS_UP)) {
                        /* an NFT team so take the speed of the active interface */
                        pstrBond->uiMapSpeedMbps = pstrIfLog->MapSpeedMbps;
                    }


                    /* do the dirty work... */
                    if (pstrBondDevice != pstrBond->pstrBondDevice) {
#if NIC_DEBUG 
                        fprintf(stderr, 
                                "cmanic: removing %s from interface table\n", 
                                pstrIfLog->InterfaceName);
#endif /* NIC_DEBUG */

                        /* this is not the first interface of this bond, so   */
                        /* delete it - we only have one logical interface for */
                        /* a bond; make sure we are not on the last interface */
                        /* and try to copy from beyond it!                    */
                        if (iLoop+1 < pstrNic->NumLogNics) {
                            memmove(pstrIfLog, 
                                    &pstrNic->pIfLog[iLoop+1],
                                    sizeof(cpqNicIfLogical_t)*
                                    (pstrNic->NumLogNics-(iLoop+1)));
                            /* we just over-wrote the entry with a new one, so     */
                            /* adjust the loop counter to re-visit this shifted    */
                            /* entry                                               */
                            iLoop--;
                        }

                        /* adjust number of logical interfaces since we removed one */
                        pstrNic->NumLogNics--;
                        /* force the for loop to exit */
                        pstrBond = (BondInterface*)0;
                        break;
                    }

                    /* this is the first logical interface for this bond, so */
                    /* clobber it with the bond info...                      */
                    iIndex = pstrBond->iSNMPIndex;
                    pstrIfLog->MapIfNumber[0] = (char)(iIndex & 0xff);
                    pstrIfLog->MapIfNumber[1] = (char)((iIndex>>8)  & 0xff);
                    pstrIfLog->MapIfNumber[2] = (char)((iIndex>>16) & 0xff);
                    pstrIfLog->MapIfNumber[3] = (char)((iIndex>>24) & 0xff);
                    pstrIfLog->MapIfNumber[4] = 0;
                    pstrIfLog->MapIfNumber[5] = 0;
                    pstrIfLog->MapIfNumber[6] = 0;
                    pstrIfLog->MapIfNumber[7] = 0;

                    strcpy(pstrIfLog->InterfaceName, pstrBond->achBondName);

                    /* TODO: make iBondingMode equal MapGroupType so we can do */
                    /*       a direct copy and remove the case below - plan    */
                    /*       this when we drop the old bonding driver          */
                    /* support (SS7.0 and later)                               */
                    switch (pstrBond->iBondingMode) {
                        case ACTIVE_BACKUP_BOND:
                            pstrIfLog->MapGroupType = MAP_GROUP_TYPE_NFT;
                            pstrIfLog->MapSwitchoverMode = 
                                SWITCHOVER_MODE_SWITCH_ON_FAIL;
                            break;

                        case ROUND_ROBIN_BOND:
                        case XOR_BOND:
                        case BROADCAST_BOND:
                            pstrIfLog->MapGroupType = MAP_GROUP_TYPE_SLB;

                            /* currently, due to a lack of other values for this */
                            /* mib object, we must assign the switchover mode    */
                            /* to the "best fit" value of switch on fail.        */
                            pstrIfLog->MapSwitchoverMode = 
                                SWITCHOVER_MODE_SWITCH_ON_FAIL;
                            break;

                        case IEEE_8023AD_BOND:
                            pstrIfLog->MapGroupType = MAP_GROUP_TYPE_AD;
                            pstrIfLog->MapSwitchoverMode = 
                                SWITCHOVER_MODE_SWITCH_ON_FAIL;
                            break;

                        case TLB_BOND:
                            pstrIfLog->MapGroupType = MAP_GROUP_TYPE_TLB;
                            pstrIfLog->MapSwitchoverMode = 
                                SWITCHOVER_MODE_SWITCH_ON_FAIL;
                            break;

                        case ALB_BOND:
                            pstrIfLog->MapGroupType = MAP_GROUP_TYPE_ALB;
                            pstrIfLog->MapSwitchoverMode = 
                                SWITCHOVER_MODE_SWITCH_ON_FAIL;
                            break;

                        case UNKNOWN_BOND:
                            pstrIfLog->MapGroupType = MAP_GROUP_TYPE_NOT_IDENTIFIABLE;
                            pstrIfLog->MapSwitchoverMode = 
                                SWITCHOVER_MODE_UNKNOWN;
                            break;

                        default:
                            if (iLogError) {
                                fprintf(stderr, 
                                        "ERROR: cmanic, unknown bonding mode "
                                        "%d for %s\n", 
                                        pstrBond->iBondingMode, 
                                        pstrBond->achBondName);
                                iLogError = 0;
                            }
                            break;
                    }

                    sprintf(pstrIfLog->MapDescription, "%s - %s", 
                            pstrBond->achBondName,
                            apchMapGroupType[pstrBond->iBondingMode]);

                    pstrIfLog->MapAdapterCount = pstrBond->iNumInterfaces;

                    /* NOTE: the iLinkOKCount may not be correct due to the   */
                    /*       delay of reporting the link being up; this       */
                    /*       becomes an issue when we detect a link status    */
                    /*       change from the drivers log messages but the     */
                    /*       bonding driver has not checked the status...     */
                    /*       this will cause the OKCount to be incorrect.     */
                    /*       to resolve this, we count the number of          */
                    /*       adapters that have link here.                    */
                    /* pstrIfLog->MapAdapterOKCount = pstrBond->iLinkOKCount; */
                    pstrIfLog->MapAdapterOKCount = 0;

                    /* for each bonded device, find the associated interface */
                    /* in the physical table                                 */
                    iLoop3 = 0;
                    for (pstrBondDevice2 = pstrBond->pstrBondDevice;
                            pstrBondDevice2 != (BondDevice*)0;
                            pstrBondDevice2 = pstrBondDevice2->pstrNext) {
                        for (iLoop2=0;
                                iLoop2<pstrNic->NumPhysNics;
                                iLoop2++) {
                            pstrIfPhys = &pstrNic->pIfPhys[iLoop2];
                            if (strcmp(pstrBondDevice2->achInterfaceName,
                                        pstrIfPhys->InterfaceName) == 0) {
                                /* entries are one based */
                                pstrIfLog->MapPhysicalAdapters[iLoop3++] = 
                                    (char)(iLoop2+1);
                                if (pstrIfPhys->AdapterCondition == 
                                        ADAPTER_CONDITION_OK) {
                                    pstrIfLog->MapAdapterOKCount++;
                                }
                                break; /* break out of inner for loop */
                            }
                        }
                    }

#if NIC_DEBUG 
                    if (iLoop3 != pstrIfLog->MapAdapterCount) {
                        fprintf(stderr, 
                                "ERROR: cmanic, interface count "
                                "%d!=%d\n", 
                                iLoop3, pstrIfLog->MapAdapterCount);
                    }
#endif /* NIC_DEBUG */


                    if (pstrBond->iNumInterfaces == pstrBond->iLinkOKCount) {
                        pstrIfLog->MapCondition = MAP_CONDITION_OK;
                        pstrIfLog->MapStatus = MAP_STATUS_OK;
                    } else if (pstrBond->iLinkOKCount == 0) {
                        pstrIfLog->MapCondition = MAP_CONDITION_FAILED;
                        pstrIfLog->MapStatus = MAP_STATUS_GROUP_FAILED;
                    } else {
                        pstrIfLog->MapCondition = MAP_CONDITION_DEGRADED;

                        /* there is not a way to force an adapter to be primary, */
                        /* the active adapter is always the primary one...       */
                        /* so if an adapter fails, it becomes the secondary.     */
                        pstrIfLog->MapStatus = MAP_STATUS_STANDBY_FAILED;
                    }

                    /* MapNumSwitchovers is the number of times a switchover  */
                    /* has occured because the primayr adapter failed...      */
                    /* currently there is no way to get this from the bonding */
                    /* driver... -1 should cause "N/A" to be displayed        */
                    /* pstrIfLog->MapNumSwitchovers = pstrBond->iLinkFailureCount;*/
                    pstrIfLog->MapNumSwitchovers = -1;

                    /* force outer for loop exit for performance reasons */
                    pstrBond = (BondInterface*)0;
                    break;
                }
            }

            /* for loop exit was forced */
            if (pstrBond == (BondInterface*)0) {
                break;
            }
        }
    }

    /* re-number the indexes since some entries may have been purged */
    for (iLoop=0;
            iLoop<pstrNic->NumLogNics;
            iLoop++) {
        pstrNic->pIfLog[iLoop].MapIndex = iLoop + 1;
    }

    /* now fill in the speed for each of the bond interfaces */
    for (pstrBond = pstrBondHead;
            pstrBond != (BondInterface*)0;
            pstrBond = pstrBond->pstrNext) {
        for (iLoop=0;
                iLoop<pstrNic->NumLogNics;
                iLoop++) {
            pstrIfLog = &pstrNic->pIfLog[iLoop];
            if (strstr(pstrIfLog->MapDescription, pstrBond->achBondName) != 
                    (char*)0) {
                pstrIfLog->MapSpeedMbps = pstrBond->uiMapSpeedMbps;
                break;
            }
        }
    }

    /* re-compute the MapOverallCondition since we now have teams */
    pstrNic->MapOverallCondition = MAP_OVERALL_CONDITION_OK;
    for (iLoop=0;
            iLoop<pstrNic->NumLogNics;
            iLoop++) {
        pstrIfLog = &pstrNic->pIfLog[iLoop];

        if (pstrIfLog->MapCondition != MAP_CONDITION_OK) {
            if (pstrIfLog->MapCondition == MAP_CONDITION_FAILED) {
                pstrNic->MapOverallCondition = MAP_OVERALL_CONDITION_FAILED;
            }
            else if ((pstrIfLog->MapCondition == MAP_CONDITION_DEGRADED) &&
                    (pstrNic->MapOverallCondition != 
                     MAP_OVERALL_CONDITION_FAILED)) {
                pstrNic->MapOverallCondition = MAP_OVERALL_CONDITION_DEGRADED;
            }
            else if ((pstrIfLog->MapCondition == MAP_CONDITION_OTHER) &&
                    (pstrNic->MapOverallCondition != 
                     MAP_OVERALL_CONDITION_FAILED) &&
                    (pstrNic->MapOverallCondition != 
                     MAP_OVERALL_CONDITION_DEGRADED)) {
                pstrNic->MapOverallCondition = MAP_OVERALL_CONDITION_OTHER;
            }
        }
    }

    /* iLogError = 1; we only log the very first error in this function... */
    /*                that is, we don't re-start logging errors after a    */
    /*                "clean" pass through this function...                */
    /* Fix for QXCR1000469574 and QXCR1000818412 
       Free pstrBondHead instead of pstrBond.
     */
    vFreeBondInterfaceList(&pstrBondHead);
}
#endif /* BONDING */

#ifdef VLANS
/******************************************************************************
 *   Description: Free list created by pstrBuildVlanRecordList function.  
 *   Parameters : pointer to pointer to VlanRecord                        
 *   Returns    : nothing                                                 
 ******************************************************************************/
void vFreeVlanRecordList(VlanRecord** ppVlanList)
{

    /* recursive free */
    if (*ppVlanList != (VlanRecord*)0) {
        vFreeVlanRecordList(&((*ppVlanList)->pstrNext));
        free(*ppVlanList);
        *ppVlanList = (VlanRecord*)0;
    }

    return;
}

/******************************************************************************
 *   Description: Print the vlan interface list and all devices           
 *   Parameters : Pointer to VlanRecord struct                            
 *   Returns    : none                                                    
 ******************************************************************************/
#if NIC_DEBUG
void vPrintVlanRecordList(VlanRecord* pstrVlan)
{

    if (pstrVlan == (VlanRecord*)0) {
        return;
    }

    fprintf(stderr, "\n");
    fprintf(stderr,
            "################################################################\n");
    fprintf(stderr,
            "Current VLAN Interfaces\n");

    while (pstrVlan != (VlanRecord*)0) {
        fprintf(stderr, "------------------------------------\n");
        fprintf(stderr, "name:      %s\n",
                pstrVlan->achDeviceName);
        fprintf(stderr, "ID:        %s\n",
                pstrVlan->achId);
        fprintf(stderr, "interface: %s\n",
                pstrVlan->achInterfaceName);

        fprintf(stderr, "\n");
        pstrVlan = pstrVlan->pstrNext;
    }

    return;
}
#endif /* NIC_DEBUG */


/******************************************************************************
 *   Description: Build a list that represents the current bonding        
 *                interfaces; returned struct must be freed by calling    
 *                function.                                               
 *   Parameters : none                                                    
 *   Returns    : pointer to VlanRecord structure                         
 ******************************************************************************/
VlanRecord* pstrBuildVlanRecordList(void)
{ 
    FILE* phVlanInfo;
    char* pchVlanId;
    char* pchInterfaceName;
    int   iMapIndex;
    VlanRecord* pstrVlanHead = (VlanRecord*)0;
    VlanRecord* pstrVlanTail;
    VlanRecord* pstrVlan;
    static int iLogError = 1;
    char        achTemp[TEMP_LEN];  


    phVlanInfo = fopen(proc_vlan_file, "r");
    if (phVlanInfo == NULL) {

#if NIC_DEBUG 
        fprintf(stderr, "cmanic: no VLANs found in %s\n", 
                proc_vlan_file);
#endif /* NIC_DEBUG */

        return((VlanRecord*)0);
    }


    /***********************************************************/
    /* the /proc/net/vlan/config file looks like this          */
    /*                                                         */             
    /*    VLAN Dev name    | VLAN ID                           */
    /*    Name-Type: VLAN_NAME_TYPE_RAW_PLUS_VID_NO_PAD        */
    /*    bond0.4092     | 4092  | bond0                       */
    /*    bond0.4093     | 4093  | bond0                       */
    /***********************************************************/

    iMapIndex = 1; /* index is one based */
    fgets(achTemp, TEMP_LEN, phVlanInfo);
    fgets(achTemp, TEMP_LEN, phVlanInfo);

    while (fgets(achTemp, TEMP_LEN, phVlanInfo) != NULL) {
        CLOBBER_NL(achTemp);

        pchVlanId = strchr(achTemp, '|');
        if (pchVlanId == (char*)0) {
            if (iLogError) {
                fprintf(stderr, 
                        "ERROR: cmanic, could not find \"|\" in \"%s\", %d,%s",
                        achTemp);
                iLogError = 0;
            }
            fclose(phVlanInfo);
            vFreeVlanRecordList(&pstrVlanHead);
            return((VlanRecord*)0);
        }

        pchInterfaceName = strchr(pchVlanId+1, '|');
        if (pchInterfaceName == (char*)0) {
            if (iLogError) {
                fprintf(stderr, 
                        "ERROR: cmanic, could not find "
                        "second \"|\" in \"%s\", %d,%s",
                        achTemp);
                iLogError = 0;
            }
            fclose(phVlanInfo);
            vFreeVlanRecordList(&pstrVlanHead);
            return((VlanRecord*)0);
        }

        /* now slice and dice */
        *pchVlanId = '\0';
        pchVlanId++;
        *pchInterfaceName = '\0';
        pchInterfaceName++;


        pstrVlan = (VlanRecord*) malloc(sizeof(VlanRecord));

        if (pstrVlan == (VlanRecord*)0) {
            if (iLogError) {
                fprintf(stderr,
                        "ERROR: cpqnic can't allocated memory\n");
                iLogError = 0;
            }
            fclose(phVlanInfo);
            vFreeVlanRecordList(&pstrVlanHead);
            return((VlanRecord*)0);
        }

        pstrVlan->iMapping = iMapIndex;
        pstrVlan->pstrNext = (VlanRecord*)0;

        sscanf(achTemp, " %s ", pstrVlan->achDeviceName);
        sscanf(pchVlanId, " %s ", pstrVlan->achId);
        sscanf(pchInterfaceName, " %s ", pstrVlan->achInterfaceName);

        pstrVlan->iSNMPIndex = 
            iGetInterfaceIndexFromName(pstrVlan->achDeviceName);

#if NIC_DEBUG 
        fprintf(stderr, 
                "cmanic: vlan read: %s, %s, %s, IfIndex=%d\n", 
                pstrVlan->achDeviceName,
                pstrVlan->achId,
                pstrVlan->achInterfaceName,
                pstrVlan->iSNMPIndex);
#endif /* NIC_DEBUG */

        /* link the struct to our list */
        if (pstrVlanHead == (VlanRecord*)0) {
            /* first time through */
            pstrVlanHead = pstrVlan;
            pstrVlanTail = pstrVlan;
        } else {
            pstrVlanTail->pstrNext = pstrVlan;
            pstrVlanTail = pstrVlan;
        }

        iMapIndex++;
    }

    iLogError = 1;
    fclose(phVlanInfo);
    return(pstrVlanHead);

} 

/******************************************************************************
 *   Description: Update the cpqNic_t structure to reflect any vlans      
 *                that are currntly configured...                         
 *   Parameters : 1) pointer to cpqNic_t                                  
 *   Returns    : nothing                                                 
 ******************************************************************************/
void vPopulateVlanMibVariables(cpqNic_t* pstrNic)
{
    int iNumVlans;
    int iLoop;
    int iLoop2;
    int iIndex;
    VlanRecord* pstrVlanHead;
    VlanRecord* pstrVlan;
    cpqNicIfVlan_t *pstrIfVlan;
    cpqNicIfLogical_t *pstrIfLog;
    dynamic_interface_info_t dyn_if_info;
    static int iLogError = 1;

    /* if vlans are not configured, do nothing */
    pstrVlanHead = pstrBuildVlanRecordList();
    if (pstrVlanHead == (VlanRecord*)0) {
        return;
    }

#if NIC_DEBUG 
    vPrintVlanRecordList(pstrVlanHead);
#endif /* NIC_DEBUG */


    /*************************************/
    /* fill in the vlan portion of the mib */
    /*************************************/

    /* get number of vlans */
    for (pstrVlan=pstrVlanHead, iNumVlans=0;
            pstrVlan != (VlanRecord*)0;
            pstrVlan = pstrVlan->pstrNext, iNumVlans++) {
    }

    pstrNic->NumVlans = iNumVlans;
    pstrNic->pIfVlan = (cpqNicIfVlan_t*) malloc(sizeof(cpqNicIfVlan_t) * 
            iNumVlans);

    for (iLoop=0, pstrVlan=pstrVlanHead;
            iLoop<pstrNic->NumVlans;
            iLoop++, pstrVlan = pstrVlan->pstrNext) {
        pstrIfVlan = &pstrNic->pIfVlan[iLoop];

        pstrIfVlan->MapIndex = iLoop+1; /* index is one based */
        pstrIfVlan->MapIfIndex = pstrVlan->iSNMPIndex;
        pstrIfVlan->MapVlanId = atoi(pstrVlan->achId);
        strcpy(pstrIfVlan->Name, pstrVlan->achDeviceName);

        /* get IPv6 address associated with this VLAN */
        if (get_dynamic_interface_info(pstrIfVlan->Name, &dyn_if_info) != -1) {
            if (dyn_if_info.pipv6_shorthand_addr != (char*)0) {
                sprintf(pstrIfVlan->IPV6Address, "%s/%d",
                        dyn_if_info.pipv6_shorthand_addr, 
                        dyn_if_info.pipv6_refix_length);
            } else {
                pstrIfVlan->IPV6Address[0] = '\0';
            }
            free_dynamic_interface_info(&dyn_if_info);
        }

        /* search for the interface index in our cpqNic */
        /* logical interface table...                   */
        pstrIfVlan->MapLogIndex = -1;
        for (iLoop2=0; iLoop2<pstrNic->NumLogNics; iLoop2++) {
            pstrIfLog = &pstrNic->pIfLog[iLoop2];
            if (strcmp(pstrIfLog->InterfaceName, pstrVlan->achInterfaceName) == 0) {
                pstrIfVlan->MapLogIndex = pstrIfLog->MapIndex;
                break;
            }
        }
    }


    /**************************************/
    /* fill in the interface vlan objects */
    /**************************************/
    for (iLoop=0;
            iLoop<pstrNic->NumLogNics;
            iLoop++) {
        pstrIfLog = &pstrNic->pIfLog[iLoop];

        for (pstrVlan=pstrVlanHead;
                pstrVlan != (VlanRecord*)0;
                pstrVlan = pstrVlan->pstrNext) {
            if (strcmp(pstrIfLog->InterfaceName, pstrVlan->achInterfaceName) == 0) {
                /* we found a vlan associated with this interface */
                pstrIfLog->MapVlans[pstrIfLog->VlanCount] = pstrVlan->iMapping;
                pstrIfLog->VlanCount++;
                pstrIfLog->MapGroupType = MAP_GROUP_TYPE_NFT;
                sprintf(pstrIfLog->MapDescription, "HP VLan Interface(s)");

                /**********************************************************/
                /* now, add the vlan index to the IfLogMapIfNumber table; */
                /* find the first empty position                          */
                /**********************************************************/
                for (iLoop2=0; 
                        iLoop2 < (OCTET_STRING_LEN/4);
                        iLoop2+=4) {
                    if ((pstrIfLog->MapIfNumber[iLoop2] == 0) &&
                            (pstrIfLog->MapIfNumber[1+iLoop2] == 0) &&
                            (pstrIfLog->MapIfNumber[2+iLoop2] == 0) &&
                            (pstrIfLog->MapIfNumber[3+iLoop2] == 0)) {
                        /* we found an empty position */
                        iIndex = pstrVlan->iSNMPIndex;
                        pstrIfLog->MapIfNumber[iLoop2] = 
                            (char)(iIndex & 0xff);
                        pstrIfLog->MapIfNumber[iLoop2+1] = 
                            (char)((iIndex>>8)  & 0xff);
                        pstrIfLog->MapIfNumber[iLoop2+2] = 
                            (char)((iIndex>>16) & 0xff);
                        pstrIfLog->MapIfNumber[iLoop2+3] = 
                            (char)((iIndex>>24) & 0xff);
                        pstrIfLog->MapIfNumber[iLoop2+4] = 0;
                        pstrIfLog->MapIfNumber[iLoop2+5] = 0;
                        pstrIfLog->MapIfNumber[iLoop2+6] = 0;
                        pstrIfLog->MapIfNumber[iLoop2+7] = 0;
                        break;
                    }
                }

                if (iLoop2 == (OCTET_STRING_LEN/4)) {
                    if (iLogError) {
                        fprintf(stderr,
                                "ERROR: cpqnic can't find free table "
                                "entry in %s\n",
                                pstrIfLog->InterfaceName);
                        iLogError = 0;
                    }
                }
            }
        }
    }

    vFreeVlanRecordList(&pstrVlanHead);

}
#endif /* VLANS */

/******************************************************************************
 *   Description: Load the complete cpqNic_t structure; this is the       
 *                caller's main entry point...                            
 *   Parameters : 1) void                                                 
 *                                                                        
 *               *The lock_mibvar_mutex must be called before this function|
 *                so the struct won't change while being accessed, and the
 *                unlock_mibvar_mutex is called after the last structure  
 *                access.                                                 
 *   Returns    : a pointer to the cpqNic_t structure                     
 ******************************************************************************/
cpqNic_t * pstrGetNicMibVariables()
{
    int i;
    int iResult;
    int iPhysIndex;
    int iLogIndex;
    int iPhysSlot;
    int do_remap = 0;
    int aiPortCount[MAX_PHYSICAL_SLOTS];  /* number of ports for a given slot */
    cpqNicIfPhysical_t *pIfPhys;
    cpqNicIfLogical_t *pIfLog;
    static int iNicProcFileCount = -1;
    static NicPort *pNicPorts = (NicPort*)0;
    static int iRemapNeeded = 1;
    NicPort *pstrNicPort;

    //if (time(NULL) >= iTimeToRecache)
    //{

    /* time to re-cache */
    iTimeToRecache = time(NULL) + NicMibVars.OsCommonPollFreq;
    iResult = iGetNicProcFileCount();
    /*fprintf(stderr,"iResult=%d, iNicProcFileCount=%d, iRemapNeeded=%d\n",
      iResult,iNicProcFileCount,iRemapNeeded); */
    if ((iNicProcFileCount != iResult) || iRemapNeeded) {
        do_remap = 1;
        iRemapNeeded = 0;
        /* we don't use count inside nicmibvar struct because we may */
        /* have skipped unsupported controlers and they will not     */
        /* be in the nicmibvar count...                              */
        iNicProcFileCount = iResult;  

        /* something has changed out from under us, free all structs */
        vFreeNicPorts(&pNicPorts);

        free(NicMibVars.pIfPhys);
        NicMibVars.pIfPhys = (cpqNicIfPhysical_t*)0;
        free(NicMibVars.pIfLog);
        NicMibVars.pIfLog = (cpqNicIfLogical_t*)0;
        free(NicMibVars.pIfVlan);
        NicMibVars.pIfVlan = (cpqNicIfVlan_t*)0;

        NicMibVars.NumLogNics = 0;
        NicMibVars.NumPhysNics = 0;
        NicMibVars.NumVlans = 0;

        /* reload map files */
        iResult = iGetNicProcFiles(&pNicPorts);
        if (iResult < 0) {
            /* out of memory! */
            iNicProcFileCount = -1; /* so we reload next time */
            return(&NicMibVars);
        }


        NicMibVars.NumPhysNics = iResult;
        NicMibVars.NumLogNics = iResult + 1; /* +1 for loopback */

        /* alloc new logical and physical structs */
        if (NicMibVars.NumPhysNics > 0) {
            NicMibVars.pIfPhys = (cpqNicIfPhysical_t*) 
                malloc(sizeof(cpqNicIfPhysical_t)*
                        NicMibVars.NumPhysNics);
            if (NicMibVars.pIfPhys == (cpqNicIfPhysical_t*)0) {
                /* out of memory */
                vFreeNicPorts(&pNicPorts);
                iNicProcFileCount = -1; /* so we reload next time */
                NicMibVars.NumLogNics = 0;
                NicMibVars.NumPhysNics = 0;
                return(&NicMibVars);
            }
            memset(NicMibVars.pIfPhys, 0, sizeof(cpqNicIfPhysical_t)*
                    NicMibVars.NumPhysNics);
        }

        if (NicMibVars.NumLogNics > 0) {
            /* yes, the above will always be true; just have check in */
            /* case things change later...                            */
            NicMibVars.pIfLog  = (cpqNicIfLogical_t*) 
                malloc(sizeof(cpqNicIfLogical_t)*
                        NicMibVars.NumLogNics);
            if (NicMibVars.pIfLog == (cpqNicIfLogical_t*)0) {
                /* out of memory */
                vFreeNicPorts(&pNicPorts);
                free(NicMibVars.pIfPhys);
                NicMibVars.pIfPhys = (cpqNicIfPhysical_t*)0;
                iNicProcFileCount = -1; /* so we reload next time */
                NicMibVars.NumLogNics = 0;
                NicMibVars.NumPhysNics = 0;
                return(&NicMibVars);
            }
            memset(NicMibVars.pIfLog, 0, sizeof(cpqNicIfLogical_t)*
                    NicMibVars.NumLogNics);
        }

    }
    /* NOTE: when we do teaming, this logic will change.... */
    iPhysIndex = 0;
    iLogIndex = 0;

    /* load loopback logical struct first, this is the way CIM wants it */
    vLoadLogLoopback(&NicMibVars.pIfLog[iLogIndex], 
            iLogIndex+1, iLogIndex+1);
    iLogIndex++;

    memset(aiPortCount, 0, (MAX_PHYSICAL_SLOTS*sizeof(int)));

    NicMibVars.MapOverallCondition = MAP_OVERALL_CONDITION_OK;

    pstrNicPort = pNicPorts;
    while (pstrNicPort != (NicPort*)0) {
        /* for each valid procfs file reload phys device info from procfs */
        if (pstrNicPort->iProcfsStatus == PROCFS_OK) {
            pIfPhys = &NicMibVars.pIfPhys[iPhysIndex];
            pIfLog = &NicMibVars.pIfLog[iLogIndex];

            if (iLoadPhysStruct(do_remap, pstrNicPort, pIfPhys) < 0) {
                /* could not load phys info, so skip this entry.... */
                pstrNicPort = pstrNicPort->next; 
                fprintf(stderr,"iLoadPhysStruct Failed\n");
                continue;
            }
            /* fill in AdpaterIndex here to fix rQm 211783     */
            pIfPhys->AdapterIndex = iPhysIndex + 1;

            if ((pIfPhys->PCI_Bus >= 0) && (pIfPhys->PCI_Slot >=0)) {
                iPhysSlot = iGetSlotNumber(pIfPhys->PCI_Bus, 
                        pIfPhys->PCI_Slot);

                if (iPhysSlot >= 0) {
                    /* we have a good physical slot number */
                    pIfPhys->AdapterSlot = iPhysSlot;

                    /* now determine the port number for the given slot */
                    if (iPhysSlot < MAX_PHYSICAL_SLOTS) {
                        /* port is one based, so inc then assign */
                        aiPortCount[iPhysSlot]++;
                        if (pIfPhys->AdapterPort == -1) {
                            pIfPhys->AdapterPort = aiPortCount[iPhysSlot];

                        }
                        if (aiPortCount[iPhysSlot] > 1) {
                            /* sort the ports based on the PCI_Slot */
                            /* rQm 235108                           */
                            int iTemp;
                            cpqNicIfPhysical_t *pIfPhysTemp;

                            for (pIfPhysTemp = NicMibVars.pIfPhys;
                                    pIfPhysTemp != pIfPhys;
                                    pIfPhysTemp++) {
                                if (pIfPhysTemp->AdapterSlot == 
                                        pIfPhys->AdapterSlot) {
                                    /* this port is in the same slot 
                                       we just added */
#if NIC_DEBUG 
                                    fprintf(stderr,
                                            "found multiple ports in slot "
                                            "%d\n", 
                                            pIfPhysTemp->AdapterSlot);
#endif
                                    if (pIfPhysTemp->PCI_Slot > 
                                            pIfPhys->PCI_Slot) {
#if NIC_DEBUG 
                                        fprintf(stderr,
                                                "switching PCI_Slot"
                                                " %d(port %d) with "
                                                "%d(port %d)\n", 
                                                pIfPhysTemp->PCI_Slot,
                                                pIfPhysTemp->AdapterPort,
                                                pIfPhys->PCI_Slot,
                                                pIfPhys->AdapterPort);
#endif
                                        /* switch the adapter port numbers */
                                        iTemp = pIfPhysTemp->AdapterPort;
                                        pIfPhysTemp->AdapterPort = 
                                            pIfPhys->AdapterPort;
                                        pIfPhys->AdapterPort = iTemp;
                                    }
                                }
                            }
                        }
                    } else {
                        fprintf(stderr, 
                                "ERROR: cpqnic slot number %d >= %d\n",
                                pIfPhys->AdapterPort, 
                                MAX_PHYSICAL_SLOTS);
                    }
                }
            }

            if (do_remap) {
                if (iGetLogicalInfo(pIfPhys, 
                            pIfLog, 
                            iLogIndex+1,
                            iPhysIndex+1) < 0) {
                    /* could not load logical info, so skip this entry.... */
                    pstrNicPort = pstrNicPort->next; 
                    fprintf(stderr,"iGetLogicalInfo Failed\n");
                    continue;
                }
            }

            /* MapOverallCondition from the CQPNIC.MIB --
             * "The overall condition of all interfaces.  This object
             *  is the worst case of any individual interface.  For
             *  example, if there is one degraded interface, this variable
             *  will have a value of degraded(3).  If there is one failed
             *  interface, this variable will have a value of failed(4)."
             */
            if (pIfPhys->AdapterCondition != ADAPTER_CONDITION_OK) {
                if (pIfPhys->AdapterCondition == ADAPTER_CONDITION_FAILED) {
                    NicMibVars.MapOverallCondition = 
                        MAP_OVERALL_CONDITION_FAILED;
                }
                else if ((pIfPhys->AdapterCondition == 
                            ADAPTER_CONDITION_DEGRADED) &&
                        (NicMibVars.MapOverallCondition != 
                         MAP_OVERALL_CONDITION_FAILED)) {
                    NicMibVars.MapOverallCondition = 
                        ADAPTER_CONDITION_DEGRADED;
                }
                else if ((pIfPhys->AdapterCondition == 
                            ADAPTER_CONDITION_OTHER) &&
                        (NicMibVars.MapOverallCondition != 
                         MAP_OVERALL_CONDITION_FAILED) &&
                        (NicMibVars.MapOverallCondition != 
                         MAP_OVERALL_CONDITION_DEGRADED)) {
                    NicMibVars.MapOverallCondition = 
                        MAP_OVERALL_CONDITION_OTHER;
                }
            }

            iPhysIndex++;
            iLogIndex++;
        } else {
            if (pstrNicPort->iProcfsStatus <= PROCFS_REMAP_NEEDED) {
                iRemapNeeded = 1;
            }
            /* else, we just skip this procfs file - it is bad and we */
            /* don't want to keep re-mapping it...                    */
        }

        pstrNicPort = pstrNicPort->next; 
    }

    /* in case we skipped procfs files */
    NicMibVars.NumPhysNics = iPhysIndex; 
    NicMibVars.NumLogNics = iLogIndex; 

    NicMibVars.MibCondition = MIB_CONDITION_OK;
    cpqHoMibHealthStatusArray[CPQMIBHEALTHINDEX] = MIB_CONDITION_OK;

    for (i=0; i<NicMibVars.NumPhysNics; i++) {
        /* if any nic has failed, the condition is failed... */
        if (NicMibVars.pIfPhys[i].AdapterCondition != 
                ADAPTER_CONDITION_OK) {
            if (NicMibVars.pIfPhys[i].AdapterCondition == 
                    ADAPTER_CONDITION_FAILED) {
                NicMibVars.MibCondition = MIB_CONDITION_FAILED;
                cpqHoMibHealthStatusArray[CPQMIBHEALTHINDEX] =
                                                MIB_CONDITION_FAILED;
            }
            else if ((NicMibVars.pIfPhys[i].AdapterCondition == 
                        ADAPTER_CONDITION_DEGRADED) &&
                    (NicMibVars.MibCondition != 
                     MIB_CONDITION_FAILED)) {
                NicMibVars.MibCondition = MIB_CONDITION_DEGRADED;
                cpqHoMibHealthStatusArray[CPQMIBHEALTHINDEX] =
                                                MIB_CONDITION_DEGRADED;
            }
            else if ((NicMibVars.pIfPhys[i].AdapterCondition == 
                        ADAPTER_CONDITION_OTHER) &&
                    (NicMibVars.MibCondition != 
                     MIB_CONDITION_FAILED) &&
                    (NicMibVars.MibCondition != 
                     MIB_CONDITION_DEGRADED)) {
                NicMibVars.MibCondition = MIB_CONDITION_UNKNOWN;
                cpqHoMibHealthStatusArray[CPQMIBHEALTHINDEX] =
                                                MIB_CONDITION_UNKNOWN;
            }
        }
    }

    return(&NicMibVars);
}

/******************************************************************************
 *   Description: Unit test; creates get_stats executable                 
 *   Parameters : 1) main arg count                                       
 *                2) main arg vector                                      
 *   Returns    : zero on success; non-zero on failure                    
 ******************************************************************************/
#ifdef UNIT_TEST_GET_STATS
int main(int argc, char **argv)
{

    int i;
    cpqNic_t* pstrCpqNic;

    if (getuid() != 0 && (geteuid() != 0)) {
        fprintf(stderr, "ERROR: cmanic, must be root to run this program\n");
        return(1);
    }

    /* prevent struct from changing while we are working with it */

    pstrCpqNic = pstrGetNicMibVariables();


    if (pstrCpqNic == (cpqNic_t*)0) {
        fprintf(stderr, "ERROR: cmanic, could not get NIC information\n");
        return(1);
    }

    vPrintCqpNic(pstrCpqNic);

#ifdef DONT_DO

    int iResult;
    NicPort *pNicPorts;

    pNicPorts = (NicPort*)0;

    iResult = iGetNicProcFiles(&pNicPorts);
    if (iResult < 0) {
        return(1);
    }

    iResult = iMapTags(pNicPorts);
    if (iResult < 0) {
        return(1);
    }

    vPrintNicPortStructs(pNicPorts);
    /* vPrintTagMapStructs(pNicPorts); */

    vFreeNicPorts(&pNicPorts);
#endif /* DONT_DO */

    return(0);
} 
#endif /* UNIT_TEST_GET_STATS */

#if HP_NETSNMP
get_data()
{
}

cleanup_agent()
{
}
#endif



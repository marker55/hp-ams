/*----------------------------------------------------------------------*\
|        Copyright (c) 2000  Compaq Computer Corporation                 |
|-----------------------------------------------------------------------*|
|                                                                        |
|  Description:                                                          |
|    Header file for functions calling cpqnic.c library.                 |
|                                                                        |
|                                                                        |
|                                                                        |
\*----------------------------------------------------------------------*/

#ifndef CPQNIC_A_H
#define CPQNIC_A_H

#define OCTET_STRING_LEN  256
#define IFLOGMAPPHYSICALADAPTERS_LEN  16
#define IFLOGMAPMACADDRESS_LEN  6
#define MAX_FILE_LINE_LEN       512
#define NIC_STRUCT_ALLOCATION   4
#define INTERFACE_NAME_LEN      16
#define OS_COMMON_POLL_FREQ     20

/* interface number length, cpqNicIfLogMapIfNumber, used in cpqNic.c */
#define MAP_IF_NUMBER_LEN  4
#define UNKNOWN_SLOT -1
#define UNKNOWN_PORT -1
#define MAC_ADDRESS_BYTES      6

#define CPQNICVAR 0
#define CPQNICVVT 1
#define CPQNICIFLOG 2
#define CPQNICIFPHYS 3
#define CPQNICVLAN 4
#define CPQNICBOND 5

int SEND_LINK_UP_TRAP_ON_STARTUP;
int SEND_LINK_DOWN_TRAP_ON_STARTUP;

char * custom_link_up_str;
char * custom_link_down_str;

#pragma pack(1)

///
/// dynamic interface informaion - not read from configuration files!!!
///
/// \see get_dynamic_interface_info()
/// \see free_dynamic_interface_info()
///
/// interface/link up,down,etc

typedef struct _cpqNicIfLogical_t 
{
   char   InterfaceName[INTERFACE_NAME_LEN+1];
   /* begin cpqNicIfLogMapTable table variables */
   int  MapIndex;
   char MapIfNumber[OCTET_STRING_LEN+1];
   char MapDescription[OCTET_STRING_LEN+1];
   int  MapGroupType;
   int  MapAdapterCount;
   int  MapAdapterOKCount;
   char MapPhysicalAdapters[IFLOGMAPPHYSICALADAPTERS_LEN+1];
   char MapMACAddress[IFLOGMAPMACADDRESS_LEN];
   int  MapSwitchoverMode;
   int  MapCondition;
   int  MapStatus;
   int  MapNumSwitchovers;
   char MapHwLocation[OCTET_STRING_LEN+1];
   uint MapSpeedMbps; /* mapped from AdapterMapSpeedMbps */
   uint VlanCount; /* number of vlans associated with this interface */
   char MapVlans[OCTET_STRING_LEN+1];
   char MapIPV4Address[OCTET_STRING_LEN+1];
   char MapIPV6Address[OCTET_STRING_LEN+1];
} cpqNicIfLogical_t;

typedef struct _cpqNicIfPhysical_t 
{
   ushort VendorID;
   ushort DeviceID;
   ushort SubsysVendorID;
   ushort SubsysDeviceID;
   char   InterfaceName[INTERFACE_NAME_LEN+1];

   /* begin cpqNicIfPhysAdapterTable table variables */
   int  AdapterIndex;
   char AdapterIfNumber[OCTET_STRING_LEN];
   int  AdapterRole;
   char AdapterMACAddress[IFLOGMAPMACADDRESS_LEN];
   char SoftwareMACAddress[IFLOGMAPMACADDRESS_LEN];
   int  AdapterSlot;         /* physical slot */
   int  PCI_Slot;            /* pci slot      */
   int  AdapterIoAddr;
   int  AdapterIrq;
   int  AdapterDma;
   uint AdapterMemAddr;
   int  AdapterPort;         /* port for dual port nics */
   int  AdapterVirtualPort;         /* port for dual port nics */
   int  PCI_Bus;
   int  AdapterDuplexState;
   int  AdapterCondition;
   int  AdapterState;
   int  AdapterStatus;
   int  AdapterStatsValid;
   int  AdapterAutoNegotiate;
   int  AdapterConfiguredSpeedDuplex;
   uint AdapterGoodTransmits;
   uint AdapterGoodReceives;
   uint AdapterBadTransmits;
   uint AdapterBadReceives;
   uint AdapterAlignmentErrors;
   uint AdapterFCSErrors;
   uint AdapterSingleCollisionFrames;
   uint AdapterMultipleCollisionFrames;
   uint AdapterDeferredTransmissions;
   uint AdapterLateCollisions;
   uint AdapterExcessiveCollisions;
   uint AdapterInternalMacTransmitErrors;
   uint AdapterCarrierSenseErrors;
   uint AdapterFrameTooLongs;
   uint AdapterInternalMacReceiveErrors;
   char AdapterHwLocation[OCTET_STRING_LEN+1];
   uint AdapterMapSpeedMbps; /* we need speed here becuase we only map procfs */
                         /* to phys struct members                         */
   char AdapterPartNumber[OCTET_STRING_LEN+1];
//   char AdapterDescription[OCTET_STRING_LEN+1];

   uint AdapterInOctets;
   uint AdapterOutOctets;
   char AdapterName[OCTET_STRING_LEN+1];
   int AdapterIoBayNo;
   char AdapterFWVersion[OCTET_STRING_LEN+1];

} cpqNicIfPhysical_t;


typedef struct _cpqNicIfVlan_t 
{
   uint MapIndex;
   uint MapLogIndex;
   uint MapIfIndex;
   uint MapVlanId;
   char Name[OCTET_STRING_LEN+1];
   char IPV6Address[OCTET_STRING_LEN+1];
} cpqNicIfVlan_t;

typedef struct _cpqNic_t 
{
   int MibRevMajor;
   int MibRevMinor;
   int MibCondition;
   int OsCommonPollFreq;      /* poll frequency in seconds */
   int MapOverallCondition;

   int NumLogNics;
   cpqNicIfLogical_t *pIfLog; 

   int NumPhysNics;
   cpqNicIfPhysical_t *pIfPhys;  /* an entry for each nic that supports the  */
                                 /* procfs format...                         */
   int NumVlans;
   cpqNicIfVlan_t *pIfVlan;      /* an entry for each vlan                   */

} cpqNic_t;
#pragma pack()


/* function prototypes */
cpqNic_t* pstrGetNicMibVariables(void);
void lock_mibvar_mutex(void);
void unlock_mibvar_mutex(void);


#endif /* CPQNIC_H */


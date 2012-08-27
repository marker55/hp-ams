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

#define PROC_NET_IF_INET6   "/proc/net/if_inet6"
#define IPV6_ADDRESS_USER_READABLE  \
           ((IPV6_ADDRESS_ASCII_CHARS+((IPV6_ADDRESS_ASCII_CHARS-1)/4))+1)
int SEND_LINK_UP_TRAP_ON_STARTUP;
int SEND_LINK_DOWN_TRAP_ON_STARTUP;

char * custom_link_up_str;
char * custom_link_down_str;
typedef enum
{
   link_unknown = 0,
   link_up,
   link_down,
   link_failed,
   link_degraded,
} link_status_t;

// IPv6 scope, see netinet/in.h
typedef enum
{
    ipv6_global_scope = 0,
    ipv6_link_scope,
    ipv6_site_scope,
    ipv6_v4compat_scope,
    ipv6_host_scope,
    ipv6_loopback_scope,
    ipv6_unspecified_scope
} ipv6_link_scope_t;

// fourth field in PROC_NET_IF_INET6 file
#define IPV6_ADDR_ANY         0x0000U
#define IPV6_ADDR_LOOPBACK    0x0010U
#define IPV6_ADDR_LINKLOCAL   0x0020U
#define IPV6_ADDR_SITELOCAL   0x0040U
#define IPV6_ADDR_COMPATv4    0x0080U

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
   unsigned int MapSpeedMbps; /* mapped from AdapterMapSpeedMbps */
   unsigned int VlanCount; /* number of vlans associated with this interface */
   char MapVlans[OCTET_STRING_LEN+1];
   char MapIPV4Address[OCTET_STRING_LEN+1];
   char MapIPV6Address[OCTET_STRING_LEN+1];
} cpqNicIfLogical_t;

typedef struct _cpqNicIfPhysical_t 
{
   unsigned short VendorID;
   unsigned short DeviceID;
   unsigned short SubsysVendorID;
   unsigned short SubsysDeviceID;
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
   unsigned int AdapterMemAddr;
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
   unsigned int AdapterGoodTransmits;
   unsigned int AdapterGoodReceives;
   unsigned int AdapterBadTransmits;
   unsigned int AdapterBadReceives;
   unsigned int AdapterAlignmentErrors;
   unsigned int AdapterFCSErrors;
   unsigned int AdapterSingleCollisionFrames;
   unsigned int AdapterMultipleCollisionFrames;
   unsigned int AdapterDeferredTransmissions;
   unsigned int AdapterLateCollisions;
   unsigned int AdapterExcessiveCollisions;
   unsigned int AdapterInternalMacTransmitErrors;
   unsigned int AdapterCarrierSenseErrors;
   unsigned int AdapterFrameTooLongs;
   unsigned int AdapterInternalMacReceiveErrors;
   char AdapterHwLocation[OCTET_STRING_LEN+1];
   unsigned int AdapterMapSpeedMbps; /* we need speed here becuase we only map procfs */
                         /* to phys struct members                         */
   char AdapterPartNumber[OCTET_STRING_LEN+1];
//   char AdapterDescription[OCTET_STRING_LEN+1];

   unsigned int AdapterInOctets;
   unsigned int AdapterOutOctets;
   char AdapterName[OCTET_STRING_LEN+1];
   int AdapterIoBayNo;
   char AdapterFWVersion[OCTET_STRING_LEN+1];

} cpqNicIfPhysical_t;


typedef struct _cpqNicIfVlan_t 
{
   unsigned int MapIndex;
   unsigned int MapLogIndex;
   unsigned int MapIfIndex;
   unsigned int MapVlanId;
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

typedef struct _dynamic_interface_info {
   char *ip_addr;
   char *netmask;
   char *broadcast;
   char *current_mac;
   char *ipv6_addr;
   char *ipv6_shorthand_addr;
   unsigned int ipv6_prefix_length;
   ipv6_link_scope_t ipv6_scope;
   link_status_t if_status;
} dynamic_interface_info;
   //
#pragma pack()


/* function prototypes */
cpqNic_t* pstrGetNicMibVariables(void);
void lock_mibvar_mutex(void);
void unlock_mibvar_mutex(void);
void free_dynamic_interface_info(dynamic_interface_info *);
int get_dynamic_interface_info(char*, dynamic_interface_info *);


#endif /* CPQNIC_H */

typedef struct
{
   char *driver_name;
   char *driver_version;
   char *firmware_version;
   char *bus_info;
   unsigned char duplex;
   unsigned char autoneg;
   unsigned short link_speed;
   link_status_t link_status;
   unsigned char perm_addr[MAC_ADDRESS_BYTES];
} ethtool_info;


// (c) Copyright 2006 Hewlett-Packard Development Company, L.P.
// All rights reserved
//
////////////////////////////////////////////////////////////////////////////////
/// \file common_structs.h
///
/// This file contains C declarations, no definitions (i.e. memory allocation) 
/// are in this file, for stucts that are *shared* between multiple source 
/// files in this directory.
///
////////////////////////////////////////////////////////////////////////////////


#ifndef COMMON_STRUCTS_H
#define COMMON_STRUCTS_H

////////////////////////////////////////////////////////////////////////////////
// SYSTEM HEADER FILES
////////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <string.h>
#include <linux/types.h>
#include <sys/types.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/sockios.h>
#include <linux/in.h>
#include <linux/in6.h>
#include <sys/socket.h>
#include <sys/stat.h>

// including arpa/inet.h causes all types of header file conflicts; declare
// the two functions we use.
/// \todo Change header files around so we don't need to define
///       inet_ntop and inet_pton locally.
//#include <arpa/inet.h>
const char *inet_ntop(int af, const void *src,
                      char *dst, socklen_t cnt);
int inet_pton(int af, const char *src, void *dst);



////////////////////////////////////////////////////////////////////////////////
// LOCAL HEADER FILES
////////////////////////////////////////////////////////////////////////////////

// these typedefs are required here since local header files use them
// v required to build on SLES9
typedef signed char s8;
typedef unsigned char u8;
typedef signed short s16;
typedef unsigned short u16;
typedef signed int s32;
typedef unsigned int u32;
typedef signed long long s64;
#ifndef PCI_U64_FMT
typedef unsigned long long u64;
#endif
// ^ required to build on SLES9

/// \todo move up after other includes when RH/SUSE get the above
///       typedefs added
#include <linux/ethtool.h>


////////////////////////////////////////////////////////////////////////////////
// DEFINES - NON MACRO
////////////////////////////////////////////////////////////////////////////////

/// copyright message
#define HP_COPYRIGHT \
        "Copyright (c) 2006 Hewlett-Packard Development Company, L.P."


/// turn on debugging
#define HP_COMMON_UTIL_DEBUG 0

#ifndef DPRINTF // allow external override of DPRINTF debug MACRO
#if HP_COMMON_UTIL_DEBUG
/// general debugging
#define DPRINTF(V...)      printf(V...)
#else  // HP_COMMON_UTIL_DEBUG
/// see above
#define DPRINTF(V...)
#endif // HP_COMMON_UTIL_DEBUG
#endif // DPRINTF

/// location of dir containing network device files
#define SYS_NET_PATH   "/sys/class/net"

/// location of dir containing network interface ipv6 addresses
#define PROC_NET_IF_INET6   "/proc/net/if_inet6"

/// the function call succeeded
#define SUCCESS   0

/// bytes in MAC address
#define MAC_ADDRESS_BYTES      6

/// bits in an IPv6 address
#define IPV6_ADDRESS_BITS      128

/// bytes in an IPv6 address
#define IPV6_ADDRESS_BYTES      (128/(sizeof(char)))

/// string length to store IPv6 address as ASCII (+1 for null)
#define IPV6_ADDRESS_ASCII_CHARS  ((2*(IPV6_ADDRESS_BYTES))+1)

/// string length to store IPv6 address in user readable ASCII (+1 for null)
/// ie fe80:208:2ff:fee7:ecba...
/// div by 4 because we have a ":" every four chars
/// (yes, IPV6_ADDRESS_ASCII_CHARS contains a null so we don't need space for
/// one here, but adding one makes the code easier to read when looking at
/// issues - one automatically thins +1 for the null.
#define IPV6_ADDRESS_USER_READABLE  \
           ((IPV6_ADDRESS_ASCII_CHARS+((IPV6_ADDRESS_ASCII_CHARS-1)/4))+1)

////////////////////////////////////////////////////////////////////////////////
// DEFINES - MACROS
////////////////////////////////////////////////////////////////////////////////

/// check that a pointer is non-null (zero), abort if it is;
/// used to check calls to malloc/new
#define CHECK_MALLOC(p)                                                   \
        if (!(p)) {                                                       \
           fprintf(stderr,                                                \
                   "ERROR: malloc failed (errno=%d), %s,%d\n",            \
                   errno, __FILE__, __LINE__);                            \
           abort()  ;                                                     \
        }

/// free and null memory
#define FREE_AND_NULL(p)                                                  \
        if ( (p) ) {                                                      \
           free(p); (p) = NULL;                                           \
        } 

/// duplicate a string checking for NULL - STRDUP Non-Null; this prevents
/// checking every string being non-null before duplicating it; strdup 
/// segfaults if a null string is passed
#define STRDUP_NN(p)   ((p)!=NULL?strdup(p):NULL)

/// copy a string checking for NULL - STRCPY Non-Null; this prevents
/// checking every string being non-null before copying it; strcpy 
/// segfaults if a null string is passed to copy. below code is for
/// readability and conformance to strcpy return value.
#define STRCPY_NN(d,s)   ((s)!=NULL?strcpy(d,s):strcpy(d,""))

typedef enum
{
   link_unknown = 0,
   link_up,
   link_down,
   link_failed,
   link_degraded,
} link_status_t;

/// IPv6 scope, see netinet/in.h
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



///////////////////////////////////////////////////////////////////
// this structure contains information from the ethtool ioctl
///////////////////////////////////////////////////////////////////
typedef struct
{
   /// driver of driver; (bnx, e1000, etc)
   char *pdriver_name;

   /// driver version; (1.7.98, 3.4.8b-1)
   char *pdriver_version;

   /// firmware version; (3.1.9)
   char *pfirmware_version;

   /// pci bus info; (0000:02:0e.0)
   char *pbus_info;
  
   /// Duplex
   unsigned char duplex;

   /// Auto Negotiation
   unsigned char autoneg;

   ///Link speed info;(0,100,1000,10000)
   ushort link_speed;	

   /// logical link status of interface (up, down, degraded, failed, unknown...)
   link_status_t link_status;

   /// permanent mac address, as stored in eeprom
   u8 perm_addr[MAC_ADDRESS_BYTES];

} ethtool_info;

/// max inf string length
#define MAX_INF_STRING_LEN 69

///
/// NIC hardware database
///
/// This is the hardware structure built from the Hardware Teams PDF file
/// located under \\txnfile01\Projects\ADG\PRJINFO\DesignDocs.
///
/// \note anytime a member is added to this function, the get_db_struct_info()
///       and free_db_struct_info() functions must be updated too.
typedef struct 
{

   /// pci vendor
   int32_t pci_ven;

   /// pci device
   int32_t pci_dev;

   /// pci sub vendor
   int32_t pci_sub_ven;

   /// pci sub device
   int32_t pci_sub_dev;

   /// OEM, CPQ, HP 
   char  *poem;

   /// pca part number
   char *ppca_part_number;

   /// spares part number
   char *pspares_part_number;

   /// board rev
   char *pboard_rev;

   /// port type
   char *pport_type;

   /// bus type
   char *pbus_type;

   /// length of pinf_name string below excluding the NULL
   int32_t inf_name_len;

   /// name
   char *pname;

} nic_hw_db_t;


///
/// dynamic interface informaion - not read from configuration files!!!
///
/// \see get_dynamic_interface_info()
/// \see free_dynamic_interface_info()
///
typedef struct 
{
   // IP address; "10.10.10.10"
   char *pip_addr;

   /// netmask; "255.255.0.0"
   char *pnetmask;

   /// broadcast address; "x.x.255.255"
   char *pbroadcast;

   /// current MAC address; "11:22:33:44:55:66"
   char *pcurrent_mac;

   // IPv6 address; "fe80:0000:0000:0000:0208:02ff:fee7:ecba"
   char *pipv6_addr;

   // IPv6 shorthand address; "fe80::208:2ff:fee7:ecba"
   char *pipv6_shorthand_addr;

   // IPv6 prefix length; ".../64"
   uint32_t pipv6_refix_length;

   // IPv6 network scope
   ipv6_link_scope_t ipv6_scope;

   /// logical link status of interface (up, down, unknown)
   link_status_t if_status;

} dynamic_interface_info_t;

#endif /* COMMON_STRUCTS_H */

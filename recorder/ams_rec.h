/////////////////////////////////////////////////////////////////////
//
//  ams_rec.h
//
//  Recorder Items for the Agentless Monitoring Service
//
//  Copyright 2011 by Hewlett-Packard Development Company, L.P.
//
/////////////////////////////////////////////////////////////////////

#ifndef INCL_RECORDER_AMS_ITEMS_H
#define INCL_RECORDER_AMS_ITEMS_H

/////////////////////////////////////////////////////////////////////
//
//  Data is logged to Recorder in variable length binary records.
//  Each data source defines its own record formats.  There is no
//  central authority maintaining a unified specification or header
//  files (see also: chaos).
//
//  AMS records have a fixed length base structure, followed by zero
//  or more variable length NUL terminated strings.
//
/////////////////////////////////////////////////////////////////////

extern int s_ams_rec_class;
extern int s_ams_rec_handle;
#include "recorder_types.h"

#pragma pack(1)

/////////////////////////////////////////////////////////////////////
//
//  AMS class
//
/////////////////////////////////////////////////////////////////////

#define REC_CLASS_AMS    "AMS"

/////////////////////////////////////////////////////////////////////
//
//  AMS codes
//
/////////////////////////////////////////////////////////////////////

enum REC_AMS_Codes
{
    REC_CODE_AMS_OS_BOOT             = 1,
    REC_CODE_AMS_OS_SHUTDOWN         = 2,
    REC_CODE_AMS_OS_INFORMATION      = 3,
    REC_CODE_AMS_HOST_NAME           = 4,
    REC_CODE_AMS_DRIVER              = 5,
    REC_CODE_AMS_SERVICE             = 6,
    REC_CODE_AMS_SOFTWARE            = 7,
    REC_CODE_AMS_OS_CRASH            = 8,
    REC_CODE_AMS_OS_EVENT            = 9,
    REC_CODE_AMS_DEVICE_EVENT        = 10,
    REC_CODE_AMS_OS_USAGE            = 11,
    REC_CODE_AMS_MEMORY_USAGE        = 12,
    REC_CODE_AMS_PROCESSOR_USAGE     = 13,
    REC_CODE_AMS_NIC_INFO            = 14,
    REC_CODE_AMS_NIC_LINK            = 15
};

#define REC_CODE_AMS_OS_BOOT_STR         "OS Boot"
#define REC_CODE_AMS_OS_SHUTDOWN_STR     "OS Shutdown"
#define REC_CODE_AMS_OS_INFORMATION_STR  "OS Info"
#define REC_CODE_AMS_HOST_NAME_STR       "Host Name"
#define REC_CODE_AMS_DRIVER_STR          "Driver"
#define REC_CODE_AMS_SERVICE_STR         "Service"
#define REC_CODE_AMS_SOFTWARE_STR        "Packages"
#define REC_CODE_AMS_OS_CRASH_STR        "OS Crash"
#define REC_CODE_AMS_OS_EVENT_STR        "OS Event"
#define REC_CODE_AMS_DEVICE_EVENT_STR    "Device Event"
#define REC_CODE_AMS_OS_USAGE_STR        "OS Usage"
#define REC_CODE_AMS_MEMORY_USAGE_STR    "Memory Usage"
#define REC_CODE_AMS_PROCESSOR_USAGE_STR "Processor Usage"
#define REC_CODE_AMS_NIC_INFO_STR        "NIC Info"
#define REC_CODE_AMS_NIC_LINK_STR        "NIC Link"

/////////////////////////////////////////////////////////////////////
//
//  REC_CODE_AMS_OS_BOOT
//
//  (There is no data attached to this record)
//
/////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////
//
//  REC_CODE_AMS_OS_SHUTDOWN
//
//  (There is no data attached to this record)
//
/////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////
//
//  REC_CODE_AMS_OS_INFORMATION
//
//  [0] = REC_AMS_OsInfoData
//  [1] = OS name string
//  [2] = OS version string
//
/////////////////////////////////////////////////////////////////////

struct REC_AMS_OsInfoData
{
    UINT8   Type;
    UINT8   Vendor;
    UINT16  Bits;           // 32 or 64
    UINT16  Version[4];     // major/minor/build/patch
    UINT16  ServicePack;
};

// REC_AMS_OsInfoData.Type
// (same as SMIF packet 0x0086)

enum REC_AMS_OsType
{
    REC_AMS_OsType_Windows           = 0,
    REC_AMS_OsType_Netware           = 1,
    REC_AMS_OsType_Linux             = 2,
    REC_AMS_OsType_FreeBSD           = 3,
    REC_AMS_OsType_Solaris           = 4,
    REC_AMS_OsType_VMware            = 5,
    REC_AMS_OsType_Other             = 254
};

// REC_AMS_OsInfoData.Vendor
// (same as SMIF packet 0x0086)

enum REC_AMS_OsVendor
{
    REC_AMS_OsVendor_Microsoft       = 0,
    REC_AMS_OsVendor_Novell          = 1,
    REC_AMS_OsVendor_RedHat          = 2,
    REC_AMS_OsVendor_SUSE            = 3,
    REC_AMS_OsVendor_Debian          = 4,
    REC_AMS_OsVendor_VMware          = 5,
    REC_AMS_OsVendor_FreeBSD         = 6,
    REC_AMS_OsVendor_Oracle          = 7,
    REC_AMS_OsVendor_Sun             = REC_AMS_OsVendor_Oracle,
    REC_AMS_OsVendor_Other           = 254
};

/////////////////////////////////////////////////////////////////////
//
//  REC_CODE_AMS_HOST_NAME
//
//  [0] = Computer name string
//  [1] = Fully qualified domain name (FQDN) string
//
/////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////
//
//  REC_CODE_AMS_DRIVER
//
//  [0] = REC_AMS_DriverData
//  [1] = Driver name string
//  [2] = Driver filename string
//  [3] = Driver version string
//  [4] = Driver timestamp string
//  [5] = Driver vendor string
//
/////////////////////////////////////////////////////////////////////

struct REC_AMS_DriverData
{
    UINT16   Flags;
};

// REC_AMS_OsDriverData.Flags

enum REC_AMS_DriverFlags
{
    REC_AMS_Driver_First             = 0x0001,
    REC_AMS_Driver_Last              = 0x0002,
    REC_AMS_Driver_Load              = 0x0004,
    REC_AMS_Driver_Unload            = 0x0008
};

/////////////////////////////////////////////////////////////////////
//
//  REC_CODE_AMS_SERVICE
//
//  [0] = REC_AMS_ServiceData
//  [1] = Service name string
//  [2] = Service filename string
//  [3] = Service version string
//  [4] = Service timestamp string
//  [5] = Service vendor string
//
/////////////////////////////////////////////////////////////////////

struct REC_AMS_ServiceData
{
    UINT16   Flags;
};

// REC_AMS_OsDriverData.Flags

enum REC_AMS_ServiceFlags
{
    REC_AMS_Service_First            = REC_AMS_Driver_First,
    REC_AMS_Service_Last             = REC_AMS_Driver_Last,
    REC_AMS_Service_Start            = REC_AMS_Driver_Load,
    REC_AMS_Service_Stop             = REC_AMS_Driver_Unload
};

/////////////////////////////////////////////////////////////////////
//
//  REC_CODE_AMS_SOFTWARE
//
//  [0] = REC_AMS_PackageData
//  [1] = Program vendor string
//  [2] = Program name string
//  [3] = Program version string
//  [4] = Installation timestamp string
//  [5] = Installation package name string
//
/////////////////////////////////////////////////////////////////////

struct REC_AMS_PackageData
{
    UINT16   Flags;
};

enum REC_AMS_PackageFlags
{
    REC_AMS_Package_First           = 0x0001,
    REC_AMS_Package_Last            = 0x0002,
    REC_AMS_Package_Install         = 0x0004,
    REC_AMS_Package_Uninstall       = 0x0008,
    REC_AMS_Package_HP              = 0x0100,
    REC_AMS_Package_OsServicePack   = 0x0200,
    REC_AMS_Package_OsHotFix        = 0x0400
};

/////////////////////////////////////////////////////////////////////
//
//  REC_CODE_AMS_OS_CRASH
//
//  [0] = REC_AMS_OsCrashData
//  [1] = Message text
//
/////////////////////////////////////////////////////////////////////

struct REC_AMS_OsCrashData
{
    UINT64  BugCheckCode;
    UINT64  BugCheckData[4];
};

/////////////////////////////////////////////////////////////////////
//
//  REC_CODE_AMS_OS_EVENT
//
//  [0] = REC_AMS_OsEventData
//  [1] = Message source
//  [2] = Message text
//
/////////////////////////////////////////////////////////////////////

struct REC_AMS_OsEventData
{
    UINT8   Severity;
    UINT8   LogType;
    UINT16  RepeatCount;
    UINT32  Id;
};

// REC_AMS_OsEventData.Severity

enum REC_AMS_EventSeverity
{
    REC_AMS_Severity_Unknown         = 0,
    REC_AMS_Severity_Information     = 1,
    REC_AMS_Severity_Warning         = 2,
    REC_AMS_Severity_Error           = 3
};

// REC_AMS_OsEventData.Log

enum REC_AMS_EventLogType
{
    REC_AMS_LogType_Unknown          = 0,
    REC_AMS_LogType_System           = 1,
    REC_AMS_LogType_Application      = 2
};

/////////////////////////////////////////////////////////////////////
//
//  REC_CODE_AMS_DEVICE_EVENT
//
//  [0] = REC_AMS_DeviceEventData
//  [1] = Location string
//  [2] = Device ID string
//  [3] = Device name string
//
/////////////////////////////////////////////////////////////////////

struct REC_AMS_DeviceEventData
{
    UINT8   EventType;
};

// REC_AMS_DeviceHotPlugEventData.Operation

enum REC_AMS_DeviceEventType
{
    REC_AMS_DeviceEvent_Insert       = 1,
    REC_AMS_DeviceEvent_Remove       = 2
};

/////////////////////////////////////////////////////////////////////
//
//  REC_CODE_AMS_OS_USAGE
//
//  [0] = REC_AMS_OsUsageData
//
/////////////////////////////////////////////////////////////////////

struct REC_AMS_OsUsageData
{
    UINT32   ProcessCount;
    UINT32   ThreadCount;
    UINT32   HandleCount;
    UINT32   LoadAverage[3];
};

/////////////////////////////////////////////////////////////////////
//
//  REC_CODE_AMS_MEMORY_USAGE
//
//  [0] = REC_AMS_MemoryUsageData
//
/////////////////////////////////////////////////////////////////////

struct REC_AMS_MemoryUsageData
{
    UINT64   PhysicalTotal;
    UINT64   PhysicalFree;
    UINT64   VirtualTotal;
    UINT64   VirtualFree;
};

/////////////////////////////////////////////////////////////////////
//
//  REC_CODE_AMS_PROCESSOR_USAGE
//
//  [0] = REC_AMS_ProcessorUsageData
//
/////////////////////////////////////////////////////////////////////

struct REC_AMS_ProcessorUsageData
{
    UINT8    User;
    UINT8    Kernel;
};

/////////////////////////////////////////////////////////////////////
//
//  REC_CODE_AMS_NIC_INFO
//
//  [0] = REC_AMS_NicInfoData
//  [1] = NIC description string
//
/////////////////////////////////////////////////////////////////////

struct REC_AMS_NicInfoData
{
    UINT32  Interface;
    UINT16  Flags;
    UINT8   MacAddress[6];
    UINT8   Ip4Address[4];
    UINT8   Ip6Address[16];
};

// REC_AMS_NicInfoData.Flags and REC_AMS_NicLinkData.Flags

enum REC_AMS_NicFlags
{
    REC_AMS_Nic_First                = 0x0001,
    REC_AMS_Nic_Last                 = 0x0002,
    REC_AMS_Nic_Add                  = 0x0004,
    REC_AMS_Nic_Remove               = 0x0008,
    REC_AMS_Nic_LinkState            = 0x0080,
    REC_AMS_Nic_LinkUp               = 0x0080,
    REC_AMS_Nic_LinkDown             = 0x0000
};

/////////////////////////////////////////////////////////////////////
//
//  REC_CODE_AMS_NIC_LINK
//
//  [0] = REC_AMS_NicLinkData
//
/////////////////////////////////////////////////////////////////////

struct REC_AMS_NicLinkData
{
    UINT32  Interface;
    UINT16  Flags;
};

#pragma pack()

typedef struct REC_AMS_OsInfoData            REC_AMS_OsInfoData;
typedef struct REC_AMS_DriverData            REC_AMS_DriverData;
typedef struct REC_AMS_ServiceData           REC_AMS_ServiceData;
typedef struct REC_AMS_PackageData          REC_AMS_PackageData;
typedef struct REC_AMS_OsCrashData           REC_AMS_OsCrashData;
typedef struct REC_AMS_OsEventData           REC_AMS_OsEventData;
typedef struct REC_AMS_DeviceEventData       REC_AMS_DeviceEventData;
typedef struct REC_AMS_OsUsageData           REC_AMS_OsUsageData;
typedef struct REC_AMS_MemoryUsageData       REC_AMS_MemoryUsageData;
typedef struct REC_AMS_ProcessorUsageData    REC_AMS_ProcessorUsageData;
typedef struct REC_AMS_NicInfoData           REC_AMS_NicInfoData;
typedef struct REC_AMS_NicLinkData           REC_AMS_NicLinkData;
typedef enum   REC_AMS_Codes                 REC_AMS_Codes;
typedef enum   REC_AMS_OsType                REC_AMS_OsType;
typedef enum   REC_AMS_OsVendor              REC_AMS_OsVendor;
typedef enum   REC_AMS_DriverFlags           REC_AMS_DriverFlags;
typedef enum   REC_AMS_ServiceFlags          REC_AMS_ServiceFlags;
typedef enum   REC_AMS_PackageFlags         REC_AMS_PackageFlags;
typedef enum   REC_AMS_EventSeverity         REC_AMS_EventSeverity;
typedef enum   REC_AMS_EventLogType          REC_AMS_EventLogType;
typedef enum   REC_AMS_DeviceEventType       REC_AMS_DeviceEventType;
typedef enum   REC_AMS_NicFlags              REC_AMS_NicFlags;

#ifdef __cplusplus
extern "C" 
#endif
void rec_ams_daemon(int* pAMSRunning,int* pRECRunning);

#endif // INCL_RECORDER_AMS_ITEMS_H

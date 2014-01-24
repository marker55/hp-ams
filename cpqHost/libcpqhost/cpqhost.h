#ifndef CPQHOST_H
#define CPQHOST_H

#ifdef __cplusplus
   extern "C" {
#endif
#include <sys/utsname.h>

#define OS_TYPE_OTHER       1
#define OS_TYPE_NETWARE     2
#define OS_TYPE_NT          3
#define OS_TYPE_SCOUNIX     4
#define OS_TYPE_UNIXWARE    5
#define OS_TYPE_LINUX      14
#define OS_TYPE_RHEL       39
#define OS_TYPE_RHEL_64    40
#define OS_TYPE_SLES       42
#define OS_TYPE_SLES_64    43
#define OS_TYPE_UBUNTU     44
#define OS_TYPE_UBUNTU_64  45
#define OS_TYPE_DEB        46
#define OS_TYPE_DEB_64     47
#define OS_TYPE_LINUX_64   48 
#define OS_TYPE_CENTOS     50
#define OS_TYPE_CENTOS_64  51
#define OS_TYPE_ORACLE     52
#define OS_TYPE_ORACLE_64  53

#define TELNET_OTHER            1
#define TELNET_AVAILABLE        2
#define TELNET_NOTAVAILABLE     3

#define SYSTEMROLE_SIZE 64
#define SYSTEMROLEDETAIL_SIZE   512

#define ONEMEG         (1024*1024)

#define MIB_STATE_OK          2

typedef struct cpqHostFwVerTable_t {
    int HostFwVerIndex;                     
    int HostFwVerCategory;                  
    int HostFwVerDeviceType;                
    unsigned char HostFwVerDisplayName[128];                
    unsigned char HostFwVerVersion[32];                    
    unsigned char HostFwVerLocation[256];                   
    unsigned char HostFwVerXmlString[256];                  
    unsigned char HostFwVerKeyString[128];                  
    int HostFwVerUpdateMethod;              
} cpqHostFWVerTable_t;

typedef struct _cpqHost_t {
    int MibCondition;
#ifdef NOT_AMS
    int OsCommonPollFreq;      /* poll frequency in seconds */
    int MapOverallCondition;
#endif
    int HostFileSysCondition;
    unsigned char HostSwVerAgentsVer[50];
    unsigned char HostMibStatusArray[256];
    unsigned char HostGUID[17];
    int HostWebMgmtPort;
    unsigned char HostGUIDCanonical[36];
    int HostTrapFlags;
    unsigned char HostSwRunningTrapDesc[256];
    unsigned char HostGenericData[256];
    unsigned char HostCriticalSoftwareUpdateData[256];

    int NumFWEntries;
    cpqHostFWVerTable_t *pFWTable;
} cpqHost_t;

typedef  struct _proc_fs_entry_t {
    void *next;
    unsigned char *fstab_field[7];
//    struct statvfs vfsstat;
    unsigned char buf[];
} proc_fs_entry_t;

typedef struct _distro_id_t {
    struct utsname  utsName;
    unsigned char *Vendor;
    unsigned char *LongDistro;
    unsigned char *Description;
    unsigned char *Role;
    unsigned char *Code;
    unsigned char *VersionString;
    int  cpqHostOsType;
    int  Version;
    int  SubVersion;
    int  BuildNumber;
    int  ServicePack;
    int  Bits;
    char *KernelVersion;
    char sysDescr[256];
} distro_id_t;

/* function prototypes */
cpqHost_t* GetHostMibVariables(void);

distro_id_t *getDistroInfo();

#ifdef __cplusplus
   } // extern "C"
#endif

#endif // CPQHOST_H

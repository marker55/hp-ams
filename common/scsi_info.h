#ifndef _SCSI_INFO_H_
#define _SCSI_INFO_H_

#include <linux/types.h>

#define __i8    char
#define IOCMASK 0x90
#define SAS_GET_CNTLR_CONFIG   0xCC770002
#define SAS_GET_CONNECTOR_INFO          0xCC770024

#define SAS_TIMEOUT      60

#define SAS_DATA_READ    0
#define SAS_DATA_WRITE   1

#define SSD_WEAR_OTHER          1
#define SSD_WEAR_OK             2
#define SSD_WEAR_5PERCENT_LEFT  4
#define SSD_WEAR_2PERCENT_LEFT  5
#define SSD_WEAR_OUT            6

#define PHYS_DRV_ROTATING_PLATTERS          2
#define PHYS_DRV_SOLID_STATE                3


#pragma pack(8)

typedef struct _ioctl_hdr {
    __u32 IOControllerNumber;
    __u32 Length;
    __u32 ReturnCode;
    __u32 Timeout;
    __u16 Direction;
} ioctl_hdr;

typedef struct _pciBusAddr {
    __u8  bBusNumber;
    __u8  bDeviceNumber;
    __u8  bFunctionNumber;
    __u8  bReserved;
} pciBusAddr;

typedef union _ioBusAddress {
    pciBusAddr PciAddress;
    __u8  bReserved[32];
} ioBusAddress;

typedef struct _sashba_config {
    __u32 uBaseIoAddress;
    struct {
        __u32 uLowPart;
        __u32 uHighPart;
    } BaseMemoryAddress;
    __u32 uBoardID;
    __u16 usSlotNumber;
    __u8  bControllerClass;
    __u8  bIoBusType;
    ioBusAddress BusAddress;
    char  szSerialNumber[81];
    __u16 usMajorRevision;
    __u16 usMinorRevision;
    __u16 usBuildRevision;
    __u16 usReleaseRevision;
    __u16 usBIOSMajorRevision;
    __u16 usBIOSMinorRevision;
    __u16 usBIOSBuildRevision;
    __u16 usBIOSReleaseRevision;
    __u32 uControllerFlags;
    __u16 usRromMajorRevision;
    __u16 usRromMinorRevision;
    __u16 usRromBuildRevision;
    __u16 usRromReleaseRevision;
    __u16 usRromBIOSMajorRevision;
    __u16 usRromBIOSMinorRevision;
    __u16 usRromBIOSBuildRevision;
    __u16 usRromBIOSReleaseRevision;
    __u8  bReserved[7];
} sashba_config;

typedef struct _sashba_config_buf {
    ioctl_hdr IoctlHeader;
    sashba_config Configuration;
} sashba_config_buf;

typedef struct _sas_connector_info {
   __u32 uPinout;  
   __u8  bConnector[16];
   __u8  bLocation;
   __u8  bReserved[11];
   __u32 uConnectorPinout;
} sas_connector_info;

typedef struct _sas_connector_info_buf {
   ioctl_hdr IoctlHeader;
   sas_connector_info Reference[32];
} sas_connector_info_buf;

#pragma pack()
sashba_config_buf * SasGetHbaConfig (int, char *);
sas_connector_info_buf * SasGetHbaConnector (int, char *);

extern int get_sata_health(int);
extern int get_sata_pwron(int);
extern int get_sata_ssd_wear(int);
extern char *get_sata_temp(int);
extern int sata_parse_current(char *);
extern int sata_parse_mcot(char *);
extern char *get_sata_DiskRev(int);

extern int get_sas_health(int);
extern int get_sas_pwron(int);
extern int get_sas_ssd_wear(int);
extern char *get_sas_temp(int);
extern int sas_parse_current(char *);
extern int sas_parse_mcot(char *);

extern int get_unit_temp(int);
extern int is_unit_ssd(int);
extern unsigned char*get_ScsiGeneric(unsigned char*);
extern unsigned long long get_BlockSize(unsigned char*);
extern int get_DiskType(char *);
extern char * get_DiskModel(char *);
extern char * get_sas_DiskRev(char *);
extern char * get_DiskState(char *);

#endif // _SCSI_INFO_H_


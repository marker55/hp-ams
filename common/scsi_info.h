#ifndef _SCSI_INFO_H_
#define _SCSI_INFO_H_

#include <linux/types.h>

#define __i8    char
#define IOCMASK 0x90
#define SAS_GET_CNTLR_CONFIG   0xCC770002
#define SAS_TIMEOUT      60

#define SAS_DATA_READ    0
#define SAS_DATA_WRITE   1

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

#pragma pack()
sashba_config_buf * SasGetHbaConfig (int, char *);

#endif // _CSMI_SAS_H_


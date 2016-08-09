#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/ioctl.h>
#include <scsi/sg.h>
#include <scsi/scsi.h>
#include <linux/version.h>
#include "scsi_info.h"
#include "utils.h"

#include <dirent.h>

#define BLOCK              0x0         // Defect list formats
#define BYTES_FROM_INDEX   0x4
#define PHYSICAL_SECTOR    0x5
#define VENDOR_SPECIFIC    0x6

typedef struct _BLOCK_DESCRIPTOR
{
   unsigned int DefectiveBlockAddress;
} BLOCK_DESCRIPTOR, *PBLOCK_DESCRIPTOR;


typedef struct _BYTES_FROM_INDEX_DESCRIPTOR
{
   unsigned int Cylinder : 24;
   unsigned int Head : 8;
   unsigned int DefectBytesFromIndex;
} BYTES_FROM_INDEX_DESCRIPTOR, *PBYTES_FROM_INDEX_DESCRIPTOR;


typedef struct _PHYSICAL_SECTOR_DESCRIPTOR
{
   unsigned int Cylinder : 24;
   unsigned int Head : 8;
   unsigned int DefectSectorNumber;
} PHYSICAL_SECTOR_DESCRIPTOR, *PPHYSICAL_SECTOR_DESCRIPTOR;


#define ATA_PASS_THROUGH_16    0x85
#define PROTO_PIO              8  /* 4 <<1 */
#define T_LENGTH               1
#define EXTEND                 1

#define SERVICE_ACTION_IN      0x9e
#define READ_CAPACITY_16       0x10

#define ATA_IDENTIFY_DEVICE    0xec

#define ATA_SMART              0xb0
#define READ_SMART_LOG         0xd5
#define SUM_SMART_ERR_LOG       1

#define ATA_READ_LOG_EXT       0x2f
#define DEV_STAT_LOG           0x04
#define SUP_STAT_PAGE          0x00
#define GEN_STAT_PAGE          0x01
#define FREEFALL_STAT_PAGE     0x02
#define ROTMEDIA_PAGE          0x03
#define GENERR_STAT_PAGE       0x04
#define TEMP_STAT_PAGE         0x05
#define TRANS_STAT_PAGE        0x05
#define SSD_STAT_PAGE          0x07

#define BDC_VPD                0xb1
#define EVPD                   0x01
#ifdef UTTEST
extern  int alphasort();
static struct dirent **ScsiGenericlist;
static char * ScsiGenericDir = "/sys/class/scsi_generic/";
static int NumScsiGeneric;
#endif

static int sd_select(const struct dirent *entry)
{
    if (strncmp(entry->d_name,"block:", 6) == 0)
        return(1);
    if (strncmp(entry->d_name,"sd", 2) == 0)
        return(1);
    return 0;
}

static int generic_select(const struct dirent *entry)
{
    if (strncmp(entry->d_name,"sg", 2) == 0)
        return(1);
    if (strncmp(entry->d_name,"scsi_generic:", 13) == 0)
        return(1);
    return 0;
}

unsigned long long get_BlockSize(unsigned char * scsi)
{
    struct dirent **BlockDisklist;
    int NumBlockDisk;
    char attribute[256];
    unsigned long long size = 0;

    memset(attribute, 0, sizeof(attribute));

#if RHEL_MAJOR == 5
    snprintf(attribute, 255, "/sys/class/scsi_device/%s/device/", scsi);
#else
    snprintf(attribute, 255, "/sys/class/scsi_device/%s/device/block/", scsi);
#endif

    if ((NumBlockDisk = scandir(attribute, &BlockDisklist,
                                sd_select, alphasort)) > 0) {
        size_t remaining = 255 - strlen(attribute);
        strncat(attribute, BlockDisklist[0]->d_name, remaining - 5);
        remaining = 255 - strlen(attribute);
        strncat(attribute, "/size", remaining);
        /* Size is in 512 block, need mmbytes so left shift 11 bits */
        size = get_sysfs_ullong(attribute);
        free(BlockDisklist[0]);
        free(BlockDisklist);
    } else {
        size_t remaining = 255 - strlen(attribute);
        strncat(attribute, "/block/", remaining);
        remaining = 255 - strlen(attribute);

        /* Look for block:sd? */
        if ((NumBlockDisk = scandir(attribute, &BlockDisklist,
                                    sd_select, alphasort)) > 0) {

            strncat(attribute,BlockDisklist[0]->d_name, remaining);
            remaining = 255 - strlen(attribute);
            strncat(attribute,  "/size", remaining);
            size = get_sysfs_ullong(attribute);
            free(BlockDisklist[0]);
            free(BlockDisklist);
        }
    }
    return size;
}

unsigned char *get_ScsiGeneric(unsigned char *scsi)
{
    char attribute[256];
    char *generic = NULL;
#if RHEL_MAJOR == 5
    int len;
    char buffer[256], lbuffer[256], *pbuffer;

    memset(attribute, 0, sizeof(attribute));
    snprintf(attribute, sizeof(attribute) - 1, "/sys/class/scsi_device/%s/device/generic", scsi);
    if ((len = readlink(attribute, lbuffer, 253)) > 0) {
        lbuffer[len]='\0'; /* Null terminate the string */
        pbuffer = basename(lbuffer);
        if (pbuffer != NULL)
            if ((generic = malloc(strlen(pbuffer) + 1)) != NULL)
                strcpy(generic, pbuffer);
    }
#else
    struct dirent **GenericDisklist;
    int NumGenericDisk;

    memset(attribute, 0, sizeof(attribute));
    snprintf(attribute, sizeof(attribute) -1,
             "/sys/class/scsi_device/%s/device/scsi_generic", scsi);

    NumGenericDisk = scandir(attribute, &GenericDisklist,
                                generic_select, alphasort);
    if (NumGenericDisk  > 0) {
        size_t gd_sz = strlen(GenericDisklist[0]->d_name);
        if ((generic = malloc(gd_sz + 1)) != NULL){
            memset(generic, 0, gd_sz + 1);
            strncpy(generic, GenericDisklist[0]->d_name, gd_sz);
        }
        free(GenericDisklist[0]);
        free(GenericDisklist);
    } else {
    }
#endif
    return (unsigned char *)generic;
}

sashba_config_buf * SasGetHbaConfig (int hbaIndex, char * driver)
{
    sashba_config_buf * hbaConfigBuf;
    char *buffer = NULL;
    int bufferSize = 16384;
    static int offset = -1;
    int fd = -1;

    if (offset < 0)
        offset = hbaIndex;

    if ((fd = open(driver, O_RDONLY)) == -1) {
        return (sashba_config_buf *) NULL;
    }

    if ((buffer = (char *)calloc(1, bufferSize)) != NULL) {
        memset(buffer, 0, bufferSize);
        hbaConfigBuf = (sashba_config_buf *)buffer;
    } else  {
        close(fd);
        return (sashba_config_buf *) NULL;
    }

    /* Setup the SCSI Request IO Control */
    hbaConfigBuf->IoctlHeader.Timeout      = SAS_TIMEOUT;
    hbaConfigBuf->IoctlHeader.ReturnCode   = 0;
    hbaConfigBuf->IoctlHeader.Length       = sizeof(sashba_config_buf) -
        sizeof(ioctl_hdr);
    hbaConfigBuf->IoctlHeader.Direction    = SAS_DATA_READ;
    hbaConfigBuf->IoctlHeader.IOControllerNumber = hbaIndex - offset;


    if ((ioctl(fd, SAS_GET_CNTLR_CONFIG, hbaConfigBuf) < 0)) {
        free(buffer);
        close(fd);
        return (sashba_config_buf *) NULL;
    } else {
        close(fd);
        return hbaConfigBuf;
    }
}

sas_connector_info_buf * SasGetHbaConnector (int hbaIndex, char * driver)
{
    sas_connector_info_buf * hbaConnectorBuf;
    char *buffer = NULL;
    int bufferSize = 16384;
    static int offset = -1;
    int fd = -1;

    if (offset < 0)
        offset = hbaIndex;

    if ((fd = open(driver, O_RDONLY)) == -1) {
        return (sas_connector_info_buf *) NULL;
    }

    if ((buffer = (char *)calloc(1, bufferSize)) != NULL) {
        memset(buffer, 0, bufferSize);
        hbaConnectorBuf = (sas_connector_info_buf *)buffer;
    } else  {
        close(fd);
        return (sas_connector_info_buf *) NULL;
    }

    /* Setup the SCSI Request IO Control */
    hbaConnectorBuf->IoctlHeader.Timeout      = SAS_TIMEOUT;
    hbaConnectorBuf->IoctlHeader.ReturnCode   = 0;
    hbaConnectorBuf->IoctlHeader.Length       = sizeof(sas_connector_info_buf) -
        sizeof(ioctl_hdr);
    hbaConnectorBuf->IoctlHeader.Direction    = SAS_DATA_READ;
    hbaConnectorBuf->IoctlHeader.IOControllerNumber = hbaIndex - offset;


    if ((ioctl(fd, SAS_GET_CONNECTOR_INFO, hbaConnectorBuf) < 0)) {
        free(buffer);
        close(fd);
        return (sas_connector_info_buf *) NULL;
    } else {
        close(fd);
        return hbaConnectorBuf;
    }
}

int get_unit_temp(int fd) 
{
    int scsiCmdLen = 10;
    unsigned char scsiCmd[10] = {LOG_SENSE, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    struct sg_io_hdr sgiob;
    unsigned char sense_buf[32];
    unsigned char response[4096];
    int resp_size = 0x62;
    int temp = -1;

    memset(&sense_buf, 0, sizeof(sense_buf));
    memset(&sgiob, 0, sizeof(sgiob));
    memset(&response, 0, sizeof(response));

    scsiCmd[2] = (unsigned char) 0x6f;
    scsiCmd[7] = (unsigned char)((resp_size >> 8) & 0xff);
    scsiCmd[8] = (unsigned char)(resp_size & 0xff);

    sgiob.interface_id = 'S';
    sgiob.cmdp = &scsiCmd[0];
    sgiob.cmd_len = scsiCmdLen;

    sgiob.timeout = 60000;

    sgiob.sbp = &sense_buf[0];
    sgiob.mx_sb_len = sizeof(sense_buf);

    sgiob.dxferp = &response;
    sgiob.dxfer_len = resp_size;
    sgiob.dxfer_direction = SG_DXFER_FROM_DEV;

    if (ioctl(fd, SG_IO, &sgiob) < 0) { 
        return -1;
    }
#ifdef UTTEST_SAS_DBG
    fprintf(stderr, "TEMP: ");
    fprintf(stderr, "%0x %0x %0x %0x %0x %0x %0x %0x %0x %0x %0x\n", 
                    response[0], response[1], response[2], response[3], response[4], response[5], response[6], response[7],
                    response[8], response[9], response[10]);
#endif
    if ((response[0] == 0x2f) && (response[10] != 0xff)){
        temp = response[10];
    }
    return temp;
}

int sata_parse_current(char *Temperature)
{
    if (Temperature[8] != 0x80) 
        return Temperature[8];
    else 
        return 0;
}

int sata_parse_mcot(char *Temperature)
{
    if (Temperature[88] != 0x80) 
        return Temperature[88];
    else 
        return 0;
}

char * get_sata_log(int fd, int log)
{
    int scsiCmdLen = 16;
    unsigned char scsiCmd[16] = 
            {ATA_PASS_THROUGH_16, PROTO_PIO|EXTEND, 0x0e, 
             0, 0, 0, 1, 
             0, DEV_STAT_LOG, 0, 0, 0, 0, 
             0, ATA_READ_LOG_EXT, 0};
    struct sg_io_hdr sgiob;
    unsigned char sense_buf[32];
    unsigned char response[512];
    int resp_size = 512;
    char *statlog = NULL;

    memset(&sense_buf, 0, sizeof(sense_buf));
    memset(&sgiob, 0, sizeof(sgiob));
    memset(&response, 0, sizeof(response));

    scsiCmd[10] = log;

    sgiob.interface_id = 'S';
    sgiob.cmdp = &scsiCmd[0];
    sgiob.cmd_len = scsiCmdLen;

    sgiob.timeout = 60000;

    sgiob.sbp = &sense_buf[0];;
    sgiob.mx_sb_len = sizeof(sense_buf);

    sgiob.dxferp = &response[0];
    sgiob.dxfer_len = resp_size;
    sgiob.dxfer_direction = SG_DXFER_FROM_DEV;

    if (ioctl(fd, SG_IO, &sgiob) < 0) {
        return NULL;
    }
    statlog = malloc(512);
    memcpy(statlog, &response[0], 512);
    return statlog;
}

char * get_sata_DiskRev(int fd)
{
    int scsiCmdLen = 16;
    unsigned char scsiCmd[16] =
            {ATA_PASS_THROUGH_16, PROTO_PIO, 0x0e,
             0, 0, 0, 1,
             0, 0, 0, 0, 0, 0,
             0, ATA_IDENTIFY_DEVICE, 0};
    struct sg_io_hdr sgiob;
    unsigned char sense_buf[32];
    unsigned char response[512];
    int resp_size = 512;
    char *sct_fwrev = NULL;

    memset(&sense_buf, 0, sizeof(sense_buf));
    memset(&sgiob, 0, sizeof(sgiob));
    memset(&response, 0, sizeof(response));

    sgiob.interface_id = 'S';
    sgiob.cmdp = &scsiCmd[0];
    sgiob.cmd_len = scsiCmdLen;

    sgiob.timeout = 60000;

    sgiob.sbp = &sense_buf[0];;
    sgiob.mx_sb_len = sizeof(sense_buf);

    sgiob.dxferp = &response[0];
    sgiob.dxfer_len = resp_size;
    sgiob.dxfer_direction = SG_DXFER_FROM_DEV;

    if (ioctl(fd, SG_IO, &sgiob) < 0) {
        return NULL;
    }
    sct_fwrev = malloc(9);
    sct_fwrev[0] = response[47];
    sct_fwrev[1] = response[46];
    sct_fwrev[2] = response[49];
    sct_fwrev[3] = response[48];
    sct_fwrev[4] = response[51];
    sct_fwrev[5] = response[50];
    sct_fwrev[6] = response[53];
    sct_fwrev[7] = response[52];
    sct_fwrev[8] = 0;
    return sct_fwrev;
}

char * get_sata_sct_stat(int fd)
{
    int scsiCmdLen = 16;
    unsigned char scsiCmd[16] = 
            {ATA_PASS_THROUGH_16, PROTO_PIO, 0x0e, 
             0, READ_SMART_LOG, 0, 1, 
             0, 0x0e, 0, 0x4f, 0, 0xc2, 
             0, ATA_SMART, 0};
    struct sg_io_hdr sgiob;
    unsigned char sense_buf[32];
    unsigned char response[512];
    int resp_size = 512;
    char *sct_stat = NULL;

    memset(&sense_buf, 0, sizeof(sense_buf));
    memset(&sgiob, 0, sizeof(sgiob));
    memset(&response, 0, sizeof(response));

    sgiob.interface_id = 'S';
    sgiob.cmdp = &scsiCmd[0];
    sgiob.cmd_len = scsiCmdLen;

    sgiob.timeout = 60000;

    sgiob.sbp = &sense_buf[0];;
    sgiob.mx_sb_len = sizeof(sense_buf);

    sgiob.dxferp = &response[0];
    sgiob.dxfer_len = resp_size;
    sgiob.dxfer_direction = SG_DXFER_FROM_DEV;

    if (ioctl(fd, SG_IO, &sgiob) < 0) {
        return NULL;
    }
    sct_stat = malloc(512);
    memcpy(sct_stat, &response[0], 512);
    return sct_stat;
}

char * get_sata_statlog(int fd)
{
    return (get_sata_log(fd, GEN_STAT_PAGE));
}

char * get_sata_ssdlog(int fd)
{
    return (get_sata_log(fd, SSD_STAT_PAGE));
}
    
char * get_sata_temp(int fd)
{
    return (get_sata_log(fd, TEMP_STAT_PAGE));
}

int get_sata_ssd_wear(int fd) 
{
    char *ssdlog = NULL;
    int wear = 255;
    
    ssdlog = get_sata_ssdlog(fd);
#ifdef UTTEST_SATA_DBG
    if (ssdlog != NULL) {
    fprintf(stderr, "\nWEAR: ");
    int s, e;
    e = 16;
    for (s = 0; s < e;s++)
        fprintf(stderr, "%0x ", ssdlog[s]);
    fprintf(stderr, "\n");
    }
#endif

    if (ssdlog != NULL) {
        wear = ssdlog[8];
        free (ssdlog);
    }
    return wear;
}

int get_sata_pwron(int fd) 
{
    int poweron = -1;
    char *statlog = NULL;

    statlog = get_sata_statlog(fd);
#ifdef UTTEST_SATA_DBG
    if (statlog != NULL) {
    fprintf(stderr, "\nPON: ");
    int s, e;
    e = 24;
    for (s = 0; s < e;s++)
        fprintf(stderr, "%0x ", statlog[s]);
    fprintf(stderr, "\n");
    }
#endif

    if (statlog != NULL) {
        poweron = (statlog[16] + (statlog[17] << 8) + 
                   (statlog[18] << 16 ) + (statlog[19] << 24 ));
        free (statlog);
    }
    return poweron;
}

char *get_sas_temp(int fd)
{
    int scsiCmdLen = 10;
    unsigned char scsiCmd[10] = {LOG_SENSE, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    struct sg_io_hdr sgiob;
    unsigned char sense_buf[32];
    unsigned char response[4096];
    int resp_size = 16;
    int len = 0;

    char *temp = NULL;

    memset(&sense_buf, 0, sizeof(sense_buf));
    memset(&sgiob, 0, sizeof(sgiob));
    memset(&response, 0, sizeof(response));

    scsiCmd[2] = (unsigned char) 0x4d;
    scsiCmd[7] = (unsigned char)((resp_size >> 8) & 0xff);
    scsiCmd[8] = (unsigned char)(resp_size & 0xff);

    sgiob.interface_id = 'S';
    sgiob.cmdp = &scsiCmd[0];
    sgiob.cmd_len = scsiCmdLen;

    sgiob.timeout = 60000;

    sgiob.sbp = &sense_buf[0];;
    sgiob.mx_sb_len = sizeof(sense_buf);

    sgiob.dxferp = &response[0];
    sgiob.dxfer_len = resp_size;
    sgiob.dxfer_direction = SG_DXFER_FROM_DEV;

    if (ioctl(fd, SG_IO, &sgiob) < 0) {
        return NULL;
    }
#ifdef UTTEST_SAS_DBG
    {
    fprintf(stderr, "\nTEMPERATURE: ");
    int s, e;
    e = 16;
    for (s = 0; s < e;s++) 
        fprintf(stderr, "%0x ", response[s]);
    fprintf(stderr, "\n");
    }
#endif
    len = response[ 3 ] + 4;
    temp = malloc(len);
    memset(temp, 0, len );
    memcpy(temp, &response[0], len);
    return temp;
}

char *get_identify_info(int fd)
{
    int scsiCmdLen = 6;
    unsigned char scsiCmd[6] = {INQUIRY, 0, 0, 0, 0, 0};

    struct sg_io_hdr sgiob;
    unsigned char sense_buf[32];
    unsigned char response[256];
    int resp_size = 0xfc;
    int len = 0;

    char *ident = NULL;

    memset(&sense_buf, 0, sizeof(sense_buf));
    memset(&sgiob, 0, sizeof(sgiob));
    memset(&response, 0, sizeof(response));

    scsiCmd[1] = (unsigned char) 0;
    scsiCmd[2] = (unsigned char) 0;
    scsiCmd[3] = (unsigned char)((resp_size >> 8) & 0xff);
    scsiCmd[4] = (unsigned char)(resp_size & 0xff);

    sgiob.interface_id = 'S';
    sgiob.cmdp = &scsiCmd[0];
    sgiob.cmd_len = scsiCmdLen;

    sgiob.timeout = 60000;

    sgiob.sbp = &sense_buf[0];;
    sgiob.mx_sb_len = sizeof(sense_buf);

    sgiob.dxferp = &response[0];
    sgiob.dxfer_len = resp_size;
    sgiob.dxfer_direction = SG_DXFER_FROM_DEV;

    if (ioctl(fd, SG_IO, &sgiob) < 0) {
        return NULL;
    }
#ifdef UTTEST_SAS_DBG
    {
    fprintf(stderr, "IDENTIFY: ");
    int s, e;
    e = 8;
    for (s = 0; s < e;s++) 
        fprintf(stderr, "%0x ", response[s]);
    fprintf(stderr, "\n");
    e = 16;
    for (s = 8; s < e;s++) 
        fprintf(stderr, "%c", response[s]);
    fprintf(stderr, "\n");
    e = 32;
    for (s = 16; s < e;s++) 
        fprintf(stderr, "%c", response[s]);
    fprintf(stderr, "\n");
    e = 36;
    for (s = 32; s < e;s++) 
        fprintf(stderr, "%c", response[s]);
    fprintf(stderr, "\n");
    e = 56;
    for (s = 36; s < e;s++) 
        fprintf(stderr, "%c", response[s]);
    fprintf(stderr, "\n");
    }
#endif
    len = response[ 4 ] + 4;
    ident = malloc(len);
    memset(ident, 0, len );
    memcpy(ident, &response[0], len);
    return ident;
}

unsigned long long  get_disk_capacity(int fd)
{
    int scsiCmdLen = 10;
    unsigned char scsiCmd[10] = {READ_CAPACITY, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    int scsiCmd2Len = 16;
    unsigned char scsiCmd2[16] = {SERVICE_ACTION_IN, READ_CAPACITY_16,
                                     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    struct sg_io_hdr sgiob;
    unsigned char sense_buf[32];
    unsigned char response[4096];
    int resp_size = 1024*4;
    unsigned long long capacity = 0;
    unsigned long long capacity2 = 0;

    memset(&sense_buf, 0, sizeof(sense_buf));
    memset(&sgiob, 0, sizeof(sgiob));
    memset(&response, 0, sizeof(response));

    sgiob.interface_id = 'S';
    sgiob.cmdp = &scsiCmd[0];
    sgiob.cmd_len = scsiCmdLen;

    sgiob.timeout = 60000;

    sgiob.sbp = &sense_buf[0];
    sgiob.mx_sb_len = sizeof(sense_buf);

    sgiob.dxferp = &response;
    sgiob.dxfer_len = resp_size;
    sgiob.dxfer_direction = SG_DXFER_FROM_DEV;

    if (ioctl(fd, SG_IO, &sgiob) < 0) {
        return 0;
    }
    capacity = ((unsigned long long)response[0] << 24) + 
               ((unsigned long long)response[1] << 16) + 
               ((unsigned long long)response[2] << 8) + 
                (unsigned long long)response[3];
    if (capacity != 0xFFFFFFFF)
        return capacity;
    else {
        memset(&sense_buf, 0, sizeof(sense_buf));
        memset(&sgiob, 0, sizeof(sgiob));
        memset(&response, 0, sizeof(response));

        sgiob.interface_id = 'S';
        sgiob.cmdp = &scsiCmd2[0];
        sgiob.cmd_len = scsiCmd2Len;
    
        sgiob.timeout = 60000;

        sgiob.sbp = &sense_buf[0];
        sgiob.mx_sb_len = sizeof(sense_buf);

        sgiob.dxferp = &response;
        sgiob.dxfer_len = resp_size;
        sgiob.dxfer_direction = SG_DXFER_FROM_DEV;

        if (ioctl(fd, SG_IO, &sgiob) < 0) {
            return 0;
        }

        capacity = ((unsigned long long)response[0] << 24) + 
                   ((unsigned long long)response[1] << 16) + 
                   ((unsigned long long)response[2] << 8) + 
                    (unsigned long long)response[3] ;
        capacity2 =((unsigned long long)response[4] << 24) + 
                   ((unsigned long long)response[5] << 16) + 
                   ((unsigned long long)response[6] << 8) + 
                    (unsigned long long)response[7] ; 
        return (capacity + (capacity2 << 32));
    }
}

int get_defect_data_size(int fd, unsigned char format)
{
    int scsiCmdLen = 10;
    unsigned char scsiCmd[10] = {READ_DEFECT_DATA, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    struct sg_io_hdr sgiob;
    unsigned char sense_buf[32];
    unsigned char response[4096];
    int resp_size = 1024*4;
    int list_sz = 0;
    int list_fmt = 0;

    memset(&sense_buf, 0, sizeof(sense_buf));
    memset(&sgiob, 0, sizeof(sgiob));
    memset(&response, 0, sizeof(response));

    scsiCmd[2] = (unsigned char) 0x08 | format;
    scsiCmd[7] = (unsigned char)((resp_size >> 8) & 0xff);
    scsiCmd[8] = (unsigned char)(resp_size & 0xff);

    sgiob.interface_id = 'S';
    sgiob.cmdp = &scsiCmd[0];
    sgiob.cmd_len = scsiCmdLen;

    sgiob.timeout = 60000;

    sgiob.sbp = &sense_buf[0];
    sgiob.mx_sb_len = sizeof(sense_buf);

    sgiob.dxferp = &response;
    sgiob.dxfer_len = resp_size;
    sgiob.dxfer_direction = SG_DXFER_FROM_DEV;

    if (ioctl(fd, SG_IO, &sgiob) < 0) {
        return 0;
    }
    list_fmt = response[1] & 0x07;
    list_sz = (response[2] << 8) + response[3];
    if (list_sz != 0) {
        switch (list_fmt) {
            case BLOCK:
            case VENDOR_SPECIFIC:
                return (list_sz / sizeof(BLOCK_DESCRIPTOR));
                break;
    
            case BYTES_FROM_INDEX:
                return(list_sz / sizeof(BYTES_FROM_INDEX_DESCRIPTOR));
                break;
    
            case PHYSICAL_SECTOR:
                return(list_sz / sizeof(PHYSICAL_SECTOR_DESCRIPTOR));
                break;

            default:  // Unknown format
                break;
       }
    }
    return 0;
}

int get_unit_speed(int fd) 
{
    int scsiCmdLen = 10;
    unsigned char scsiCmd[10] = {MODE_SENSE_10, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    struct sg_io_hdr sgiob;
    unsigned char sense_buf[32];
    unsigned char response[4096];
    int resp_size = 1024*4;

    memset(&sense_buf, 0, sizeof(sense_buf));
    memset(&sgiob, 0, sizeof(sgiob));
    memset(&response, 0, sizeof(response));

    scsiCmd[2] = (unsigned char) 4;
    scsiCmd[7] = (unsigned char)((resp_size >> 8) & 0xff);
    scsiCmd[8] = (unsigned char)(resp_size & 0xff);

    sgiob.interface_id = 'S';
    sgiob.cmdp = &scsiCmd[0];
    sgiob.cmd_len = scsiCmdLen;

    sgiob.timeout = 60000;

    sgiob.sbp = &sense_buf[0];
    sgiob.mx_sb_len = sizeof(sense_buf);

    sgiob.dxferp = &response;
    sgiob.dxfer_len = resp_size;
    sgiob.dxfer_direction = SG_DXFER_FROM_DEV;

    if (ioctl(fd, SG_IO, &sgiob) < 0) { 
        return 0;
    }
#ifdef UTTEST_SAS_DBG
    fprintf(stderr, "SPEED: ");
    fprintf(stderr, "%0x %0x %0x %0x %0x %0x %0x %0x %0x %0x %0x\n", 
                    response[0], response[1], response[2], response[3], response[4], response[5], response[6], response[7],
                    response[8], response[9], response[10]);
#endif
    return ( (response[37] << 8) + response[36]);
}


int is_unit_smart(int fd) 
{
    int scsiCmdLen = 6;
    unsigned char scsiCmd[10] = {MODE_SENSE, 0, 0, 0, 0, 0};
    struct sg_io_hdr sgiob;
    unsigned char sense_buf[32];
    unsigned char response[4096];
    int resp_size = 252;
    int ssd = 0;
    int i;
    int length = 0;

    memset(&sense_buf, 0, sizeof(sense_buf));
    memset(&sgiob, 0, sizeof(sgiob));
    memset(&response, 0, sizeof(response));

    scsiCmd[1] = (unsigned char) 0x08;
    scsiCmd[2] = (unsigned char) 0x1c;
    scsiCmd[4] = (unsigned char) 0xff;

    sgiob.interface_id = 'S';
    sgiob.cmdp = &scsiCmd[0];
    sgiob.cmd_len = scsiCmdLen;

    sgiob.timeout = 60000;

    sgiob.sbp = &sense_buf[0];
    sgiob.mx_sb_len = sizeof(sense_buf);

    sgiob.dxferp = &response;
    sgiob.dxfer_len = resp_size;
    sgiob.dxfer_direction = SG_DXFER_FROM_DEV;

    if (ioctl(fd, SG_IO, &sgiob) < 0) { 
        return -1;
    }
    length = (response[2]<<8) + response[3];

    if (length > 100) return ssd;
    for (i = 0; i < length; i++) {
        if (response[i + 4] == 0x11)
            return 1;
    }
    return ssd;
}

int is_unit_ssd(int fd) 
{
    int scsiCmdLen = 6;
    unsigned char scsiCmd[6] = {INQUIRY, 0, 0, 0, 0, 0};
    struct sg_io_hdr sgiob;
    unsigned char sense_buf[32];
    unsigned char response[256];
    int resp_size = 0xfc;

    memset(&sense_buf, 0, sizeof(sense_buf));
    memset(&sgiob, 0, sizeof(sgiob));
    memset(&response, 0, sizeof(response));

    scsiCmd[1] = (unsigned char) EVPD;
    scsiCmd[2] = (unsigned char) BDC_VPD;
    scsiCmd[3] = (unsigned char)((resp_size >> 8) & 0xff);
    scsiCmd[4] = (unsigned char)(resp_size & 0xff);

    sgiob.interface_id = 'S';
    sgiob.cmdp = &scsiCmd[0];
    sgiob.cmd_len = scsiCmdLen;

    sgiob.timeout = 60000;

    sgiob.sbp = &sense_buf[0];;
    sgiob.mx_sb_len = sizeof(sense_buf);

    sgiob.dxferp = &response[0];
    sgiob.dxfer_len = resp_size;
    sgiob.dxfer_direction = SG_DXFER_FROM_DEV;

    if (ioctl(fd, SG_IO, &sgiob) < 0) {
        return 0;
    }
#ifdef UTTEST_SAS_DBG
    {
    fprintf(stderr, "\nSSD: ");
    int s, e;
    e = 8;
    for (s = 0; s < e;s++)
        fprintf(stderr, "%0x ", response[s]);
    fprintf(stderr, "\n");
    }
#endif
    if (sgiob.status == 0 ) {
        if ((response[4] == 0) && (response[5] == 1 )) {
            return 1;
        }
    }
    return 0;
}

int get_sas_ssd_wear(int fd) 
{
    int scsiCmdLen = 10;
    unsigned char scsiCmd[10] = {LOG_SENSE, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    struct sg_io_hdr sgiob;
    unsigned char sense_buf[32];
    unsigned char response[4096];
    int resp_size = 16;
    int wear = 255;

    memset(&sense_buf, 0, sizeof(sense_buf));
    memset(&sgiob, 0, sizeof(sgiob));
    memset(&response, 0, sizeof(response));

    scsiCmd[2] = (unsigned char) 0x51;
    scsiCmd[7] = (unsigned char)((resp_size >> 8) & 0xff);
    scsiCmd[8] = (unsigned char)(resp_size & 0xff);

    sgiob.interface_id = 'S';
    sgiob.cmdp = &scsiCmd[0];
    sgiob.cmd_len = scsiCmdLen;

    sgiob.timeout = 60000;

    sgiob.sbp = &sense_buf[0];
    sgiob.mx_sb_len = sizeof(sense_buf);

    sgiob.dxferp = &response;
    sgiob.dxfer_len = resp_size;
    sgiob.dxfer_direction = SG_DXFER_FROM_DEV;

    if (ioctl(fd, SG_IO, &sgiob) < 0) { 
        return -1;
    }
#ifdef UTTEST_SAS_DBG
    {
    fprintf(stderr, "\nWEAR: ");
    int s, e;
    e = 12;
    for (s = 0; s < e;s++)
        fprintf(stderr, "%0x ", response[s]);
    fprintf(stderr, "\n");
    }
#endif
    if ((response[0] == 0x11) && (response[3] == 8)) {
        wear = response[11];
    }
    return wear;
}

int get_sas_pwron(int fd) 
{
    int scsiCmdLen = 10;
    unsigned char scsiCmd[10] = {LOG_SENSE, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    struct sg_io_hdr sgiob;
    unsigned char sense_buf[32];
    unsigned char response[4096];
    int resp_size = 32;

    memset(&sense_buf, 0, sizeof(sense_buf));
    memset(&sgiob, 0, sizeof(sgiob));
    memset(&response, 0, sizeof(response));

    scsiCmd[2] = (unsigned char) 0x55;
    scsiCmd[7] = (unsigned char)((resp_size >> 8) & 0xff);
    scsiCmd[8] = (unsigned char)(resp_size & 0xff);

    sgiob.interface_id = 'S';
    sgiob.cmdp = &scsiCmd[0];
    sgiob.cmd_len = scsiCmdLen;

    sgiob.timeout = 60000;

    sgiob.sbp = &sense_buf[0];
    sgiob.mx_sb_len = sizeof(sense_buf);

    sgiob.dxferp = &response;
    sgiob.dxfer_len = resp_size;
    sgiob.dxfer_direction = SG_DXFER_FROM_DEV;

    if (ioctl(fd, SG_IO, &sgiob) < 0) { 
        return -1;
    }
#ifdef UTTEST_SAS_DBG
    {
    fprintf(stderr, "\nPON: ");
    int s, e;
    e = 20;
    for (s = 0; s < e;s++)
        fprintf(stderr, "%0x ", response[s]);
    fprintf(stderr, "\n");
    }
#endif
    if (response[0] == 0x15) {
        return ((response[8] << 24) + (response[9] << 16) + 
                (response[10] << 8) + response[11])/60;
    }
    return -1;
}

int get_sata_health(int fd) 
{
    int scsiCmdLen = 16;
    unsigned char scsiCmd[16] = 
            {ATA_PASS_THROUGH_16, PROTO_PIO, 0x0e, 
             0, READ_SMART_LOG, 0, 1, 
             0, SUM_SMART_ERR_LOG, 0, 0x4f, 0, 0xc2, 
             0, ATA_SMART, 0};
    struct sg_io_hdr sgiob;
    unsigned char sense_buf[32];
    unsigned char response[4096];
    int resp_size = 512;
    int health = -1;

    memset(&sense_buf, 0, sizeof(sense_buf));
    memset(&sgiob, 0, sizeof(sgiob));
    memset(&response, 0, sizeof(response));

    sgiob.timeout = 20000;

    sgiob.sbp = &sense_buf[0];;
    sgiob.mx_sb_len = sizeof(sense_buf);

    sgiob.interface_id = 'S';
    sgiob.cmdp = &scsiCmd[0];
    sgiob.cmd_len = scsiCmdLen;

    sgiob.dxferp = &response;
    sgiob.dxfer_len = resp_size;
    sgiob.dxfer_direction = SG_DXFER_FROM_DEV;

    if (ioctl(fd, SG_IO, &sgiob) < 0) { 
        return -1;
    }
#if UTTEST_SAS_DBG || UTTEST_SATA_DBG
    {
    fprintf(stderr, "\nHEALTH: ");
    int s, e;
    e = 24;
    for (s = 0; s < e;s++)
        fprintf(stderr, "%0x ", response[s]);
    fprintf(stderr, "\n");
    }
#endif
    if (response[0] == 1) {
        return (response[1]);
    }
    return health;
}

int get_sas_health(int fd) 
{
    int scsiCmdLen = 10;
    unsigned char scsiCmd[10] = {LOG_SENSE, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    struct sg_io_hdr sgiob;
    unsigned char sense_buf[32];
    unsigned char response[4096];
    int resp_size = 0x62;
    int health = -1;

    memset(&sense_buf, 0, sizeof(sense_buf));
    memset(&sgiob, 0, sizeof(sgiob));
    memset(&response, 0, sizeof(response));

    scsiCmd[2] = (unsigned char) 0x6f;
    scsiCmd[7] = (unsigned char)((resp_size >> 8) & 0xff);
    scsiCmd[8] = (unsigned char)(resp_size & 0xff);

    sgiob.interface_id = 'S';
    sgiob.cmdp = &scsiCmd[0];
    sgiob.cmd_len = scsiCmdLen;

    sgiob.timeout = 60000;

    sgiob.sbp = &sense_buf[0];
    sgiob.mx_sb_len = sizeof(sense_buf);

    sgiob.dxferp = &response;
    sgiob.dxfer_len = resp_size;
    sgiob.dxfer_direction = SG_DXFER_FROM_DEV;

    if (ioctl(fd, SG_IO, &sgiob) < 0) { 
        return -1;
    }
#if UTTEST_SAS_DBG || UTTEST_SATA_DBG
    {
    fprintf(stderr, "\nHEALTH: ");
    int s, e;
    e = 24;
    for (s = 0; s < e;s++)
        fprintf(stderr, "%0x ", response[s]);
    fprintf(stderr, "\n");
    }
#endif
    if (response[0] == 0x2f) {
        health = (response[8] << 8) + response[9];
    }
    return health;
}

char *get_unit_sn(int fd) 
{
    int scsiCmdLen = 6;
    unsigned char scsiCmd[6] = {INQUIRY, 0, 0, 0, 0, 0};
    struct sg_io_hdr sgiob;
    unsigned char sense_buf[32];
    unsigned char response[256];
    int resp_size = 0xfc;
    int len = 0;
    char *serial = NULL;

    memset(&sense_buf, 0, sizeof(sense_buf));
    memset(&sgiob, 0, sizeof(sgiob));
    memset(&response, 0, sizeof(response));

    scsiCmd[1] = (unsigned char) 1;
    scsiCmd[2] = (unsigned char) 0x80;
    scsiCmd[3] = (unsigned char)((resp_size >> 8) & 0xff);
    scsiCmd[4] = (unsigned char)(resp_size & 0xff);

    sgiob.interface_id = 'S';
    sgiob.cmdp = &scsiCmd[0];
    sgiob.cmd_len = scsiCmdLen;

    sgiob.timeout = 60000;

    sgiob.sbp = &sense_buf[0];;
    sgiob.mx_sb_len = sizeof(sense_buf);

    sgiob.dxferp = &response[0];
    sgiob.dxfer_len = resp_size;
    sgiob.dxfer_direction = SG_DXFER_FROM_DEV;

    if (ioctl(fd, SG_IO, &sgiob) < 0) { 
        return NULL;
    }
    if (response[1] == 0x80) {
        int start = 4;
        len = (response[2] << 8) + response[3];
        serial = malloc(len+1);
        memset(serial, 0, len +1);
        while (response[start] == ' ') {
            start++;
            len--;
        }
        memcpy(serial, &response[start], len);
    }
    return serial;
}

int sas_parse_current(char *Temperature)
{
    if ((Temperature[4] == 0) && (Temperature[5] == 0))
        return Temperature[9];
    else 
        return 0;
}

int sas_parse_mcot(char *Temperature)
{
    if ((Temperature[10] == 0) && (Temperature[11] == 1))
        return Temperature[15];
    else 
        return 0;
}

unsigned char *inq_parse_vendor(char *Identify)
{
    int start = 8;
    int len = 8;
    int end = 15;
    char * buffer = (char *) 0;
    if (Identify != (char *) 0) {
        while (Identify[start] == ' ') {
            start++;
            len--;
        }
        while ((Identify[end] == ' ') || (Identify[end] == 0)) {
            end--;
            len--;
        }
        buffer = malloc(len + 1);
        memset(buffer, 0, len + 1);
        memcpy(buffer, &Identify[start], len);
        return ((unsigned char *) buffer);
    }
    return NULL;
}

unsigned char *inq_parse_prodID(char *Identify)
{
    int start = 16;
    int len = 16;
    int end = 31;
    char * buffer = (char *) 0;
    if (Identify != (char *) 0) {
        while (Identify[start] == ' ') {
            start++;
            len--;
        }
        while ((Identify[end] == ' ') || (Identify[end] == 0)) {
            end--;
            len--;
        }
        buffer = malloc(len + 1);
        memset(buffer, 0, len + 1);
        memcpy(buffer, &Identify[start], len);
        return ((unsigned char *) buffer);
    }
    return NULL;
}

unsigned char *inq_parse_rev(char *Identify)
{
    int start = 32;
    int len = 4;
    int end = 35;
    char * buffer = (char *) 0;
    if (Identify != (char *) 0) {
        while (Identify[start] == ' ') {
            start++;
            len--;
        }
        while ((Identify[end] == ' ') || (Identify[end] == 0)) {
            end--;
            len--;
        }
        buffer = malloc(len + 1);
        memset(buffer, 0, len + 1);
        memcpy(buffer, &Identify[start], len);
        return ((unsigned char *) buffer);
    }
    return NULL;
}

unsigned char inq_parse_type(char *Identify)
{
    if (Identify != (char *) 0)
        return (Identify[0] & 0x1f);
    return 0;
}

int get_DiskType(char *scsi)
{
    char attribute[256];

    memset(attribute, 0, sizeof(attribute));

    snprintf(attribute, sizeof(attribute) - 1, "/sys/class/scsi_device/%s/device/type", scsi);
    return(get_sysfs_uint(attribute));
}

char * get_DiskModel(char *scsi)
{
    char attribute[256];

    memset(attribute, 0, sizeof(attribute));

    snprintf(attribute, sizeof(attribute) - 1, "/sys/class/scsi_device/%s/device/model", scsi);
    return(get_sysfs_str(attribute));
}

char * get_sas_DiskRev(char *scsi)
{
    char attribute[256];

    memset(attribute, 0, sizeof(attribute));

    snprintf(attribute, sizeof(attribute) - 1, "/sys/class/scsi_device/%s/device/rev", scsi);
    return(get_sysfs_str(attribute));
}

char * get_DiskState(char *scsi)
{
    char attribute[256];

    memset(attribute, 0, sizeof(attribute));

    snprintf(attribute,  sizeof(attribute) - 1, "/sys/class/scsi_device/%s/device/state", scsi);
    return(get_sysfs_str(attribute));
}

#ifdef UTTEST
int main(int argc, char **argv) 
{
    char disk_name[256];
    int disk_fd;
    unsigned char * serialnum;
    unsigned char response[256];
    int Health, Speed, Temp, Mcot, Wear, Pon;
    unsigned long long Capacity;
    char *Vendor;
    char *ProductID;
    char *Rev;

    int drive_count = 4;
    int j=0;
    char *Identify;
    char *Temperature;
    /* Look for scsi_genric:sg? */
    if ((NumScsiGeneric = scandir(ScsiGenericDir, &ScsiGenericlist,
                    generic_select, alphasort))  > 0) {
        for (j = 0; j < NumScsiGeneric; j++) {
            memset(disk_name, 0, 256);
            snprintf(disk_name, sizeof(disk_name) - 1, "/dev/%s",ScsiGenericlist[j]->d_name);
            free(ScsiGenericlist[j]);
            if ((disk_fd = open(disk_name, O_RDWR | O_NONBLOCK)) < 0 )
            continue;

            if ((Identify = get_identify_info(disk_fd)) != NULL ) {
                fprintf(stderr, "%s: ", disk_name);
                if (inq_parse_type(Identify) == 0x0d) {
                    fprintf(stderr, " Enclosure\n\n");
                    continue;
                }
                if (inq_parse_type(Identify) == 0x0c) {
                    fprintf(stderr, " Controller\n\n");
                    continue;
                }
                if ((Vendor = inq_parse_vendor(Identify)) != NULL) {
                    fprintf(stderr,"Vendor=%s", Vendor);
                    free(Vendor);
                }
                if ((ProductID = inq_parse_prodID(Identify)) != NULL) {
                    fprintf(stderr," Product ID=%s", ProductID);
                    free(ProductID);
                }
                if ((Rev = inq_parse_rev(Identify)) != NULL) {
                    fprintf(stderr," FW rev=%s\n", Rev);
                    free(Rev);
               }
                free(Identify);
                if ((Capacity = get_disk_capacity(disk_fd)) > 0)
                    fprintf(stderr,"Drive capacity = %d\n", Capacity);
                if (is_unit_ssd(disk_fd)) {
                    Wear = get_sata_ssd_wear(disk_fd);
                    fprintf(stderr, " (SSD %d%% used)\n", Wear);
                } else
                    fprintf(stderr, "\n");
    
            }
            if (strncmp(Vendor, "ATA", 3) == 0) {
                if ((Temperature = get_sas_temp(disk_fd)) != NULL ) {
                    Temp = sas_parse_current(Temperature);
                    Mcot = sas_parse_mcot(Temperature);
                    fprintf(stderr, "Current Temp = %dC,  Maximum Temp = %dC\n", Temp, Mcot);
                }
                if ((Health = get_ssd_health(disk_fd)) != 0 )
                fprintf(stderr," Bad Health");
        else 
                fprintf(stderr," Good Health");

                if (is_unit_ssd(disk_fd)) {
                    Wear = get_sas_ssd_wear(disk_fd);
                    fprintf(stderr, " (SSD %d%% used)\n", Wear);
                } else
                fprintf(stderr, "\n");

            if ((serialnum = get_unit_sn(disk_fd)) < 0 ) {
            fprintf(stderr,"ioctl failed: %s\n", strerror(errno));
            continue;
        } else {
                fprintf(stderr, "S/N = %s\n",serialnum);
            free(serialnum);
        }

                Pon = get_sas_pwron(disk_fd);
                fprintf(stderr,"Power On Hours = %d\n", Pon);
    
                Temp = get_unit_temp(disk_fd);
                fprintf(stderr,"Drive temperature = %d\n", Temp);
    
            Speed = get_unit_speed(disk_fd);
            fprintf(stderr,"Drive speed = %d\n",Speed);

            fprintf(stderr, "\n");
            } else {
                if ((Health = get_sata_health(disk_fd)) != 0 )
                    fprintf(stderr," Bad Health(%d)", Health);
                else
                    fprintf(stderr," Good Health");
                if (is_unit_ssd(disk_fd)) {
                    Wear = get_sata_ssd_wear(disk_fd);
                    fprintf(stderr, " (SSD %d%% used)\n", Wear);
                } else
                    fprintf(stderr, "\n");
                Pon = get_sata_pwron(disk_fd);
                fprintf(stderr,"Power On Hours = %d\n", Pon);
    
            }
            close(disk_fd);
        }
        free(ScsiGenericlist);
    }

}
#endif

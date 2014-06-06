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

#ifdef UTTEST
extern  int alphasort();
static struct dirent **ScsiGenericlist;
static char * ScsiGenericDir = "/sys/class/scsi_generic/";
static int NumScsiGeneric;

static int generic_select(const struct dirent *entry)
{
    if (strncmp(entry->d_name,"sg", 2) == 0)
        return(1);
    return 0;
}

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

unsigned long long get_BlockSize(char * scsi)
{
    struct dirent **BlockDisklist;
    int NumBlockDisk;
    char attribute[256];
    unsigned long long size = 0;

    memset(attribute, 0, sizeof(attribute));

#if RHEL_MAJOR == 5
    sprintf(attribute, "/sys/class/scsi_device/%s/device/", scsi);
#else
    sprintf(attribute, "/sys/class/scsi_device/%s/device/block/", scsi);
#endif

    if ((NumBlockDisk = scandir(attribute, &BlockDisklist,
                                sd_select, alphasort)) > 0) {
        strcat(attribute, BlockDisklist[0]->d_name);
        strcat(attribute,"/size");
        /* Size is in 512 block, need mmbytes so left shift 11 bits */
        size = get_sysfs_ullong(attribute);
        free(BlockDisklist[0]);
        free(BlockDisklist);
    } else {
        strcat(attribute, "/block/");

        /* Look for block:sd? */
        if ((NumBlockDisk = scandir(attribute, &BlockDisklist,
                                    sd_select, alphasort)) > 0) {

            strcat(attribute,BlockDisklist[0]->d_name);
            strcat(attribute,  "/size");
            size = get_sysfs_ullong(attribute);
            free(BlockDisklist[0]);
            free(BlockDisklist);
        }
    }
    return size;
}

unsigned char *get_ScsiGeneric(char *scsi)
{
    char attribute[256];
    char *generic = NULL;
#if RHEL_MAJOR == 5
    int len;
    char buffer[256], lbuffer[256], *pbuffer;

    memset(attribute, 0, sizeof(attribute));
    sprintf(attribute, "/sys/class/scsi_device/%s/device/generic", scsi);
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
    sprintf(attribute, "/sys/class/scsi_device/%s/device/scsi_generic", scsi);

    NumGenericDisk = scandir(attribute, &GenericDisklist,
                                generic_select, alphasort);
    if (NumGenericDisk  > 0) {
        if ((generic = malloc(strlen(GenericDisklist[0]->d_name) + 1)) != NULL){
            strcpy(generic, GenericDisklist[0]->d_name);
        }
        free(GenericDisklist[0]);
        free(GenericDisklist);
    } else {
    }
#endif
    return (generic);
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
    int senseCmdLen = 10;
    unsigned char senseCmd[10] = {LOG_SENSE, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    struct sg_io_hdr sgiob;
    unsigned char sense_buf[32];
    unsigned char response[4096];
    int resp_size = 0x62;
    int temp = -1;

    memset(&sense_buf, 0, 32);
    memset(&sgiob, 0, sizeof(sgiob));
    memset(&response, 0, sizeof(response));

    senseCmd[2] = (unsigned char) 0x6f;
    senseCmd[7] = (unsigned char)((resp_size >> 8) & 0xff);
    senseCmd[8] = (unsigned char)(resp_size & 0xff);

    sgiob.interface_id = 'S';
    sgiob.cmdp = &senseCmd[0];
    sgiob.cmd_len = senseCmdLen;

    sgiob.timeout = 60000;

    sgiob.sbp = &sense_buf[0];
    sgiob.mx_sb_len = 32;

    sgiob.dxferp = &response;
    sgiob.dxfer_len = resp_size;
    sgiob.dxfer_direction = SG_DXFER_FROM_DEV;

    if (ioctl(fd, SG_IO, &sgiob) < 0) { 
        return -1;
    }
    if ((response[0] == 0x2f) && (response[10] != 0xff)){
        temp = response[10];
    }
    return temp;
}

char *get_identify_info(int fd)
{
    int inqCmdLen = 6;
    unsigned char inqCmd[6] = {INQUIRY, 0, 0, 0, 0, 0};
    struct sg_io_hdr sgiob;
    unsigned char sense_buf[32];
    unsigned char response[256];
    int resp_size = 0xfc;
    int len = 0;
    char *ident = NULL;

    memset(&sense_buf, 0, 32);
    memset(&sgiob, 0, sizeof(sgiob));
    memset(&response, 0, sizeof(response));

    inqCmd[1] = (unsigned char) 0;
    inqCmd[2] = (unsigned char) 0;
    inqCmd[3] = (unsigned char)((resp_size >> 8) & 0xff);
    inqCmd[4] = (unsigned char)(resp_size & 0xff);

    sgiob.interface_id = 'S';
    sgiob.cmdp = &inqCmd[0];
    sgiob.cmd_len = inqCmdLen;

    sgiob.timeout = 60000;

    sgiob.sbp = &sense_buf[0];;
    sgiob.mx_sb_len = 32;

    sgiob.dxferp = &response[0];
    sgiob.dxfer_len = resp_size;
    sgiob.dxfer_direction = SG_DXFER_FROM_DEV;

    if (ioctl(fd, SG_IO, &sgiob) < 0) {
        return NULL;
    }
    len = response[ 4 ] + 4;
    ident = malloc(len);
    memset(ident, 0, len );
    memcpy(ident, &response[0], len);
    return ident;
}

unsigned long long  get_disk_capacity(int fd)
{
    int senseCmdLen = 10;
    unsigned char senseCmd[10] = {READ_CAPACITY, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    int senseCmd2Len = 16;
    unsigned char senseCmd2[16] = {0x9E, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    struct sg_io_hdr sgiob;
    unsigned char sense_buf[32];
    unsigned char response[4096];
    int resp_size = 1024*4;
    unsigned long long capacity = 0;
    unsigned long long capacity2 = 0;

    memset(&sense_buf, 0, 32);
    memset(&sgiob, 0, sizeof(sgiob));
    memset(&response, 0, sizeof(response));

    sgiob.interface_id = 'S';
    sgiob.cmdp = &senseCmd[0];
    sgiob.cmd_len = senseCmdLen;

    sgiob.timeout = 60000;

    sgiob.sbp = &sense_buf[0];
    sgiob.mx_sb_len = 32;

    sgiob.dxferp = &response;
    sgiob.dxfer_len = resp_size;
    sgiob.dxfer_direction = SG_DXFER_FROM_DEV;

    if (ioctl(fd, SG_IO, &sgiob) < 0) {
        return 0;
    }
    capacity = ((unsigned long long)response[0] << 24) + ((unsigned long long)response[1] << 16) + 
               ((unsigned long long)response[2] << 8) + (unsigned long long)response[3];
    if (capacity != 0xFFFFFFFF)
        return capacity;
    else {
        memset(&sense_buf, 0, 32);
        memset(&sgiob, 0, sizeof(sgiob));
        memset(&response, 0, sizeof(response));

        sgiob.interface_id = 'S';
        sgiob.cmdp = &senseCmd2[0];
        sgiob.cmd_len = senseCmd2Len;
    
        sgiob.timeout = 60000;

        sgiob.sbp = &sense_buf[0];
        sgiob.mx_sb_len = 32;

        senseCmd2[1] = 0x10;

        sgiob.dxferp = &response;
        sgiob.dxfer_len = resp_size;
        sgiob.dxfer_direction = SG_DXFER_FROM_DEV;

        if (ioctl(fd, SG_IO, &sgiob) < 0) {
            return 0;
        }

        capacity = ((unsigned long long)response[0] << 24) + ((unsigned long long)response[1] << 16) + 
                   ((unsigned long long)response[2] << 8) + (unsigned long long)response[3] ;
        capacity2 =((unsigned long long)response[4] << 24) + ((unsigned long long)response[5] << 16) + 
                   ((unsigned long long)response[6] << 8) + (unsigned long long)response[7] ; 
        return (capacity + (capacity2 << 32));
    }
}
int get_defect_data_size(int fd)
{
    int senseCmdLen = 10;
    unsigned char senseCmd[10] = {READ_DEFECT_DATA, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    struct sg_io_hdr sgiob;
    unsigned char sense_buf[32];
    unsigned char response[4096];
    int resp_size = 1024*4;

    memset(&sense_buf, 0, 32);
    memset(&sgiob, 0, sizeof(sgiob));
    memset(&response, 0, sizeof(response));

    senseCmd[2] = (unsigned char) 0x0d;
    senseCmd[7] = (unsigned char)((resp_size >> 8) & 0xff);
    senseCmd[8] = (unsigned char)(resp_size & 0xff);

    sgiob.interface_id = 'S';
    sgiob.cmdp = &senseCmd[0];
    sgiob.cmd_len = senseCmdLen;

    sgiob.timeout = 60000;

    sgiob.sbp = &sense_buf[0];
    sgiob.mx_sb_len = 32;

    sgiob.dxferp = &response;
    sgiob.dxfer_len = resp_size;
    sgiob.dxfer_direction = SG_DXFER_FROM_DEV;

    if (ioctl(fd, SG_IO, &sgiob) < 0) {
        return 0;
    }
    return ( (response[2] << 8) + response[3]);

}

int get_unit_speed(int fd) 
{
    int senseCmdLen = 10;
    unsigned char senseCmd[10] = {MODE_SENSE_10, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    struct sg_io_hdr sgiob;
    unsigned char sense_buf[32];
    unsigned char response[4096];
    int resp_size = 1024*4;

    memset(&sense_buf, 0, 32);
    memset(&sgiob, 0, sizeof(sgiob));
    memset(&response, 0, sizeof(response));

    senseCmd[2] = (unsigned char) 4;
    senseCmd[7] = (unsigned char)((resp_size >> 8) & 0xff);
    senseCmd[8] = (unsigned char)(resp_size & 0xff);

    sgiob.interface_id = 'S';
    sgiob.cmdp = &senseCmd[0];
    sgiob.cmd_len = senseCmdLen;

    sgiob.timeout = 60000;

    sgiob.sbp = &sense_buf[0];
    sgiob.mx_sb_len = 32;

    sgiob.dxferp = &response;
    sgiob.dxfer_len = resp_size;
    sgiob.dxfer_direction = SG_DXFER_FROM_DEV;

    if (ioctl(fd, SG_IO, &sgiob) < 0) { 
        return 0;
    }
    return ( (response[37] << 8) + response[36]);
}


int is_unit_smart(int fd) 
{
    int senseCmdLen = 6;
    unsigned char senseCmd[10] = {MODE_SENSE, 0, 0, 0, 0, 0};
    struct sg_io_hdr sgiob;
    unsigned char sense_buf[32];
    unsigned char response[4096];
    int resp_size = 252;
    int ssd = 0;
    int i;
    int length = 0;

    memset(&sense_buf, 0, 32);
    memset(&sgiob, 0, sizeof(sgiob));
    memset(&response, 0, sizeof(response));

    senseCmd[1] = (unsigned char) 0x08;
    senseCmd[2] = (unsigned char) 0x1c;
    senseCmd[4] = (unsigned char) 0xff;

    sgiob.interface_id = 'S';
    sgiob.cmdp = &senseCmd[0];
    sgiob.cmd_len = senseCmdLen;

    sgiob.timeout = 60000;

    sgiob.sbp = &sense_buf[0];
    sgiob.mx_sb_len = 32;

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
    int inqCmdLen = 6;
    unsigned char inqCmd[6] = {INQUIRY, 0, 0, 0, 0, 0};
    struct sg_io_hdr sgiob;
    unsigned char sense_buf[32];
    unsigned char response[256];
    int resp_size = 0xfc;
    int len = 0;
    char *serial = NULL;

    memset(&sense_buf, 0, 32);
    memset(&sgiob, 0, sizeof(sgiob));
    memset(&response, 0, sizeof(response));

    inqCmd[1] = (unsigned char) 1;
    inqCmd[2] = (unsigned char) 0xb1;
    inqCmd[3] = (unsigned char)((resp_size >> 8) & 0xff);
    inqCmd[4] = (unsigned char)(resp_size & 0xff);

    sgiob.interface_id = 'S';
    sgiob.cmdp = &inqCmd[0];
    sgiob.cmd_len = inqCmdLen;

    sgiob.timeout = 60000;

    sgiob.sbp = &sense_buf[0];;
    sgiob.mx_sb_len = 32;

    sgiob.dxferp = &response[0];
    sgiob.dxfer_len = resp_size;
    sgiob.dxfer_direction = SG_DXFER_FROM_DEV;

    if (ioctl(fd, SG_IO, &sgiob) < 0) {
        return 0;
    }
    if (sgiob.status == 0 ) {
        if ((response[4] == 0) && (response[5] == 1 )) {
            return 1;
        }
    }
    return 0;
}

int get_unit_health(int fd) 
{
    int senseCmdLen = 10;
    unsigned char senseCmd[10] = {LOG_SENSE, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    struct sg_io_hdr sgiob;
    unsigned char sense_buf[32];
    unsigned char response[4096];
    int resp_size = 0x62;
    int health = -1;

    memset(&sense_buf, 0, 32);
    memset(&sgiob, 0, sizeof(sgiob));
    memset(&response, 0, sizeof(response));

    senseCmd[2] = (unsigned char) 0x6f;
    senseCmd[7] = (unsigned char)((resp_size >> 8) & 0xff);
    senseCmd[8] = (unsigned char)(resp_size & 0xff);

    sgiob.interface_id = 'S';
    sgiob.cmdp = &senseCmd[0];
    sgiob.cmd_len = senseCmdLen;

    sgiob.timeout = 60000;

    sgiob.sbp = &sense_buf[0];
    sgiob.mx_sb_len = 32;

    sgiob.dxferp = &response;
    sgiob.dxfer_len = resp_size;
    sgiob.dxfer_direction = SG_DXFER_FROM_DEV;

    if (ioctl(fd, SG_IO, &sgiob) < 0) { 
        return -1;
    }
    if (response[0] == 0x2f) {
        health = (response[8] << 8) + response[9];
    }
    return health;
}

char *get_unit_sn(int fd) 
{
    int inqCmdLen = 6;
    unsigned char inqCmd[6] = {INQUIRY, 0, 0, 0, 0, 0};
    struct sg_io_hdr sgiob;
    unsigned char sense_buf[32];
    unsigned char response[256];
    int resp_size = 0xfc;
    int len = 0;
    char *serial = NULL;

    memset(&sense_buf, 0, 32);
    memset(&sgiob, 0, sizeof(sgiob));
    memset(&response, 0, sizeof(response));

    inqCmd[1] = (unsigned char) 1;
    inqCmd[2] = (unsigned char) 0x80;
    inqCmd[3] = (unsigned char)((resp_size >> 8) & 0xff);
    inqCmd[4] = (unsigned char)(resp_size & 0xff);

    sgiob.interface_id = 'S';
    sgiob.cmdp = &inqCmd[0];
    sgiob.cmd_len = inqCmdLen;

    sgiob.timeout = 60000;

    sgiob.sbp = &sense_buf[0];;
    sgiob.mx_sb_len = 32;

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
        return (buffer);
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
        return (buffer);
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
        return (buffer);
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

    sprintf(attribute, "/sys/class/scsi_device/%s/device/type", scsi);
    return(get_sysfs_uint(attribute));
}

char * get_DiskModel(char *scsi)
{
    char attribute[256];

    memset(attribute, 0, sizeof(attribute));

    sprintf(attribute, "/sys/class/scsi_device/%s/device/model", scsi);
    return(get_sysfs_str(attribute));
}

char * get_DiskRev(char *scsi)
{
    char attribute[256];

    memset(attribute, 0, sizeof(attribute));

    sprintf(attribute, "/sys/class/scsi_device/%s/device/rev", scsi);
    return(get_sysfs_str(attribute));
}

char * get_DiskState(char *scsi)
{
    char attribute[256];

    memset(attribute, 0, sizeof(attribute));

    sprintf(attribute, "/sys/class/scsi_device/%s/device/state", scsi);
    return(get_sysfs_str(attribute));
}

#ifdef UTTEST

int main(int argc, char **argv) 
{
    char disk_name[256];
    int disk_fd;
    unsigned char * serialnum;
    unsigned char response[256];
    int Health, Speed;
    unsigned long long Capacity;
    char *Vendor;
    char *ProductID;
    char *Rev;

    int drive_count = 4;
    int j=0;
    char *Identify;
    /* Look for scsi_genric:sg? */
    if ((NumScsiGeneric = scandir(ScsiGenericDir, &ScsiGenericlist,
                    generic_select, alphasort))  > 0) {
        for (j = 0; j < NumScsiGeneric; j++) {
            memset(disk_name, 0, 256);
            sprintf(disk_name, "/dev/%s",ScsiGenericlist[j]->d_name);
            free(ScsiGenericlist[j]);
            if ((disk_fd = open(disk_name, O_RDWR | O_NONBLOCK)) < 0 )
            continue;

            fprintf(stderr, "%s: ", disk_name);
            if ((Identify = get_identify_info(disk_fd)) != NULL ) {
                if (inq_parse_type(Identify) == 0x0d) {
                    fprintf(stderr, " Enclosure\n\n");
                    continue;
                }
                if (inq_parse_type(Identify) == 0x0c) {
                    fprintf(stderr, " Controller\n\n");
                    continue;
                }
                if ((Vendor = inq_parse_vendor(Identify)) != NULL) {
                    fprintf(stderr," %s", Vendor);
                    free(Vendor);
                }
                if ((ProductID = inq_parse_prodID(Identify)) != NULL) {
                    fprintf(stderr," %s", ProductID);
                    free(ProductID);
                }
                if ((Rev = inq_parse_rev(Identify)) != NULL) {
                    fprintf(stderr," %s", Rev);
                    free(Rev);
               }
                free(Identify);
            }
            if ((Health = get_unit_health(disk_fd)) != 0 )
                fprintf(stderr," Bad Health");
        else 
                fprintf(stderr," Good Health");

            if (is_unit_ssd(disk_fd))
                fprintf(stderr, " (SSD)\n");
            else
                fprintf(stderr, "\n");

            if ((serialnum = get_unit_sn(disk_fd)) < 0 ) {
            fprintf(stderr,"ioctl failed: %s\n", strerror(errno));
            continue;
        } else {
                fprintf(stderr, "S/N = %s\n",serialnum);
            free(serialnum);
        }

            Speed = get_unit_speed(disk_fd);
            fprintf(stderr,"Drive speed = %d\n",Speed);
            if ((Capacity = get_disk_capacity(disk_fd)) > 0)
                fprintf(stderr,"Drive capacity = %d\n", Capacity);

            fprintf(stderr, "\n");
            close(disk_fd);
        }
        free(ScsiGenericlist);

    }

}
#endif

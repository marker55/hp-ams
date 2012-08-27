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
#include "scsi_info.h"

sashba_config_buf * SasGetHbaConfig (int hbaIndex, char * driver)
{
    sashba_config_buf * hbaConfigBuf;
    char *buffer = NULL;
    int bufferSize = 16384;
    int fd = -1;

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
    hbaConfigBuf->IoctlHeader.IOControllerNumber = hbaIndex;

    if ((ioctl(fd, SAS_GET_CNTLR_CONFIG, hbaConfigBuf) < 0)) {
        free(buffer);
        close(fd);
        return (sashba_config_buf *) NULL;
    } else {
        close(fd);
        return hbaConfigBuf;
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


int get_unit_health(int fd) 
{
    int senseCmdLen = 10;
    unsigned char senseCmd[10] = {LOG_SENSE, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    struct sg_io_hdr sgiob;
    unsigned char sense_buf[32];
    unsigned char response[4096];
    int resp_size = 0x62;
    int health = 0;

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
        len = (response[2] << 8) + response[3];
        serial = malloc(len);
        memcpy(serial, &response[4], len);
    }
    return serial;
}

#ifdef UTTEST

int main(int argc, char **argv) 
{
    char *drives[] = {"/dev/sdb",
        "/dev/sdc",
        "/dev/sdd",
        "/dev/sde",
        "",
    };
    int drv_fd[4];
    unsigned char * serialnum;
    unsigned char response[256];
    int Health, Speed;

    int drive_count = 4;
    int j=0;
    for (j = 0; j < drive_count; j++) {
        if ((drv_fd[j] = open(drives[j], O_RDWR | O_NONBLOCK)) < 0 ) {
            continue;
        }


        if ((Health = get_unit_health(drv_fd[j])) != 0 ) 
            fprintf(stderr,"Bad Health\n");
        else 
            fprintf(stderr,"Good Health\n");

        Speed = get_unit_speed(drv_fd[j]);
        fprintf(stderr,"Drive speed = %d\n",Speed);

        if ((serialnum = get_unit_sn(drv_fd[j])) < 0 ) {
            fprintf(stderr,"ioctl failed: %s\n", strerror(errno));
            continue;
        } else {
            fprintf(stderr, "%s\n",serialnum);
            free(serialnum);
        }

    }

}
#endif

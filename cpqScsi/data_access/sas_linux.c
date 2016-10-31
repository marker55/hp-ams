#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <net-snmp/library/container.h>
#include <net-snmp/library/snmp_debug.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/errno.h>
#include <getopt.h>
#include <linux/socket.h>
#include <linux/version.h>

#include "common/scsi_info.h"

#include "cpqScsi.h"
#include "cpqSasHbaTable.h"
#include "cpqSasPhyDrvTable.h"

#include "sashba.h"
#include "sasphydrv.h"
#include "common/smbios.h"
#include "common/utils.h"
#include "common/nl_udev.h"
#include <linux/netlink.h>

#include "sas_linux.h"

extern unsigned char     cpqHoMibHealthStatusArray[];
extern int trap_fire;
extern int pcislot_scsi_host(char * buffer);

extern int file_select(const struct dirent *);
void SendSasTrap(int, 
        cpqSasPhyDrvTable_entry *);
void netsnmp_arch_sashba_init(void); 
void netsnmp_arch_sasphydrv_init(void);

extern  int alphasort();

static struct dirent **SasHostlist;
static char * SasHostDir = "/sys/class/sas_host/";
static int NumSasHost;

static struct dirent **ScsiHostlist;
static char * ScsiHostDir = "/sys/class/scsi_host/";
static int NumScsiHost;

static struct dirent **SasDevicelist;
static char * SasDeviceDir = "/sys/class/sas_device/";
static int NumSasDevice;

static char *current_Hba;

static int target_select(const struct dirent *entry)
{
    if (strncmp(entry->d_name,"target", 6) == 0)
        return(1);
    return 0;
}

static int phy_select(const struct dirent *entry)
{
    if (strncmp(entry->d_name,"phy-", 4) == 0)
        return(1);
    return 0;
}

static int end_device_select(const struct dirent *entry)
{
    if (strncmp(entry->d_name,"end_device-", 11) == 0)
        return(1);
    return 0;
}

static int hba_select(const struct dirent *entry)
{
    int      i = 0;
    for (i = 0; i < NumSasHost; i++) {
        if ((strlen(entry->d_name) == strlen(SasHostlist[i]->d_name)) && 
            (!strncmp(entry->d_name,
                    SasHostlist[i]->d_name,
                    strlen(SasHostlist[i]->d_name))))
            return(1);
    }
    return 0;
}

char * get_EndDevice(char *deviceLink) 
{
    char *token = NULL;
    char *enddevice = NULL;
    int name_len = 0;
    DEBUGMSGTL(("sasphydrv:container:load", "get_EndDevice devicelink = %s\n", deviceLink));
    if ((deviceLink = strstr(deviceLink, "end_device-")) != NULL) {
        if ((token = index(deviceLink, '/')) != NULL) 
            name_len = (int)(token - deviceLink) + 1;
         else 
            name_len = strlen(deviceLink) + 1;
        enddevice = malloc(name_len);
        if (enddevice != NULL) {
            memset(enddevice, 0, name_len); 
            strncpy(enddevice, deviceLink, name_len - 1);
        }
    }
    DEBUGMSGTL(("sasphydrv:container:load", "get_EndDevice %s\n", enddevice));
    return (enddevice);
}

int get_Host(char *deviceLink) 
{
    int Host;
    while (deviceLink != NULL ) {
        if ((deviceLink = strstr(deviceLink, "host")) != NULL) {
            sscanf(deviceLink, "host%d", &Host);
            DEBUGMSGTL(("sasphydrv:container:load", "get_Host exit\n"));
            return Host;
        }
    }
    return -1;
 }

int get_Expander(char *deviceLink) 
{
    int Exp = -1;
    int Cntlr = 0;

    while (deviceLink != NULL ) {
        if ((deviceLink = strstr(deviceLink, "expander-")) != NULL) {
            sscanf(deviceLink, "expander-%d:%d", &Cntlr, &Exp);
            deviceLink++;
        }
    }
    DEBUGMSGTL(("sasphydrv:container:load", "get_Expander %d\n", Exp));
    return Exp;
 }

int get_PortID(char *deviceLink) 
{
    char *devlink;
    char *slashdev;
    int Port = -1;
    int Cntlr = -1;
    struct dirent **Phylist;
    int NumPhy,k;
    

    devlink = deviceLink;
    while (devlink != NULL ) {
        if ((devlink = strstr(devlink, "port-")) != NULL) {
            if ((slashdev = strstr(devlink, "/")) != NULL) 
                *slashdev = 0;
            DEBUGMSGTL(("sasphydrv:container:load","get_PortID %s\n", devlink));
            NumPhy = scandir(devlink, &Phylist, phy_select, alphasort);
            if (NumPhy  > 0) {
                for (k = 0; k < NumPhy; k++) {
                    sscanf(Phylist[k]->d_name,"phy-%d:%d",
                           &Cntlr,
                           &Port);
                     free(Phylist[k]);
                }
                free(Phylist);
	        }
            return Port;
        }
    }
    return -1;
}

int get_Cntlr(char *deviceLink) 
{
    int Exp;
    int Cntlr = 0;
    char *devlink;

    devlink = deviceLink;
    while (devlink != NULL ) {
        if ((devlink = strstr(devlink, "expander-")) != NULL) {
            sscanf(devlink, "expander-%d:%d", &Cntlr, &Exp);
            DEBUGMSGTL(("sasphydrv:container:load", "get_Cntlr %d\n", Cntlr));
            return Cntlr;
        }
    }

    devlink = deviceLink;
    while (devlink != NULL ) {
        if ((devlink = strstr(devlink, "end_device-")) != NULL) {
            sscanf(devlink, "end_device-%d:%d", &Cntlr, &Exp);
            DEBUGMSGTL(("sasphydrv:container:load", "get_Cntlr %d\n", Cntlr));
            return Cntlr;
        }
    }

    return -1;
} 

int get_BoxID(char *deviceLink) 
{
    int BoxID = 0;
    while (deviceLink != NULL ) {
        if ((deviceLink = strstr(deviceLink, "expander-")) != NULL) {
            deviceLink++;
            BoxID++;
        }
    }
    DEBUGMSGTL(("sasphydrv:container:load", "get_BoxID %d\n", BoxID));
    return BoxID;
 }

int get_ExpPhyID(char *deviceLink) 
{
    int Cntlr = 0;
    int Exp = -1;
    char attribute[256];

    //while (deviceLink != NULL ) {
        if ((deviceLink = strstr(deviceLink, "expander-")) != NULL) {
            if (Exp == -1)  {
                sscanf(deviceLink, "expander-%d:%d", &Cntlr, &Exp);
                /* use /sys/class/sas_device/expander- */
                memset(attribute, 0, sizeof(attribute));
                sprintf(attribute, "%sexpander-%d:%d/phy_identifier", 
                                    SasDeviceDir, Cntlr , Exp);
                DEBUGMSGTL(("sasphydrv:container:load", "get phyID from = %s\n", attribute));
                DEBUGMSGTL(("sasphydrv:container:load", "get_ExpPhyID\n"));
                return (get_sysfs_int(attribute));
            }
        //    deviceLink++;
        }
    //}
    return -1;
 }

int get_DevBayID(char *d_name)
{
    char attribute[256];

    memset(attribute, 0, sizeof(attribute));
    sprintf(attribute, "%s%s/bay_identifier", SasDeviceDir, d_name);
    DEBUGMSGTL(("sasphydrv:container:load", "get_DevBayID\n"));
    return (get_sysfs_int(attribute));
}

int get_DevPhyID(char *d_name)
{
    char attribute[256];

    memset(attribute, 0, sizeof(attribute));
    sprintf(attribute, "%s%s/phy_identifier", SasDeviceDir, d_name);
    DEBUGMSGTL(("sasphydrv:container:load", "get_DevPhyID\n"));
    return (get_sysfs_int(attribute));
}

char * get_DevProto(char *d_name)
{
    char attribute[256];

    memset(attribute, 0, sizeof(attribute));
    sprintf(attribute, "%s%s/target_port_protocols", SasDeviceDir, d_name);
    DEBUGMSGTL(("sasphydrv:container:load", "get_DevProto\n"));
    return (get_sysfs_str(attribute));
}

char * get_ScsiLink(char * deviceLink)
{
    int scsiCntlr = -1;
    int scsiBus = -1;
    int scsiTarget = -1;
    char *scsi = NULL;

    while (deviceLink != NULL ) {
        if ((deviceLink = strstr(deviceLink, "target")) != NULL) {
            sscanf(deviceLink, "target%d:%d:%d",
                        &scsiCntlr,
                        &scsiBus,
                        &scsiTarget);
            deviceLink++;
        }
    }
    if (scsiCntlr < 0) 
        return NULL;
    scsi = malloc(24);
    memset(scsi, 0, 24);
    sprintf(scsi, "%d:%d:%d:0", scsiCntlr, scsiBus, scsiTarget);
    DEBUGMSGTL(("sasphydrv:container:load", "get_ScsiLink scsi = %s\n", scsi));
    return scsi;
}

char * get_ScsiDname(char * d_name)
{
    struct dirent **Targetlist;
    int NumTarget;
    int k;
    int scsiCntlr = -1;
    int scsiBus = -1;
    int scsiTarget = -1;
    char *scsi = NULL;

    char attribute[256];

    memset(attribute, 0, sizeof(attribute));
    sprintf(attribute, "%s%s/device", SasDeviceDir, d_name);
            /* use /sys/class/sas_device/sas_end_device-/device to get target */
    DEBUGMSGTL(("sasphydrv:container:load", "target = %s\n", attribute));
    NumTarget = scandir(attribute, &Targetlist, target_select, alphasort);
    for (k = 0; k < NumTarget; k++) {
        sscanf(Targetlist[k]->d_name,"target%d:%d:%d",
                        &scsiCntlr,
                        &scsiBus,
                        &scsiTarget);
        free(Targetlist[k]);
    }
    free(Targetlist);

    DEBUGMSGTL(("sasphydrv:container:load", "get_ScsiDname\n"));
    if (scsiCntlr < 0) 
        return NULL;
    scsi = malloc(24);
    memset(scsi, 0, 24);
    sprintf(scsi, "%d:%d:%d:0", scsiCntlr, scsiBus, scsiTarget);
    return scsi;
}

char *get_ScsiSasAddress(char *scsi)
{
    char *value;
    char attribute[256];

    memset(attribute, 0, sizeof(attribute));

    sprintf(attribute, "/sys/class/scsi_device/%s/device/sas_address", scsi);
    value = get_sysfs_str(attribute);
    DEBUGMSGTL(("sasphydrv:container:load", "get_ScsiSasAddress %s\n",value));
    return(value);
}

cpqSasPhyDrvTable_entry *sas_add_disk(char *deviceLink, 
                                      cpqSasHbaTable_entry *hba,
                                      netsnmp_container* container)
{
    char *dname;
    int Host = -1;
    int Exp = -1;
    int PhyID = 0;
    int BoxID = 0;
    int BayID = 0;
    char *protocol = NULL;
    char *scsi;
    int scsiCntlr = -1;
    int scsiBus = -1;
    int scsiTarget = -1;
    netsnmp_index tmp;
    oid oid_index[2];
    cpqSasPhyDrvTable_entry *old;
    cpqSasPhyDrvTable_entry *disk;
    char *value;
    char *generic;
    int disk_fd;
    char *serialnum = NULL;
    int Health = 0, Speed = 0, Temp = -1, Mcot = -1, Wear = -1;
    char *Temperature;
    //int j, k;
    //long  rc = 0;

    if ((dname = get_EndDevice(deviceLink)) == NULL)
        return NULL;
    DEBUGMSGTL(("sasphydrv:container:load", "\nAdding new disk %s\n", dname));
    Host = get_Host(deviceLink);
    if ((Exp = get_Expander(deviceLink)) >= 0) {
    /* use /sys/class/sas_device/expander- */
        PhyID = get_ExpPhyID(deviceLink);
        BoxID = get_BoxID(deviceLink);
    } else
        PhyID = get_DevPhyID(deviceLink);

    if (BoxID == 0) 
        if (PhyID > 3)
            BayID = (PhyID % 4) + 1;
        else
            BayID = PhyID  + 5;
    else 
        BayID = get_DevBayID(dname);

    protocol = get_DevProto(dname);
    
    if ((scsi = get_ScsiLink(deviceLink)) == NULL) 
        if ((scsi = get_ScsiDname(dname)) == NULL) {
            free(dname);
            free(protocol);
            return NULL;
        }

    if (get_DiskType(scsi)) {
        free(dname);
        free(protocol);
        free(scsi);
        return NULL;
    }
    free(dname);

    sscanf(scsi, "%d:%d:%d:0", &scsiCntlr, &scsiBus, &scsiTarget);
    oid_index[0] = scsiCntlr;
    oid_index[1] = scsiTarget;
    tmp.len = 2;
    tmp.oids = &oid_index[0];

    old = CONTAINER_FIND(container, &tmp);
    DEBUGMSGTL(("sasphydrv:container:load", "Old disk=%p\n", old));
    if (old != NULL ) {
        DEBUGMSGTL(("sasphydrv:container:load", "Re-Use old entry\n"));
        old->OldStatus = old->cpqSasPhyDrvStatus;
        disk = old;
    } else {
        disk = cpqSasPhyDrvTable_createEntry(container,
                (oid)Host, 
                (oid)scsiTarget);
        if (disk == NULL) {
            free(scsi);
            return disk;
        }
        disk->hba = hba;
        DEBUGMSGTL(("sasphydrv:container:load", "Entry created\n"));

        disk->cpqSasPhyDrvSSDPercntEndrnceUsed = -1;
        disk->cpqSasPhyDrvSSDWearStatus = SSD_WEAR_OTHER;
        disk->cpqSasPhyDrvPowerOnHours = -1;
        disk->cpqSasPhyDrvSSDEstTimeRemainingHours = -1;
        disk->cpqSasPhyDrvAuthenticationStatus = 1;

        if (Exp >=0) {
            disk->cpqSasPhyDrvLocationString_len =
                sprintf(disk->cpqSasPhyDrvLocationString,
                        "Port %sBox %d Bay %d",
                        hba->Reference[PhyID].bConnector, BoxID, BayID);
            disk->cpqSasPhyDrvPlacement = SAS_PHYS_DRV_PLACE_EXTERNAL;
        } else {
            /* Check to see if we were able to get CSMI connector info */
            if (strlen((char *)hba->Reference[PhyID].bConnector))
                disk->cpqSasPhyDrvLocationString_len =
                    sprintf(disk->cpqSasPhyDrvLocationString,
                            "Port %sBay %d",
                            hba->Reference[PhyID].bConnector, BayID);
            else
                disk->cpqSasPhyDrvLocationString_len =
                    sprintf(disk->cpqSasPhyDrvLocationString,
                            "Port %dI Bay %d",
                            (PhyID / 4) + 1, BayID);

            disk->cpqSasPhyDrvPlacement = SAS_PHYS_DRV_PLACE_INTERNAL;
	    }

        if ((value = get_DiskModel(scsi)) != NULL) {
            if (!strncmp(protocol, "sata", 4))
                strcpy(disk->cpqSasPhyDrvModel,"ATA     ");
            else
                strcpy(disk->cpqSasPhyDrvModel,"HP      ");
            strncat(disk->cpqSasPhyDrvModel, value, sizeof(disk->cpqSasPhyDrvModel)-strlen(disk->cpqSasPhyDrvModel));
            disk->cpqSasPhyDrvModel_len =
                strlen(disk->cpqSasPhyDrvModel);
            free(value);
        }

        if ((value = get_sas_DiskRev(scsi)) != NULL) {
            strncpy(disk->cpqSasPhyDrvFWRev, value, sizeof(disk->cpqSasPhyDrvFWRev) - 1);
            disk->cpqSasPhyDrvFWRev_len = 
                                strlen(disk->cpqSasPhyDrvFWRev);
            free(value);
        }

        if ((value = get_DiskState(scsi)) != NULL) {
            if (strcmp(value, "running") == 0)
                disk->cpqSasPhyDrvStatus = SAS_PHYS_STATUS_OK;
            free(value);
        }

        if ((value = get_ScsiSasAddress(scsi)) != NULL) {
            strncpy(disk->cpqSasPhyDrvSasAddress, value + 2, sizeof(disk->cpqSasPhyDrvSasAddress) - 1);	
            disk->cpqSasPhyDrvSasAddress_len = 
                         strlen(disk->cpqSasPhyDrvSasAddress);
            free(value);
	    }

        disk->cpqSasPhyDrvSize = get_BlockSize((unsigned char *)scsi) >> 11;

        generic = (char *)get_ScsiGeneric((unsigned char *)scsi);
        if (generic != NULL) {
            memset(disk->cpqSasPhyDrvOsName, 0, 256);
            disk->cpqSasPhyDrvOsName_len = 
                                sprintf(disk->cpqSasPhyDrvOsName, 
                                        "/dev/%s", 
                                        generic);
            free(generic);
        } 
        DEBUGMSGTL(("sasphydrv:container:load", "generic dev is %s\n",
                disk->cpqSasPhyDrvOsName));
    }  /* New Entry */


    disk->cpqSasPhyDrvType = 2;
    if (protocol != NULL) {
        if (!strncmp(protocol, "sata", 4))
            disk->cpqSasPhyDrvType = 3;
        free(protocol);
    }

    disk->cpqSasPhyDrvMediaType = PHYS_DRV_ROTATING_PLATTERS;
    disk_fd = open(disk->cpqSasPhyDrvOsName, O_RDWR | O_NONBLOCK);
    if (disk_fd >= 0) {
        if ((serialnum = get_unit_sn(disk_fd)) != NULL  ) {
            memset(disk->cpqSasPhyDrvSerialNumber, 0, 41);
            strncpy(disk->cpqSasPhyDrvSerialNumber, serialnum, 
                    sizeof(disk->cpqSasPhyDrvSerialNumber) - 1);
            disk->cpqSasPhyDrvSerialNumber_len = 
                strlen(disk->cpqSasPhyDrvSerialNumber);
            free(serialnum);
        }

        disk->cpqSasPhyDrvRotationalSpeed = CPQ_REG_OTHER;
        if ((Speed = get_unit_speed(disk_fd)) >= 0) { 
            if (Speed > ROT_SPEED_7200_MIN)
                disk->cpqSasPhyDrvRotationalSpeed++;
            if (Speed > ROT_SPEED_10K_MIN)
                disk->cpqSasPhyDrvRotationalSpeed++;
            if (Speed > ROT_SPEED_15K_MIN)
                disk->cpqSasPhyDrvRotationalSpeed++;
        }

        if (disk->cpqSasPhyDrvType == 3)
            disk->cpqSasPhyDrvPowerOnHours = get_sata_pwron(disk_fd);
        else
            disk->cpqSasPhyDrvPowerOnHours = get_sas_pwron(disk_fd);

        if (is_unit_ssd(disk_fd)) {
            disk->cpqSasPhyDrvRotationalSpeed = SAS_PHYS_DRV_ROT_SPEED_SSD;
            disk->cpqSasPhyDrvMediaType = PHYS_DRV_SOLID_STATE;
            if (disk->cpqSasPhyDrvType == 3)
                Wear = get_sata_ssd_wear(disk_fd);
            else
                Wear = get_sas_ssd_wear(disk_fd);
            disk->cpqSasPhyDrvSSDPercntEndrnceUsed = Wear;
            disk->cpqSasPhyDrvSSDWearStatus = SSD_WEAR_OK;
            DEBUGMSGTL(("sasphydrv:container:load", "Wear is %d%%\n", Wear));
            if (disk->cpqSasPhyDrvSSDPercntEndrnceUsed >= 95)
                disk->cpqSasPhyDrvSSDWearStatus = SSD_WEAR_5PERCENT_LEFT;
            if (disk->cpqSasPhyDrvSSDPercntEndrnceUsed >= 98)
                disk->cpqSasPhyDrvSSDWearStatus = SSD_WEAR_2PERCENT_LEFT;
            if (disk->cpqSasPhyDrvSSDPercntEndrnceUsed >= 100)
                disk->cpqSasPhyDrvSSDWearStatus = SSD_WEAR_OUT;
        }

        if (disk->cpqSasPhyDrvType == 3) {
            if ((Temperature = get_sata_temp(disk_fd)) != NULL ) {
                Temp = sata_parse_current(Temperature);
                Mcot = sata_parse_mcot(Temperature);
                disk->cpqSasPhyDrvCurrTemperature = Temp;
                disk->cpqSasPhyDrvTemperatureThreshold = Mcot;
                free(Temperature);
            } 

            if ((Health = get_sata_health(disk_fd)) >= 0) {
                if (((Health != 0) && (Wear >= 95)) || (Temp > Mcot)) {
                    disk->cpqSasPhyDrvStatus = SAS_PHYS_STATUS_PREDICTIVEFAILURE;
                } else {
                    disk->cpqSasPhyDrvStatus = SAS_PHYS_STATUS_OK;
                }
            } else {
                disk->cpqSasPhyDrvStatus = SAS_PHYS_STATUS_FAILED;
            }
        } else {
            if ((Temperature = get_sas_temp(disk_fd)) != NULL ) {
                Temp = sas_parse_current(Temperature);
                Mcot = sas_parse_mcot(Temperature);
                disk->cpqSasPhyDrvCurrTemperature = Temp;
                disk->cpqSasPhyDrvTemperatureThreshold = Mcot;
                free(Temperature);
            } else {
                if ((Temp = get_unit_temp(disk_fd)) >= 0) 
                    disk->cpqSasPhyDrvCurrTemperature = Temp;
            }

            if ((Health = get_sas_health(disk_fd)) >= 0) {
                if (((Health != 0) && (Wear >= 95)) || (Temp > Mcot)) {
                    disk->cpqSasPhyDrvStatus = SAS_PHYS_STATUS_PREDICTIVEFAILURE;
                } else {
                    disk->cpqSasPhyDrvStatus = SAS_PHYS_STATUS_OK;
                }
            } else {
                disk->cpqSasPhyDrvStatus = SAS_PHYS_STATUS_FAILED;
            }

        }
        if ((disk->cpqSasPhyDrvType == 2) && 
            (disk->cpqSasPhyDrvStatus != SAS_PHYS_STATUS_FAILED)) {
            if (disk->cpqSasPhyDrvMediaType == PHYS_DRV_ROTATING_PLATTERS)
                disk->cpqSasPhyDrvUsedReallocs = get_defect_data_size(disk_fd, 5);
            if (disk->cpqSasPhyDrvMediaType == PHYS_DRV_SOLID_STATE)
                disk->cpqSasPhyDrvUsedReallocs = get_defect_data_size(disk_fd, 6);
            if (disk->cpqSasPhyDrvUsedReallocs == -1) 
                disk->cpqSasPhyDrvUsedReallocs = 0;
        } else
            disk->cpqSasPhyDrvUsedReallocs = 0;

        DEBUGMSGTL(("sasphydrv:container:load", 
                    "Current Temp = %dC,  Maximum Temp = %dC\n", Temp, Mcot));

        close(disk_fd);
    }

    disk->cpqSasPhyDrvMemberLogDrv = SAS_DISK_NOT_ARRAY_MEMBER;

    switch(hba->cpqSasHbaModel) {
        case SAS_HOST_MODEL_SPUTNIK:
        case SAS_HOST_MODEL_CALLISTA:
        case SAS_HOST_MODEL_GAGARIN:
        case SAS_HOST_MODEL_QUARTER:
        case SAS_HOST_MODEL_COLT:
        case SAS_HOST_MODEL_MORGAN:
        case SAS_HOST_MODEL_ARES:
        case SAS_HOST_MODEL_ROCKETS:
        case SAS_HOST_MODEL_BELGIAN:
        case SAS_HOST_MODEL_VOSTOK:
        case SAS_HOST_MODEL_ARABIAN:
            disk->cpqSasPhyDrvHotPlug = SAS_PHYS_DRV_HOT_PLUG;
            break;

        default:
            disk->cpqSasPhyDrvHotPlug = SAS_PHYS_DRV_NOT_HOT_PLUG;
    }
    switch (disk->cpqSasPhyDrvStatus) {
        case SAS_PHYS_STATUS_OK:
            disk->cpqSasPhyDrvCondition = SAS_PHYS_COND_OK;
            break;
        case SAS_PHYS_STATUS_NOTAUTHENTICATED:
        case SAS_PHYS_STATUS_PREDICTIVEFAILURE:
            disk->cpqSasPhyDrvCondition = SAS_PHYS_COND_DEGRADED;
            break;
        case SAS_PHYS_STATUS_OFFLINE:
        case SAS_PHYS_STATUS_FAILED:
        case SAS_PHYS_STATUS_SSDWEAROUT:
            disk->cpqSasPhyDrvCondition = SAS_PHYS_COND_FAILED;
            break;
        case SAS_PHYS_STATUS_OTHER:
        default:
            disk->cpqSasPhyDrvCondition = SAS_PHYS_COND_OTHER;
            break;
    }

    hba->cpqSasHbaOverallCondition =
        MAKE_CONDITION(hba->cpqSasHbaOverallCondition,
                               disk->cpqSasPhyDrvCondition);
    if (old == NULL) {
        CONTAINER_INSERT(container, disk);
        DEBUGMSGTL(("sasphydrv:container:load", "entry inserted\n"));
    }

    /* send a trap if drive is failed or degraded on the first time 
       through or it it changes from OK to degraded of failed */
    if ((old == NULL) || (disk->OldStatus == SAS_PHYS_STATUS_OK)) {
        switch (disk->cpqSasPhyDrvStatus) {
            case SAS_PHYS_STATUS_PREDICTIVEFAILURE:
            case SAS_PHYS_STATUS_OFFLINE:
            case SAS_PHYS_STATUS_FAILED:
                SendSasTrap(SAS_TRAP_PHYSDRV_STATUS_CHANGE, disk);
                break;
            default:
                break;
        }
    }

    free(scsi);
    return disk;
}

static void cpqsas_add_sas_end_device(char *devpath, char *devname, char *devtype, void *data) 
{
    cpqSasPhyDrvTable_entry *disk;

    int  Cntlr, Bus, Target, Lun;

    if (devname != NULL)
        if (sscanf(devname, "bsg/%d:%d:%d:%d", &Cntlr, &Bus, &Target, &Lun) != 4)
            return;
    if (devpath != NULL) {

        cpqSasHbaTable_entry *hba;
        netsnmp_container *hba_container;
        netsnmp_iterator  *it;
        netsnmp_cache *hba_cache;
        if (strstr(devpath, "end_device-") == NULL)
            return;
        hba_cache = netsnmp_cache_find_by_oid(cpqSasHbaTable_oid,
                cpqSasHbaTable_oid_len);
        if (hba_cache == NULL) {
            return ;
        }
        hba_container = hba_cache->magic;
        it = CONTAINER_ITERATOR(hba_container);
        hba = ITERATOR_FIRST(it);

        while ( hba != NULL ) {
            if (Cntlr == hba->cpqSasHbaIndex) {
                DEBUGMSGTL(("sasphydrv:container:load", 
                            "\nNew disk inserted %s\n", devpath));
                disk = sas_add_disk(devpath, 
                                    hba, 
                                    (netsnmp_container *)data);
                if (disk != NULL) {
                    disk->cpqSasPhyDrvStatus = SAS_PHYS_STATUS_OK;
                    disk->cpqSasPhyDrvCondition = SAS_PHYS_COND_OK;
                    SendSasTrap(SAS_TRAP_PHYSDRV_STATUS_CHANGE, disk);
                }
            }
            hba = ITERATOR_NEXT( it );
        }
        ITERATOR_RELEASE( it );
    }
}

static void cpqsas_remove_sas_end_device(char *devpath, char *devname, char *devtype, void *data)
{
    netsnmp_index tmp;
    oid oid_index[2];
    cpqSasPhyDrvTable_entry *disk;

    int  Cntlr = 0, Bus = 0, Target = 0, Lun = 0;

    if (devname != NULL)
        if (sscanf(devname, "bsg/%d:%d:%d:%d", &Cntlr, &Bus, &Target, &Lun) != 4)
            return;
    if (strstr(devpath, "end_device-") == NULL)
        return;

    oid_index[0] = Cntlr;
    oid_index[1] = Target;
    tmp.len = 2;
    tmp.oids = &oid_index[0];
    disk = CONTAINER_FIND((netsnmp_container *)data, &tmp);
   
    if (disk != NULL) {

        switch (disk->cpqSasPhyDrvStatus) {
        case 2:  /* ok(2) */
        case 3:  /* predictiveFailure(3) */
        case 4:  /* offline(4) */
        case 5:  /* failed(5) */
            disk->cpqSasPhyDrvStatus += 4;/* add missing */
            break;
        case 10:  /* ssdWearOut(10) */
            disk->cpqSasPhyDrvStatus = 
                            SAS_PHYS_STATUS_MISSINGWASSSDWEAROUT;
            break;
        case 12:  /* notAuthenticated(12) */
            disk->cpqSasPhyDrvStatus = 
                            SAS_PHYS_STATUS_MISSINGWASNOTAUTHENTICATED;
            break;
        default: 
            break;
        }
        disk->cpqSasPhyDrvCondition = SAS_PHYS_COND_FAILED;
        SendSasTrap(SAS_TRAP_PHYSDRV_STATUS_CHANGE, disk);

        CONTAINER_REMOVE((netsnmp_container *)data, disk);
    }
}

int netsnmp_arch_sashba_container_load(netsnmp_container* container)
{

    cpqSasHbaTable_entry *entry;
    cpqSasHbaTable_entry *old;

    sashba_config_buf * hbaConfig = NULL;
    sas_connector_info_buf * hbaConnector = NULL;
    unsigned int vendor, device, BoardID;
    int HbaIndex, Host;
    unsigned int pciSlot;
    char buffer[256], lbuffer[256], *pbuffer;
    char attribute[256];
    char *value;
    long  rc = 0;
    int i, len;
    netsnmp_index tmp;
    oid oid_index[2];

    DEBUGMSGTL(("sashba:container:load", "loading\n"));

    DEBUGMSGTL(("sashba:container:load", "Container=%p\n",container));

    HbaCondition = CPQ_REG_OK;

    /* Find all SAS HB's */
    if ((NumSasHost = 
                scandir(SasHostDir, &SasHostlist, file_select, alphasort)) <= 0)
        return -1;

    /* Now check for those HBA's in  the SCSI hosts */
    NumScsiHost = scandir(ScsiHostDir, &ScsiHostlist, hba_select, alphasort);
    /* We're done with SasHost so lets clean it up */
    for (i = 0; i < NumSasHost; i++) 
        free(SasHostlist[i]);
    free(SasHostlist);

    if (NumScsiHost <= 0)
        /* Should not happen */
        return -1;


    for (i=0; i < NumScsiHost; i++) {

        memset(buffer, 0, sizeof(buffer));
        strncpy(buffer, ScsiHostDir, sizeof(buffer) - 1);
        strncat(buffer, ScsiHostlist[i]->d_name,
                sizeof(buffer) - strlen(buffer) - 1);

        strncpy(attribute, buffer, sizeof(attribute) - 1);
        strncat(attribute, sysfs_attr[CLASS_PROC_NAME],
            sizeof(attribute) - strlen(attribute) - 1);
        if ((value = get_sysfs_str(attribute)) != NULL) {
            if (strncmp(value, "hp", 2) == 0) {
                free(value);
                free(ScsiHostlist[i]);
                continue;
            }
            free(value);
        }

        strncpy(attribute, buffer, sizeof(attribute) - 1);
        strncat(attribute, sysfs_attr[DEVICE_SUBSYSTEM_DEVICE], 
                sizeof(attribute) - strlen(attribute) - 1);
        device = get_sysfs_shex(attribute);

        strncpy(attribute, buffer, sizeof(attribute) - 1);
        strncat(attribute, sysfs_attr[DEVICE_SUBSYSTEM_VENDOR], 
                sizeof(attribute) - strlen(attribute) - 1);
        vendor = get_sysfs_shex(attribute);

        BoardID = device << 16;
        BoardID += vendor;

        /*  We will need the host name later on */
        sscanf(ScsiHostlist[i]->d_name, "host%d", &Host); 

        HbaIndex = Host;

        oid_index[0] = HbaIndex;
        tmp.len = 1;
        tmp.oids = &oid_index[0];

        old = CONTAINER_FIND(container, &tmp);
        DEBUGMSGTL(("sashba:container:load", "HBA = %p\n",old));
        if (old != NULL ) {
            DEBUGMSGTL(("sashba:container:load", "Re-Use old entry\n"));
            old->OldStatus = old->cpqSasHbaStatus;
            entry = old;
        } else {
            entry = cpqSasHbaTable_createEntry(container, (oid)HbaIndex);
            if (NULL == entry)
                continue;   /* error already logged by func */

            DEBUGMSGTL(("sashba:container:load", "Entry created\n"));

            entry->cpqSasHbaStatus = CPQ_REG_OTHER;

            sprintf(entry->host,"%d:",Host);
            entry->cpqSasHbaIndex = HbaIndex;

            strcpy(attribute, buffer);
            pciSlot = pcislot_scsi_host(attribute);

            entry->cpqSasHbaHwLocation[0] ='\0';
            entry->cpqSasHbaHwLocation_len = 0;
            if (pciSlot > 0 ) {
                DEBUGMSGTL(("sashba:container:load", "Got pcislot info for %s = %d\n",
                            buffer,
                            pciSlot));

                if (pciSlot > 0x1000)
                    entry->cpqSasHbaHwLocation_len =
                        snprintf(entry->cpqSasHbaHwLocation,
                                128,
                                "Blade %d, Slot %d",
                                (pciSlot>>8) & 0xF,
                                (pciSlot>>16) & 0xFF);
                else {
                    entry->cpqSasHbaHwLocation_len =
                        snprintf(entry->cpqSasHbaHwLocation,
                                128, "Slot %d", pciSlot);
                    DEBUGMSGTL(("sashba:container:load", "pcislot  = %s\n",
                                    entry->cpqSasHbaHwLocation));
               }

            } else {
                strcpy(entry->cpqSasHbaHwLocation, "Embedded");
                entry->cpqSasHbaHwLocation_len =
                    strlen(entry->cpqSasHbaHwLocation);
            }

            memset(attribute, 0, sizeof(attribute));
            strncpy(attribute, buffer, sizeof(attribute) - 1);
            strncat(attribute, "/device",
                sizeof(attribute) - strlen(attribute) - 8);
            if ((len = readlink(buffer, lbuffer, 253)) > 0) {
                lbuffer[len]='\0'; /* Null terminate the string */
                pbuffer = lbuffer;
                while ((pbuffer = strstr(pbuffer, "/0000:")) != NULL) {
                    entry->cpqSasHbaPciLocation_len = 12;

                        strncpy(entry->cpqSasHbaPciLocation, pbuffer + 1,12);

                    DEBUGMSGTL(("sashba:container:load", "pbuffer  = %s\n", pbuffer));
                    DEBUGMSGTL(("sashba:container:load", "PciLocation  = %s\n", entry->cpqSasHbaPciLocation));

                    pbuffer += 1;
                }
            }

            switch (BoardID) {
                case PCI_SSID_SPUTNIK_8INT:
                case PCI_SSID_BLADE1:
                case PCI_SSID_BLADE2:
                case PCI_SSID_BLADE3:
                    entry->cpqSasHbaModel = SAS_HOST_MODEL_SPUTNIK;
                    break;
                case PCI_SSID_SPUTNIK_4INT:
                    entry->cpqSasHbaModel = SAS_HOST_MODEL_CALLISTA;
                    break;
                case PCI_SSID_VOSTOK:
                    entry->cpqSasHbaModel = SAS_HOST_MODEL_VOSTOK;
                    break;
                case PCI_SSID_GAGARIN:
                    entry->cpqSasHbaModel = SAS_HOST_MODEL_GAGARIN;
                    break;
                case PCI_SSID_ARES:
                    entry->cpqSasHbaModel = SAS_HOST_MODEL_ARES;
                    break;
                case PCI_SSID_ROCKETS:
                    entry->cpqSasHbaModel = SAS_HOST_MODEL_ROCKETS;
                    break;
                case PCI_SSID_QUARTER:
                    entry->cpqSasHbaModel = SAS_HOST_MODEL_QUARTER;
                    break;
                case PCI_SSID_BELGIAN:
                    entry->cpqSasHbaModel = SAS_HOST_MODEL_BELGIAN;
                    break;
                case PCI_SSID_COLT:
                    entry->cpqSasHbaModel = SAS_HOST_MODEL_COLT;
                    break;
                case PCI_SSID_MORGAN:
                    entry->cpqSasHbaModel = SAS_HOST_MODEL_MORGAN;
                    break;
                case PCI_SSID_ARABIAN:
                    entry->cpqSasHbaModel = SAS_HOST_MODEL_ARABIAN;
                    break;

                default:
                    entry->cpqSasHbaModel = SAS_HOST_MODEL_SAS_HBA;
                    break;
            }

            /* Get the firmware version and register it with the AgentX master */
            strncpy(attribute, buffer, sizeof(attribute) - 1);
            strncat(attribute, sysfs_attr[CLASS_VERSION_FW],
                    sizeof(attribute) - strlen(attribute) - 1);
            if ((value = get_sysfs_str(attribute)) != NULL) {

                strncpy(entry->cpqSasHbaFwVersion, value, 
                        sizeof(entry->cpqSasHbaFwVersion) - 1);    
                entry->cpqSasHbaFwVersion_len = 
                                        strlen(entry->cpqSasHbaFwVersion);

                free(value);
            }

            strncpy(attribute, buffer, sizeof(attribute) - 1);
            strncat(attribute, sysfs_attr[CLASS_VERSION_BIOS],
                    sizeof(attribute) - strlen(attribute) - 1);
            if ((value = get_sysfs_str(attribute)) != NULL) {
                strncpy(entry->cpqSasHbaBiosVersion, value,
                        sizeof(entry->cpqSasHbaBiosVersion) - 1);    
                entry->cpqSasHbaBiosVersion_len = 
                    strlen(entry->cpqSasHbaBiosVersion);

                free(value);
            }

            strncpy(attribute, buffer, sizeof(attribute) - 1);
            strncat(attribute, sysfs_attr[CLASS_BOARD_TRACER],
                    sizeof(attribute) - strlen(attribute) - 1);
            if ((value = get_sysfs_str(attribute)) != NULL) {
                strncpy(entry->cpqSasHbaSerialNumber, value,
                        sizeof(entry->cpqSasHbaSerialNumber) - 1);    
                entry->cpqSasHbaSerialNumber_len = 
                    strlen(entry->cpqSasHbaSerialNumber);
                free(value);
            }

            strncpy(attribute, buffer, sizeof(attribute) - 1);
            strncat(attribute, sysfs_attr[CLASS_STATE],
                    sizeof(attribute) - strlen(attribute) - 1);
            if ((value = get_sysfs_str(attribute)) != NULL) {
                if (strcmp(value, "running") == 0)
                    entry->cpqSasHbaStatus = SAS_HOST_STATUS_OK;
                free(value);
            } else
                entry->cpqSasHbaStatus = SAS_HOST_STATUS_FAILED;

            memset(buffer, 0, sizeof(buffer));
            strncpy(buffer, SasHostDir, sizeof(buffer) - 1);
            strncat(buffer, ScsiHostlist[i]->d_name,
                    sizeof(buffer) - strlen(buffer) - 1);
            strcat(buffer, "/device/../driver");
            if ((len = readlink(buffer, lbuffer, 253)) > 0) {
                lbuffer[len]='\0'; /* Null terminate the string */
                pbuffer = basename(lbuffer);

                memset(&buffer, 0, sizeof(buffer));
                strcpy(buffer, "/dev/");
                if (!strcmp(pbuffer, "mpt2sas") )
                    strcat(buffer, "mpt2ctl");
                else
                    strcat(buffer, "mptctl");
                if ((hbaConfig = SasGetHbaConfig(HbaIndex, buffer)) != NULL ) {
                    strncpy(entry->cpqSasHbaSerialNumber,
                            hbaConfig->Configuration.szSerialNumber,
                            sizeof(entry->cpqSasHbaSerialNumber) - 1);
                    entry->cpqSasHbaSerialNumber_len =
                        strlen(entry->cpqSasHbaSerialNumber);
                    free(hbaConfig);
                }
            }
            memset(entry->Reference, 0, sizeof(sas_connector_info) * 32);
            DEBUGMSGTL(("sashba:container:load", 
                        "getting Connector info %d, %s\n", HbaIndex, buffer));
            if ((hbaConnector = SasGetHbaConnector(HbaIndex, buffer)) != NULL ){
                int iii;
                memcpy(entry->Reference, hbaConnector->Reference, 
                       sizeof(sas_connector_info) * 32);
                free(hbaConnector);
                for (iii = 0; iii < 32; iii++) {
                    int conlen = strlen((char *)entry->Reference[iii].bConnector) - 1;
                    if (entry->Reference[iii].bConnector[conlen] == 0x20 ) 
                        entry->Reference[iii].bConnector[conlen] = 0;
                    DEBUGMSGTL(("sashba:container:load", "Connector= %s\n",
                                entry->Reference[iii].bConnector));
                }
            }

            rc = CONTAINER_INSERT(container, entry);
            DEBUGMSGTL(("sashba:container:load", "container inserted\n"));

            switch (entry->cpqSasHbaStatus) {
                case SAS_HOST_STATUS_OK:
                    entry->cpqSasHbaCondition = SAS_HOST_COND_OK;
                    break;
                case SAS_HOST_STATUS_FAILED:
                    entry->cpqSasHbaCondition = SAS_HOST_COND_FAILED;
                    break;
                case SAS_HOST_STATUS_OTHER:
                default:
                    entry->cpqSasHbaCondition = SAS_HOST_COND_OTHER;
            }

        }
        entry->cpqSasHbaOverallCondition = entry->cpqSasHbaCondition;

        if (entry->cpqSasHbaOverallCondition > HbaCondition) 
            HbaCondition = entry->cpqSasHbaCondition;

        free(ScsiHostlist[i]);
    }
    free(ScsiHostlist);

    cpqHoMibHealthStatusArray[CPQMIBHEALTHINDEX] = HbaCondition;

    return(rc);
}

int netsnmp_arch_sasphydrv_container_load(netsnmp_container* container)
{
    cpqSasHbaTable_entry *hba;
    cpqSasPhyDrvTable_entry *disk;
    netsnmp_container *hba_container;
    netsnmp_iterator  *it;
    netsnmp_cache *hba_cache;
    int len;
    int j;
    int SasCondition = CPQ_REG_OK;

    DEBUGMSGTL(("sasphydrv:container:load", "loading\n"));
    /*
     * find  the HBa container.
     */

    hba_cache = netsnmp_cache_find_by_oid(cpqSasHbaTable_oid, 
            cpqSasHbaTable_oid_len);
    if (hba_cache == NULL) {
        return 0;
    }
    hba_container = hba_cache->magic;
    DEBUGMSGTL(("sasphydrv:container:load", "HbaContainer=%p\n",hba_container));
    DEBUGMSGTL(("sasphydrv:container:load", 
                "HbaContainerSize=%ld\n",CONTAINER_SIZE(hba_container)));
    it = CONTAINER_ITERATOR( hba_container );
    hba = ITERATOR_FIRST( it );
    DEBUGMSGTL(("sasphydrv:container:load", "hba=%p\n",hba));
    while ( hba != NULL ) {
        hba->cpqSasHbaOverallCondition = hba->cpqSasHbaCondition;
        current_Hba = hba->host;
        DEBUGMSGTL(("sasphydrv:container:load", 
                    "Starting Loop %s\n", current_Hba));
        DEBUGMSGTL(("sasphydrv:container:load", 
                    "Scanning Dir %s\n", SasDeviceDir));
        /* Now chack for those HBA's in  the SCSI diskss */
        if ((NumSasDevice = scandir(SasDeviceDir, &SasDevicelist, 
                        end_device_select, alphasort)) <= 0) {
            ITERATOR_RELEASE( it );
            return -1;
	    }

        for (j= 0; j< NumSasDevice; j++) {
            char buffer[256], lbuffer[256];
            int Cntlr;
            memset(&buffer, 0, sizeof(buffer));
            strncpy(buffer, SasDeviceDir, sizeof(buffer) - 1);
            strncat(buffer, SasDevicelist[j]->d_name,
                    sizeof(buffer) - strlen(buffer) - 1);
#if RHEL_MAJOR == 5
            strncat(buffer, "/device", sizeof(buffer) - strlen(buffer) - 1);
#endif
            free(SasDevicelist[j]);
            if ((len = readlink(buffer, lbuffer, 253)) <= 0) 
                continue;

            lbuffer[len]='\0'; /* Null terminate the string */
            /* if it's not the controller we are working on, skip it */
            DEBUGMSGTL(("sasphydrv:container:load",
                "Index %ld\n", hba->cpqSasHbaIndex));
            if ((Cntlr = get_Cntlr(lbuffer)) != hba->cpqSasHbaIndex)
                continue;
            disk = sas_add_disk(lbuffer, hba, container);
            if (disk != NULL) {
                 hba->cpqSasHbaOverallCondition =
                    MAKE_CONDITION(hba->cpqSasHbaOverallCondition,
                               disk->cpqSasPhyDrvCondition);
                if (trap_fire)
                    SendSasTrap(SAS_TRAP_PHYSDRV_STATUS_CHANGE, disk);
            }

        }
        SasCondition = MAKE_CONDITION(SasCondition,
                                      hba->cpqSasHbaOverallCondition); 
        free(SasDevicelist);
        hba = ITERATOR_NEXT( it );
    }
    ITERATOR_RELEASE( it );

    cpqHoMibHealthStatusArray[CPQMIBHEALTHINDEX] = SasCondition;

    DEBUGMSGTL(("sasphydrv:container", "init\n"));
    udev_register("bsg","add", NULL, cpqsas_add_sas_end_device, container);
    udev_register("bsg","remove", NULL, cpqsas_remove_sas_end_device, container);
    return  1;
}

void SendSasTrap(int trapID,     
        cpqSasPhyDrvTable_entry *disk)
{
    static oid compaq[] = { 1, 3, 6, 1, 4, 1, 232 };
    static oid compaq_len = 7;
    static oid sysName[] = { 1, 3, 6, 1, 2, 1, 1, 5, 0 };
    static oid cpqHoTrapFlags[] = { 1, 3, 6, 1, 4, 1, 232, 11, 2, 11, 1, 0 };
    static oid cpqSasHbaHwLocation[] = 
    { 1, 3, 6, 1, 4, 1, 232, 5, 5, 1, 1, 1, 2, 255 };
    static oid cpqSasPhyDrvHbaIndex[] = 
    { 1, 3, 6, 1, 4, 1, 232, 5, 5, 2, 1, 1, 1, 255, 255 };
    static oid cpqSasPhyDrvIndex[] =  
    { 1, 3, 6, 1, 4, 1, 232, 5, 5, 2, 1, 1, 2, 255, 255 };
    static oid cpqSasPhyDrvLocationString[] = 
    { 1, 3, 6, 1, 4, 1, 232, 5, 5, 2, 1, 1, 3, 255, 255 };
    static oid cpqSasPhyDrvModel[] =
    { 1, 3, 6, 1, 4, 1, 232, 5, 5, 2, 1, 1, 4, 255, 255 };
    static oid cpqSasPhyDrvStatus[] = 
    { 1, 3, 6, 1, 4, 1, 232, 5, 5, 2, 1, 1, 5, 255, 255 };
    static oid cpqSasPhyDrvFWRev[] =
    { 1, 3, 6, 1, 4, 1, 232, 5, 5, 2, 1, 1, 7, 255, 255 };
    static oid cpqSasPhyDrvSerialNumber[] =
    { 1, 3, 6, 1, 4, 1, 232, 5, 5, 2, 1, 1, 10, 255, 255 };
    static oid cpqSasPhyDrvType[] =
    { 1, 3, 6, 1, 4, 1, 232, 5, 5, 2, 1, 1, 16,255, 255 };
    static oid cpqSasPhyDrvSasAddress[] = 
    { 1, 3, 6, 1, 4, 1, 232, 5, 5, 2, 1, 1, 17, 255, 255 };

    netsnmp_variable_list *var_list = NULL;

    unsigned int cpqHoTrapFlag;
    DEBUGMSGTL(("sasphydrv:container:load", "Trap:DiskCondition = %ld\n",
                disk->cpqSasPhyDrvCondition));
    cpqHoTrapFlag = disk->cpqSasPhyDrvCondition << 2;
    if (trap_fire) 
        cpqHoTrapFlag = trap_fire << 2;

    snmp_varlist_add_variable(&var_list, sysName,
            sizeof(sysName) / sizeof(oid),
            ASN_OCTET_STR,
            (u_char *) "",
            0);

    snmp_varlist_add_variable(&var_list, cpqHoTrapFlags,
            sizeof(cpqHoTrapFlags) / sizeof(oid),
            ASN_INTEGER, (u_char *)&cpqHoTrapFlag,
            sizeof(cpqHoTrapFlag));

    cpqSasHbaHwLocation[OID_LENGTH(cpqSasHbaHwLocation) - 1] = 
            disk->cpqSasPhyDrvHbaIndex;
    snmp_varlist_add_variable(&var_list, cpqSasHbaHwLocation,
            sizeof(cpqSasHbaHwLocation) / sizeof(oid),
            ASN_OCTET_STR,
            (u_char *) disk->hba->cpqSasHbaHwLocation,
             disk->hba->cpqSasHbaHwLocation_len);

    cpqSasPhyDrvLocationString[OID_LENGTH(cpqSasPhyDrvLocationString) - 2] =
            disk->cpqSasPhyDrvHbaIndex;
    cpqSasPhyDrvLocationString[OID_LENGTH(cpqSasPhyDrvLocationString) - 1] =
            disk->cpqSasPhyDrvIndex;
    snmp_varlist_add_variable(&var_list, cpqSasPhyDrvLocationString,
            sizeof(cpqSasPhyDrvLocationString) / sizeof(oid),
            ASN_OCTET_STR,
            (u_char *) disk->cpqSasPhyDrvLocationString,
            disk->cpqSasPhyDrvLocationString_len);

    cpqSasPhyDrvHbaIndex[OID_LENGTH(cpqSasPhyDrvHbaIndex) - 2] =
            disk->cpqSasPhyDrvHbaIndex;
    cpqSasPhyDrvHbaIndex[OID_LENGTH(cpqSasPhyDrvHbaIndex) - 1] =
            disk->cpqSasPhyDrvIndex;
    snmp_varlist_add_variable(&var_list, cpqSasPhyDrvHbaIndex,
            sizeof(cpqSasPhyDrvHbaIndex) / sizeof(oid),
            ASN_INTEGER, (u_char *) &disk->cpqSasPhyDrvHbaIndex,
            sizeof(disk->cpqSasPhyDrvHbaIndex));

    cpqSasPhyDrvIndex[OID_LENGTH(cpqSasPhyDrvIndex) - 2] =
            disk->cpqSasPhyDrvHbaIndex;
    cpqSasPhyDrvIndex[OID_LENGTH(cpqSasPhyDrvIndex) - 1] =
            disk->cpqSasPhyDrvIndex;
    snmp_varlist_add_variable(&var_list, cpqSasPhyDrvIndex,
            sizeof(cpqSasPhyDrvIndex) / sizeof(oid),
            ASN_INTEGER, (u_char *)&disk->cpqSasPhyDrvIndex,
            sizeof(disk->cpqSasPhyDrvIndex));

    cpqSasPhyDrvStatus[OID_LENGTH(cpqSasPhyDrvStatus) - 2] =
            disk->cpqSasPhyDrvHbaIndex;
    cpqSasPhyDrvStatus[OID_LENGTH(cpqSasPhyDrvStatus) - 1] =
            disk->cpqSasPhyDrvIndex;
    snmp_varlist_add_variable(&var_list, cpqSasPhyDrvStatus,
            sizeof(cpqSasPhyDrvStatus) / sizeof(oid),
            ASN_INTEGER,(u_char *)&disk->cpqSasPhyDrvStatus,
            sizeof(disk->cpqSasPhyDrvStatus));

    cpqSasPhyDrvType[OID_LENGTH(cpqSasPhyDrvType) - 2] =
            disk->cpqSasPhyDrvHbaIndex;
    cpqSasPhyDrvType[OID_LENGTH(cpqSasPhyDrvType) - 1] =
            disk->cpqSasPhyDrvIndex;
    snmp_varlist_add_variable(&var_list, cpqSasPhyDrvType,
            sizeof(cpqSasPhyDrvType) / sizeof(oid),
            ASN_INTEGER,(u_char *)&disk->cpqSasPhyDrvType,
            sizeof(disk->cpqSasPhyDrvType));

    cpqSasPhyDrvModel[OID_LENGTH(cpqSasPhyDrvModel) - 2] =
            disk->cpqSasPhyDrvHbaIndex;
    cpqSasPhyDrvModel[OID_LENGTH(cpqSasPhyDrvModel) - 1] =
            disk->cpqSasPhyDrvIndex;
    snmp_varlist_add_variable(&var_list, cpqSasPhyDrvModel,
            sizeof(cpqSasPhyDrvModel)/ sizeof(oid),
            ASN_OCTET_STR,
            (u_char *) disk->cpqSasPhyDrvModel,
            disk->cpqSasPhyDrvModel_len);

    cpqSasPhyDrvFWRev[OID_LENGTH(cpqSasPhyDrvFWRev) - 2] =
            disk->cpqSasPhyDrvHbaIndex;
    cpqSasPhyDrvFWRev[OID_LENGTH(cpqSasPhyDrvFWRev) - 1] =
            disk->cpqSasPhyDrvIndex;
    snmp_varlist_add_variable(&var_list, cpqSasPhyDrvFWRev,
            sizeof(cpqSasPhyDrvFWRev) / sizeof(oid),
            ASN_OCTET_STR,
            (u_char *) disk->cpqSasPhyDrvFWRev,
            disk->cpqSasPhyDrvFWRev_len);

    cpqSasPhyDrvSerialNumber[OID_LENGTH(cpqSasPhyDrvSerialNumber) - 2] =
            disk->cpqSasPhyDrvHbaIndex;
    cpqSasPhyDrvSerialNumber[OID_LENGTH(cpqSasPhyDrvSerialNumber) - 1] =
            disk->cpqSasPhyDrvIndex;
    snmp_varlist_add_variable(&var_list, cpqSasPhyDrvSerialNumber,
            sizeof(cpqSasPhyDrvSerialNumber) / sizeof(oid),
            ASN_OCTET_STR,
            (u_char *) disk->cpqSasPhyDrvSerialNumber,
            disk->cpqSasPhyDrvSerialNumber_len);

    cpqSasPhyDrvSasAddress[OID_LENGTH(cpqSasPhyDrvSasAddress) - 2] =
            disk->cpqSasPhyDrvHbaIndex;
    cpqSasPhyDrvSasAddress[OID_LENGTH(cpqSasPhyDrvSasAddress) - 1] =
            disk->cpqSasPhyDrvIndex;
    snmp_varlist_add_variable(&var_list, cpqSasPhyDrvSasAddress,
            sizeof(cpqSasPhyDrvSasAddress) / sizeof(oid),
            ASN_OCTET_STR,
            (u_char *) disk->cpqSasPhyDrvSasAddress,
            disk->cpqSasPhyDrvSasAddress_len);


            send_enterprise_trap_vars(SNMP_TRAP_ENTERPRISESPECIFIC,
                    trapID,
                    compaq,
                    compaq_len,
                    var_list);

    snmp_free_varbind(var_list);

            return;
}


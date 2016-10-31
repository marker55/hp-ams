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
#include <sys/ioctl.h>
#include <dirent.h>
#include <unistd.h>
#include <getopt.h>

#include "cpqIde.h"
#include "cpqIdeControllerTable.h"
#include "cpqIdeAtaDiskTable.h"

#include "idecntlr.h"
#include "idedisk.h"
#include "common/scsi_info.h"
#include "common/smbios.h"
#include "common/utils.h"
#include "common/nl_udev.h"

#include "ide_linux.h"

extern unsigned char     cpqHoMibHealthStatusArray[];
extern int trap_fire;

extern int file_select(const struct dirent *);
void SendIdeTrap(int, cpqIdeAtaDiskTable_entry *);
extern int pcislot_scsi_host(char * buffer);

void netsnmp_arch_idecntlr_init(void); 
void netsnmp_arch_idedisk_init(void);

extern  int alphasort();

static struct dirent **ScsiHostlist;
static char * ScsiHostDir = "/sys/class/scsi_host/";

static struct dirent **ScsiDisklist;
static char * ScsiDiskDir = "/sys/class/scsi_disk/";

static int ide_disk_select(const struct dirent *entry)
{
    char buffer[256], lbuffer[256], *pbuffer;
    int len;

    memset(buffer, 0, 256);
    strcpy(buffer, ScsiDiskDir);
    strcat(buffer, entry->d_name);

    if ((len = readlink(buffer, lbuffer, 253)) > 0) {
        lbuffer[len]='\0'; /* Null terminate the string */
        if ((pbuffer = strstr(lbuffer, "/ata")) != NULL) {
            return(1);
        }
    }
    return 0;
}

static int ide_select(const struct dirent *entry)
{
    char buffer[256], lbuffer[256], *pbuffer;
    int len;

    memset(buffer, 0, 256);
    strcpy(buffer, ScsiHostDir);
    strcat(buffer, entry->d_name);

    if ((len = readlink(buffer, lbuffer, 253)) > 0) {
        lbuffer[len]='\0'; /* Null terminate the string */
        if ((pbuffer = strstr(lbuffer, "/ata")) != NULL) {
            return(1);
        }
    }
    return 0;
}

cpqIdeAtaDiskTable_entry *ide_add_disk(char *deviceLink,
		 char *scsi,
                 cpqIdeControllerTable_entry *cntlr,
		 int disk_index,
                 netsnmp_container* container)
{
    cpqIdeAtaDiskTable_entry *disk;
    cpqIdeAtaDiskTable_entry *old;

    char *value;
    char *generic;

    char *serialnum = NULL;
    int disk_fd;
    int Health = 0, Temp = -1, Mcot = -1, Wear = -1;
    char *Temperature;

    netsnmp_index tmp;
    oid oid_index[2];

    /*
     * find  the HBa container.
     */
    SataDiskCondition = CPQ_REG_OK;

            DEBUGMSGTL(("idedisk:container:load",
                        "looking for cntlr=%ld, Disk = %d\n", 
                        cntlr->cpqIdeControllerIndex, disk_index));
            oid_index[0] = cntlr->cpqIdeControllerIndex;
            oid_index[1] = disk_index;
            tmp.len = 2;
            tmp.oids = &oid_index[0];

            old = CONTAINER_FIND(container, &tmp);

            DEBUGMSGTL(("idedisk:container:load", "Old disk=%p\n", old));
            if (old != NULL ) {
                DEBUGMSGTL(("idedisk:container:load", "Re-Use old entry\n"));
                old->OldStatus = old->cpqIdeAtaDiskStatus;
                old->OldSSDWearStatus = old->cpqIdeAtaDiskSSDWearStatus;
                disk = old;
            } else {
                disk = cpqIdeAtaDiskTable_createEntry(container,
                            (oid)cntlr->cpqIdeControllerIndex, 
                        (oid)disk_index);
                if (disk == NULL) 
            return 0;

                DEBUGMSGTL(("idedisk:container:load", "Entry created\n"));
        DEBUGMSGTL(("idedisk:container:load", "scsi = %s\n", scsi));
                if ((value = get_DiskModel(scsi)) != NULL) {
                    strncpy(disk->cpqIdeAtaDiskModel, value,
                            sizeof(disk->cpqIdeAtaDiskModel) - 1);
                    disk->cpqIdeAtaDiskModel_len = 
                                               strlen(disk->cpqIdeAtaDiskModel);
                    free(value);
                }

                if ((value = get_DiskState(scsi)) != NULL) {
                    if (strcmp(value, "running") == 0)
                disk->cpqIdeAtaDiskStatus = IDE_DISK_STATUS_OK;
                    free(value);
                }

                disk->cpqIdeAtaDiskCapacity = 
                                get_BlockSize((unsigned char *)scsi) >> 11;

                generic = (char *)get_ScsiGeneric((unsigned char *)scsi);
                if (generic != NULL) {
                    memset(disk->cpqIdeAtaDiskOsName, 0, 256);
                    disk->cpqIdeAtaDiskOsName_len =
                                        sprintf(disk->cpqIdeAtaDiskOsName,
                                                "/dev/%s",
                                                generic);
                    free(generic);
                }
                DEBUGMSGTL(("idedisk:container:load", "generic dev is %s\n",
                                disk->cpqIdeAtaDiskOsName));

            }
    if ((disk_fd = open(disk->cpqIdeAtaDiskOsName, O_RDWR | O_NONBLOCK)) >= 0) {
                if ((serialnum = get_unit_sn(disk_fd)) != NULL  ) {
                    strncpy(disk->cpqIdeAtaDiskSerialNumber, serialnum, 40);
                    disk->cpqIdeAtaDiskSerialNumber_len = strlen(serialnum);
                    free(serialnum);
                }

                if ((value = get_sata_DiskRev(disk_fd)) != NULL) {
                    strncpy(disk->cpqIdeAtaDiskFwRev, value,
                            sizeof(disk->cpqIdeAtaDiskFwRev) - 1);
                    disk->cpqIdeAtaDiskFwRev_len = 
                                               strlen(disk->cpqIdeAtaDiskFwRev);
                    free(value);
                }

                disk->cpqIdeAtaDiskPowerOnHours = get_sata_pwron(disk_fd);

                disk->cpqIdeAtaDiskSSDEstTimeRemainingHours = -1;

                if ((Temperature = get_sata_temp(disk_fd)) != NULL ) {
                    Temp = sata_parse_current(Temperature);
                    Mcot = sata_parse_mcot(Temperature);
                    disk->cpqIdeAtaDiskCurrTemperature = Temp;
                    disk->cpqIdeAtaDiskTemperatureThreshold = Mcot;
                    free(Temperature);
                }

                if (is_unit_ssd(disk_fd)) {
                    disk->cpqIdeAtaDiskMediaType = PHYS_DRV_SOLID_STATE;
                    Wear = get_sata_ssd_wear(disk_fd);
                    disk->cpqIdeAtaDiskSSDPercntEndrnceUsed = Wear;
                    disk->cpqIdeAtaDiskSSDWearStatus = SSD_WEAR_OK;
                    if (disk->cpqIdeAtaDiskSSDPercntEndrnceUsed >= 95)
                        disk->cpqIdeAtaDiskSSDWearStatus = 
                                                        SSD_WEAR_5PERCENT_LEFT;
                    if (disk->cpqIdeAtaDiskSSDPercntEndrnceUsed >= 98)
                        disk->cpqIdeAtaDiskSSDWearStatus = 
                                                        SSD_WEAR_2PERCENT_LEFT;
                    if (disk->cpqIdeAtaDiskSSDPercntEndrnceUsed >= 100)
                        disk->cpqIdeAtaDiskSSDWearStatus = SSD_WEAR_OUT;
                } else {
                    disk->cpqIdeAtaDiskMediaType = PHYS_DRV_ROTATING_PLATTERS;
                    disk->cpqIdeAtaDiskSSDPercntEndrnceUsed = 
                                                    get_sata_ssd_wear(disk_fd);
                    disk->cpqIdeAtaDiskSSDWearStatus = SSD_WEAR_OTHER;
                } 

                if ((Health = get_sata_health(disk_fd)) >= 0) {
                    disk->cpqIdeAtaDiskSmartEnabled = 2;
                    if (Health == 0) {
                        disk->cpqIdeAtaDiskStatus = 2; /* OK */
                        disk->cpqIdeAtaDiskCondition = 2; /* OK */
                    } else {
                        disk->cpqIdeAtaDiskStatus = 3; /* predictive failure */
                        disk->cpqIdeAtaDiskCondition = 3; /*  Degraded*/
                    }
                } else {
                    disk->cpqIdeAtaDiskSmartEnabled = 1;
                    disk->cpqIdeAtaDiskStatus = 1; /* Other */
                    disk->cpqIdeAtaDiskCondition = 1; /* Other */
                }
                close(disk_fd);
            }
            disk->cpqIdeAtaDiskChannel = 4; /* Serial */
            disk->cpqIdeAtaDiskTransferMode = 1;
            disk->cpqIdeAtaDiskLogicalDriveMember = 1;
            disk->cpqIdeAtaDiskIsSpare = 1;
            disk->cpqIdeAtaDiskType = 3;  /* Only SATA drives */
            disk->cpqIdeAtaDiskSataVersion = 3; /*assume version 2 */
    disk->cpqIdeAtaDiskNumber = 20 + disk_index;

            disk->cpqIdeAtaDiskCondition = 
                            MAKE_CONDITION(disk->cpqIdeAtaDiskCondition, 
                                           disk->cpqIdeAtaDiskStatus);
            cntlr->cpqIdeControllerOverallCondition =
                MAKE_CONDITION(cntlr->cpqIdeControllerOverallCondition,
                               disk->cpqIdeAtaDiskStatus);
            if (old == NULL) {
                CONTAINER_INSERT(container, disk);
                DEBUGMSGTL(("idedisk:container:load", "entry inserted\n"));
            }
            if ((old != NULL) && 
                (disk->cpqIdeAtaDiskSSDWearStatus != old->OldSSDWearStatus)){
                SendIdeTrap(IDE_TRAP_DISK_SSD_WEAR_STATUS_CHANGE, disk);
                DEBUGMSGTL(("idedisk:container:load", "Sending Trap\n"));
            }
            if ((old != NULL) && (old->OldStatus != disk->cpqIdeAtaDiskStatus)){
                SendIdeTrap(IDE_TRAP_DISK_STATUS_CHANGE, disk);
                DEBUGMSGTL(("idedisk:container:load", "Sending Trap\n"));
            }
            SataDiskCondition |=  (1 < (disk->cpqIdeAtaDiskCondition -1));
    
    if ((SataDiskCondition & (1 < (CPQ_REG_DEGRADED -1))) != 0)
        cpqHoMibHealthStatusArray[CPQMIBHEALTHINDEX] = CPQ_REG_DEGRADED;

    return disk;
}

static void cpqide_add_ide_end_device(char *devpath, 
				      char *devname, 
				      char *devtype, 
				      void *data)
{
    cpqIdeAtaDiskTable_entry *disk;

    int  Cntlr, Bus, Target;
    int  disk_index;
    char scsi[128];

    DEBUGMSGTL(("idedisk:container:load", "Inserted disk\n"));
    if (devname != NULL) {
        if (sscanf(devname, "bsg/%d:%d:%d:0", &Cntlr, &Bus, &Target) != 3)
            return;
        if (sscanf(devname, "bsg/%s", scsi) != 1)
            return;
    }
    disk_index = Cntlr + (2 * Target) + 1;

    if (devpath != NULL) {
        cpqIdeControllerTable_entry *ctlr;
        netsnmp_container *ctlr_container;
        netsnmp_iterator  *it;
        netsnmp_cache *ctlr_cache;

        ctlr_cache = netsnmp_cache_find_by_oid(cpqIdeControllerTable_oid,
                cpqIdeControllerTable_oid_len);
        if (ctlr_cache == NULL) 
            return ;
        
        ctlr_container = ctlr_cache->magic;
        it = CONTAINER_ITERATOR(ctlr_container);
        ctlr = ITERATOR_FIRST(it);

        while ( ctlr != NULL ) {
            if (Cntlr == ctlr->cpqIdeControllerIndex) {
                DEBUGMSGTL(("idedisk:container:load",
                            "\nNew disk inserted %s\n", devpath));
                disk = ide_add_disk(devpath,
				                    scsi,
                                    ctlr,
				                    disk_index,
                                    (netsnmp_container *)data);
                if (disk != NULL) {
                    disk->cpqIdeAtaDiskStatus = IDE_DISK_STATUS_OK;
                    disk->cpqIdeAtaDiskCondition = IDE_DISK_COND_OK;
                    SendIdeTrap(IDE_TRAP_DISK_STATUS_CHANGE, disk);
                }
            }
            ctlr = ITERATOR_NEXT( it );
        }
        ITERATOR_RELEASE( it );
    }
}

static void cpqide_remove_ide_end_device(char *devpath, 
					 char *devname, 
					 char *devtype, 
					 void *data)
{
    netsnmp_index tmp;
    oid oid_index[2];
    cpqIdeAtaDiskTable_entry *disk;
    int disk_index = 0;

    int  Cntlr = 0, Bus = 0, Target = 0;

    if (devname != NULL) {
        if (sscanf(devname, "bsg/%d:%d:%d:0", &Cntlr, &Bus, &Target) != 3)
            return;
    } else
        return;

    disk_index = Cntlr + (2 * Target) + 1;
    oid_index[0] = Cntlr;
    oid_index[1] = disk_index;
    tmp.len = 2;
    tmp.oids = &oid_index[0];
    disk = CONTAINER_FIND((netsnmp_container *)data, &tmp);

    if (disk != NULL) {
        disk->cpqIdeAtaDiskCondition = IDE_DISK_COND_FAILED;

        SendIdeTrap(IDE_TRAP_DISK_STATUS_CHANGE, disk);

        CONTAINER_REMOVE((netsnmp_container *)data, disk);
    }
}

int netsnmp_arch_idecntlr_container_load(netsnmp_container* container)
{
    cpqIdeControllerTable_entry *entry;
    cpqIdeControllerTable_entry *ctlr;

    char buffer[256], lbuffer[256], *pbuffer, *sbuffer;
    static int NumIdeHost;
    int CntlrIndex = 1;
    char attribute[256];
    char *value;
    long  rc = 0;
    int i, len;

    DEBUGMSGTL(("idecntlr:container:load", "loading\n"));
    DEBUGMSGTL(("idecntlr:container:load", "Container=%p\n",container));

    /* Find all SCSI Hosts */
    if ((NumIdeHost =
              scandir(ScsiHostDir, &ScsiHostlist, ide_select, alphasort)) <= 0)
        /* Should not happen */
        return -1;

    DEBUGMSGTL(("idecntlr:container:load", "NumIdeHost = %d\n", NumIdeHost));
    for (i=0; i < NumIdeHost; i++) {
        netsnmp_iterator  *it;

        entry = NULL;
        memset(&buffer, 0, sizeof(buffer));
        strncpy(buffer, ScsiHostDir, sizeof(buffer) - 1);
        strncat(buffer, ScsiHostlist[i]->d_name,
                sizeof(buffer) - strlen(buffer) - 1);
        free(ScsiHostlist[i]);

        if ((len = readlink(buffer, lbuffer, 253)) <= 0) 
            continue;

        lbuffer[len]='\0'; /* Null terminate the string */
        if ((pbuffer = strstr(lbuffer, "/ata")) == NULL) 
            continue;
        *pbuffer = '\0';
        if ((sbuffer = strrchr(lbuffer, '/')) == NULL) 
            continue;
        sbuffer++;
        it = CONTAINER_ITERATOR(container);
        ctlr = ITERATOR_FIRST(it);

        while ( ctlr != NULL ) {
            if (!strcmp(sbuffer, ctlr->cpqIdeControllerPciLocation));
                break;
            ctlr = ITERATOR_NEXT(it);
        }
        ITERATOR_RELEASE( it );
        if (ctlr != NULL)
            continue;

        entry = cpqIdeControllerTable_createEntry(container,
                (oid)CntlrIndex);

        if (NULL != entry) {
            memset(entry->cpqIdeControllerPciLocation, 0, 128);
            memset(entry->cpqIdeControllerHwLocation, 0, 128);

            strncpy(entry->cpqIdeControllerPciLocation, sbuffer, 12);
            entry->cpqIdeControllerPciLocation_len = 12;
            DEBUGMSGTL(("idecntlr:container:load",
			            "Entry created %d\n", 
			            CntlrIndex));
            /*  We will need the host name later on */
            CntlrIndex++;
            entry->cpqIdeControllerOverallCondition = IDE_CONTROLLER_STATUS_OK;
            entry->cpqIdeControllerStatus = CPQ_REG_OTHER;

            entry->cpqIdeControllerSlot =  pcislot_scsi_host(buffer);
            if (entry->cpqIdeControllerSlot > 0 ) {
                entry->cpqIdeControllerHwLocation_len = 
                    snprintf(entry->cpqIdeControllerHwLocation, 
			                 128, 
			                 "Slot %ld", 
			                 entry->cpqIdeControllerSlot);
            } else {
                strcpy(entry->cpqIdeControllerHwLocation, "Embedded");
                entry->cpqIdeControllerHwLocation_len = 
			                    strlen(entry->cpqIdeControllerHwLocation);
            }

            strncpy(attribute, buffer, sizeof(attribute) - 1);
            strncat(attribute, sysfs_attr[CLASS_STATE],
                    sizeof(attribute) - strlen(attribute) - 1);
            if ((value = get_sysfs_str(attribute)) != NULL) {
                if (strcmp(value, "running") == 0)
                    entry->cpqIdeControllerStatus = IDE_CONTROLLER_STATUS_OK;
                free(value);
            }

            strcpy(entry->cpqIdeControllerModel, "Standard IDE Controller");
            entry->cpqIdeControllerModel_len = 
                strlen(entry->cpqIdeControllerModel);

            entry->cpqIdeControllerCondition = 
                MAKE_CONDITION(entry->cpqIdeControllerCondition, 
                        entry->cpqIdeControllerStatus);
            rc = CONTAINER_INSERT(container, entry);
            DEBUGMSGTL(("idecntlr:container:load", "container inserted\n"));
        }
    }
    free(ScsiHostlist);

    return(rc);
}


int netsnmp_arch_idedisk_container_load(netsnmp_container* container)
{
    cpqIdeControllerTable_entry *cntlr;
    cpqIdeAtaDiskTable_entry *disk;
    netsnmp_container *cntlr_container;
    netsnmp_iterator  *it;
    netsnmp_cache *cntlr_cache;

    char buffer[256], lbuffer[256], *pbuffer, *sbuffer;
    unsigned int Host = 0, Bus = 0, Target = 0;
    static int NumIdeDisk;
    int j, len;
    int disk_index = 0;

    DEBUGMSGTL(("idedisk:container:load", "loading\n"));
    /*
     * find  the HBa container.
     */
    SataDiskCondition = CPQ_REG_OK;

    cntlr_cache = netsnmp_cache_find_by_oid(cpqIdeControllerTable_oid, 
            cpqIdeControllerTable_oid_len);
    if (cntlr_cache == NULL) {
        return 0;
    }
    cntlr_container = cntlr_cache->magic;
    DEBUGMSGTL(("idedisk:container:load", "Container=%p\n",cntlr_container));
    DEBUGMSGTL(("idedisk:container:load", 
                "ContainerSize=%ld\n", CONTAINER_SIZE(cntlr_container)));
    /* Find all SCSI Hosts */
    if ((NumIdeDisk =
          scandir(ScsiDiskDir, &ScsiDisklist, ide_disk_select, alphasort)) <= 0)
	return 0;

    DEBUGMSGTL(("idedisk:container:load", 
		"SATA disk count = %d\n", 
		NumIdeDisk));
    for (j = 0; j < NumIdeDisk; j++) {
        DEBUGMSGTL(("idedisk:container:load", "Working on disk %s\n", 
                    ScsiDisklist[j]->d_name));
        sscanf(ScsiDisklist[j]->d_name, "%d:%d:%d:0", &Host, &Bus, &Target);
        disk_index = Host + (2 * Target) + 1;

        memset(&buffer, 0, sizeof(buffer));
        strncpy(buffer, ScsiDiskDir, sizeof(buffer) - 1);
        strncat(buffer, ScsiDisklist[j]->d_name,
                sizeof(buffer) - strlen(buffer) - 1);

        if ((len = readlink(buffer, lbuffer, 253)) <= 0)
            continue;

        lbuffer[len]='\0'; /* Null terminate the string */
        if ((pbuffer = strstr(lbuffer, "/ata")) == NULL)
            continue;
        *pbuffer = '\0';
        if ((sbuffer = strrchr(lbuffer, '/')) == NULL)
            continue;
        sbuffer++;
    	DEBUGMSGTL(("idedisk:container:load", "sbuffer = %s\n", sbuffer));
        it = CONTAINER_ITERATOR( cntlr_container );
        cntlr = ITERATOR_FIRST( it );
        while (cntlr != NULL ) {
            DEBUGMSGTL(("idedisk:container:load", "cntlr=%p\n", cntlr));
            DEBUGMSGTL(("idedisk:container:load", 
                        "Starting Loop %ld\n", cntlr->cpqIdeControllerIndex));
    	    DEBUGMSGTL(("idedisk:container:load", 
		                "PciLocation = %s\n", 
		                cntlr->cpqIdeControllerPciLocation));

            cntlr->cpqIdeControllerOverallCondition =
                MAKE_CONDITION(cntlr->cpqIdeControllerOverallCondition,
                        cntlr->cpqIdeControllerCondition);

   	    DEBUGMSGTL(("idedisk:container:load", 
			"Adding disk %d\n", 
			disk_index));
            disk = ide_add_disk(lbuffer, 
				                ScsiDisklist[j]->d_name, 
				                cntlr, 
				                disk_index, 
				                container);

            cntlr->cpqIdeControllerOverallCondition =
                MAKE_CONDITION(cntlr->cpqIdeControllerOverallCondition,
                        disk->cpqIdeAtaDiskStatus);
        cntlr = ITERATOR_NEXT( it );
        } /* controller iterator */
        free(ScsiDisklist[j]);
    ITERATOR_RELEASE( it );
    } /*for j=0, numIdedisk */
    free(ScsiDisklist);

    if ((SataDiskCondition & (1 < (CPQ_REG_DEGRADED -1))) != 0)
        cpqHoMibHealthStatusArray[CPQMIBHEALTHINDEX] = CPQ_REG_DEGRADED;

    DEBUGMSGTL(("idedisk:container", "init\n"));
    udev_register("bsg","add", NULL, cpqide_add_ide_end_device, container);
    udev_register("bsg","remove",NULL, cpqide_remove_ide_end_device, container);

    return 1;
}

void SendIdeTrap(int trapID,
                 cpqIdeAtaDiskTable_entry *disk)
{
    static oid compaq[] = { 1, 3, 6, 1, 4, 1, 232 };
    static oid compaq_len = 7;
    static oid sysName[] = { 1, 3, 6, 1, 2, 1, 1, 5, 0 };
    static oid cpqHoTrapFlags[] = { 1, 3, 6, 1, 4, 1, 232, 11, 2, 11, 1, 0 };
    static oid cpqIdeAtaDiskControllerIndex[] = 
                    { 1, 3, 6, 1, 4, 1, 232, 14, 2, 4, 1, 1, 1 };
    static oid cpqIdeAtaDiskIndex[] =
                    { 1, 3, 6, 1, 4, 1, 232, 14, 2, 4, 1, 1, 2 };
    static oid cpqIdeAtaDiskModel[] = 
                    { 1, 3, 6, 1, 4, 1, 232, 14, 2, 4, 1, 1, 3 };
    static oid cpqIdeAtaDiskFwRev[] = 
                    { 1, 3, 6, 1, 4, 1, 232, 14, 2, 4, 1, 1, 4 };
    static oid cpqIdeAtaDiskSerialNumber[] = 
                    { 1, 3, 6, 1, 4, 1, 232, 14, 2, 4, 1, 1, 5 };
    static oid cpqIdeAtaDiskStatus[] = 
                    { 1, 3, 6, 1, 4, 1, 232, 14, 2, 4, 1, 1, 6 };
    static oid cpqIdeAtaDiskChannel[] = 
                    { 1, 3, 6, 1, 4, 1, 232, 14, 2, 4, 1, 1, 11 };
    static oid cpqIdeAtaDiskNumber[] = 
                    { 1, 3, 6, 1, 4, 1, 232, 14, 2, 4, 1, 1, 12 };

    static oid cpqIdeAtaDiskSSDWearStatus[] = 
    { 1, 3, 6, 1, 4, 1, 232, 14, 2, 4, 1, 1, 19 };
    netsnmp_variable_list *var_list = NULL;

    unsigned int cpqHoTrapFlag;
    DEBUGMSGTL(("idedisk:container:load", "Trap:DiskCondition = %ld\n",
                disk->cpqIdeAtaDiskCondition));
    cpqHoTrapFlag = disk->cpqIdeAtaDiskCondition << 2;
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

    snmp_varlist_add_variable(&var_list, cpqIdeAtaDiskControllerIndex,
                              sizeof(cpqIdeAtaDiskControllerIndex)/sizeof(oid),
                              ASN_INTEGER, 
                              (u_char *) &disk->cpqIdeAtaDiskControllerIndex,
            sizeof(disk->cpqIdeAtaDiskControllerIndex));

    snmp_varlist_add_variable(&var_list, cpqIdeAtaDiskIndex,
                              sizeof(cpqIdeAtaDiskIndex) / sizeof(oid),
                              ASN_INTEGER, (u_char *) &disk->cpqIdeAtaDiskIndex,
            sizeof(disk->cpqIdeAtaDiskIndex));

    snmp_varlist_add_variable(&var_list, cpqIdeAtaDiskModel,
                              sizeof(cpqIdeAtaDiskModel) / sizeof(oid),
                              ASN_OCTET_STR,
                              (u_char *) disk->cpqIdeAtaDiskModel,
                              0);

    snmp_varlist_add_variable(&var_list, cpqIdeAtaDiskFwRev,
                              sizeof(cpqIdeAtaDiskFwRev) / sizeof(oid),
                              ASN_OCTET_STR,
                              (u_char *) disk->cpqIdeAtaDiskFwRev,
                              0);

    snmp_varlist_add_variable(&var_list, cpqIdeAtaDiskSerialNumber,
                              sizeof(cpqIdeAtaDiskSerialNumber) / sizeof(oid),
                              ASN_OCTET_STR,
                              (u_char *) disk->cpqIdeAtaDiskSerialNumber,
                              0);

    if (trapID != IDE_TRAP_DISK_SSD_WEAR_STATUS_CHANGE)
    snmp_varlist_add_variable(&var_list, cpqIdeAtaDiskStatus,
                              sizeof(cpqIdeAtaDiskStatus) / sizeof(oid),
                              ASN_INTEGER, 
                              (u_char *) &disk->cpqIdeAtaDiskStatus,
                sizeof(disk->cpqIdeAtaDiskStatus));
    else
        snmp_varlist_add_variable(&var_list, cpqIdeAtaDiskSSDWearStatus,
                sizeof(cpqIdeAtaDiskSSDWearStatus) / sizeof(oid),
                ASN_INTEGER, 
                (u_char *) &disk->cpqIdeAtaDiskSSDWearStatus,
                sizeof(disk->cpqIdeAtaDiskSSDWearStatus));

    snmp_varlist_add_variable(&var_list, cpqIdeAtaDiskChannel,
                              sizeof(cpqIdeAtaDiskChannel) / sizeof(oid),
                              ASN_INTEGER, 
                              (u_char *) &disk->cpqIdeAtaDiskChannel,
            sizeof(disk->cpqIdeAtaDiskChannel));

    snmp_varlist_add_variable(&var_list, cpqIdeAtaDiskNumber,
                              sizeof(cpqIdeAtaDiskNumber) / sizeof(oid),
                              ASN_INTEGER, 
                              (u_char *) &disk->cpqIdeAtaDiskNumber,
            sizeof(disk->cpqIdeAtaDiskNumber));

    DEBUGMSGTL(("idedisk:container", "SendTrap\n"));
    send_enterprise_trap_vars(SNMP_TRAP_ENTERPRISESPECIFIC,
                    trapID,
                    compaq,
                    compaq_len,
                    var_list);

    snmp_free_varbind(var_list);

}


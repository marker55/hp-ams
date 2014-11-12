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

#include "ide_linux.h"

extern unsigned char     cpqHoMibHealthStatusArray[];

extern int file_select(const struct dirent *);
void SendIdeTrap(int, cpqIdeAtaDiskTable_entry *);

void netsnmp_arch_idecntlr_init(void); 
void netsnmp_arch_idedisk_init(void);

extern  int alphasort();
extern unsigned char*get_ScsiGeneric(unsigned char*);
extern unsigned long long get_BlockSize(unsigned char*);
extern int get_DiskType(char *);
extern char * get_DiskModel(char *);
extern char * get_sata_DiskRev(char *);
extern char * get_DiskState(char *);

static struct dirent **ScsiHostlist;
static char * ScsiHostDir = "/sys/class/scsi_host/";
static int NumScsiHost;

static struct dirent **ScsiDisklist;
static char * ScsiDiskDir = "/sys/class/scsi_disk/";
static int NumScsiDisk;

static struct dirent **BlockDisklist;
static int NumBlockDisk;

static struct dirent **GenericDisklist;
static int NumGenericDisk;

static char *current_Cntlr;

static int disk_select(const struct dirent *entry)
{
    if (strncmp(entry->d_name, current_Cntlr, strlen(current_Cntlr)) == 0)
            return(1);
    return 0;
}

int netsnmp_arch_idecntlr_container_load(netsnmp_container* container)
{

    cpqIdeControllerTable_entry *entry;

    int CntlrIndex=0, Host;
    char buffer[256];
    char attribute[256];
    char *value;
    long  rc = 0;
    int i;

    DEBUGMSGTL(("idecntlr:container:load", "loading\n"));
    DEBUGMSGTL(("idecntlr:container:load", "Container=%p\n",container));

    /* Find all SCSI Hosts */
    if ((NumScsiHost =
            scandir(ScsiHostDir, &ScsiHostlist, file_select, alphasort)) <= 0)
        /* Should not happen */
        return -1;

    for (i=0; i < NumScsiHost; i++) {

        entry = NULL;
        memset(&buffer, 0, sizeof(buffer));
        strncpy(buffer, ScsiHostDir, sizeof(buffer) - 1);
        strncat(buffer, ScsiHostlist[i]->d_name,
                    sizeof(buffer) - strlen(buffer) - 1);

        strncpy(attribute, buffer, sizeof(attribute) - 1);
        strncat(attribute, sysfs_attr[CLASS_PROC_NAME],
            sizeof(attribute) - strlen(attribute) - 1);
        if ((value = get_sysfs_str(attribute)) != NULL) {
            if ((strcmp(value, "ahci") == 0) || 
                (strcmp(value, "ata_piix") == 0)) {
               /*  We will need the host name later on */
                sscanf(ScsiHostlist[i]->d_name, "host%d", &Host);
                CntlrIndex = Host;

                entry = cpqIdeControllerTable_createEntry(container,
                                                          (oid)CntlrIndex);
            }
            free(value);
        }
        
        if (NULL != entry) {
            DEBUGMSGTL(("idecntlr:container:load", "Entry created %d\n", CntlrIndex));
            /*  We will need the host name later on */
            sprintf(entry->host, "%d:", Host);
            entry->cpqIdeControllerOverallCondition = IDE_CONTROLLER_STATUS_OK;
            entry->cpqIdeControllerStatus = CPQ_REG_OTHER;
    
            entry->cpqIdeControllerSlot =  pcislot_scsi_host(buffer);

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
        free(ScsiHostlist[i]);
    }
    free(ScsiHostlist);

    return(rc);
}

int netsnmp_arch_idedisk_container_load(netsnmp_container* container)
{
    cpqIdeControllerTable_entry *cntlr;
    cpqIdeAtaDiskTable_entry *disk;
    cpqIdeAtaDiskTable_entry *old;
    netsnmp_container *cntlr_container;
    netsnmp_iterator  *it;
    netsnmp_cache *cntlr_cache;
    netsnmp_container *fw_container = NULL;
    netsnmp_cache *fw_cache = NULL;

    char buffer[256];
    char attribute[256];
    char *value;
    long long size;
    char *scsi;
    char *generic;

    int Cntlr, Bus, Index, Target;
    char * OS_name;
    char *serialnum = NULL;
    int j;
    long  rc = 0;
    int disk_fd;
    int Health = 0, Temp = -1, Mcot = -1, Wear = -1;
    unsigned char *Temperature;

    netsnmp_index tmp;
    oid oid_index[2];

    DEBUGMSGTL(("idedisk:container:load", "loading\n"));
    /*
     * find  the HBa container.
     */
    SataDiskCondition = CPQ_REG_OK;

    cntlr_cache = netsnmp_cache_find_by_oid(cpqIdeControllerTable_oid, 
                                            cpqIdeControllerTable_oid_len);
    if (cntlr_cache == NULL) {
        return -1;
    }
    cntlr_container = cntlr_cache->magic;
    DEBUGMSGTL(("idedisk:container:load", "Container=%p\n",cntlr_container));
    DEBUGMSGTL(("idedisk:container:load", 
                "ContainerSize=%ld\n", CONTAINER_SIZE(cntlr_container)));
    it = CONTAINER_ITERATOR( cntlr_container );
    cntlr = ITERATOR_FIRST( it );
    DEBUGMSGTL(("idedisk:container:load", "cntlr=%p\n",cntlr));
    while ( cntlr != NULL ) {
        current_Cntlr = cntlr->host;
        DEBUGMSGTL(("idedisk:container:load", 
                    "Starting Loop %s\n", current_Cntlr));
        cntlr->cpqIdeControllerOverallCondition =
                  MAKE_CONDITION(cntlr->cpqIdeControllerOverallCondition,
                                 cntlr->cpqIdeControllerCondition);
        /* Now chack for those HBA's in  the SCSI diskss */
        if ((NumScsiDisk = scandir(ScsiDiskDir, &ScsiDisklist, 
                                   disk_select, alphasort)) <= 0) {
            free(ScsiDisklist);
            cntlr = ITERATOR_NEXT( it );
            continue;
        }	

        for (j= 0; j< NumScsiDisk; j++) {
            memset(&buffer, 0, sizeof(buffer));
            strncpy(buffer, ScsiDiskDir, sizeof(buffer) - 1);
            strncat(buffer, ScsiDisklist[j]->d_name, 
                    sizeof(buffer) - strlen(buffer) - 1);
            DEBUGMSGTL(("idedisk:container:load", "Working on disk %s\n", 
                        ScsiDisklist[j]->d_name));

            sscanf(ScsiDisklist[j]->d_name,"%d:%d:%d:%d",
                        &Cntlr, &Bus, &Index, &Target);

            DEBUGMSGTL(("idedisk:container:load",
                        "looking for cntlr=%ld, Disk = %d\n", 
                        cntlr->cpqIdeControllerIndex, Index));
            oid_index[0] = cntlr->cpqIdeControllerIndex;
            oid_index[1] = Index;
            tmp.len = 2;
            tmp.oids = &oid_index[0];

            old = CONTAINER_FIND(container, &tmp);
            DEBUGMSGTL(("idedisk:container:load", "Old disk=%p\n",old));
            if (old != NULL ) {
                DEBUGMSGTL(("idedisk:container:load", "Re-Use old entry\n"));
                old->OldStatus = old->cpqIdeAtaDiskStatus;
                disk = old;
            } else {
                disk = cpqIdeAtaDiskTable_createEntry(container,
                            (oid)cntlr->cpqIdeControllerIndex, 
                            Index);
                if (disk == NULL) 
                    continue;

                DEBUGMSGTL(("idedisk:container:load", "Entry created\n"));

		scsi = ScsiDisklist[j]->d_name;

                if ((value = get_DiskModel(scsi)) != NULL) {
                    strncpy(disk->cpqIdeAtaDiskModel, value,
                            sizeof(disk->cpqIdeAtaDiskModel) - 1);
                    disk->cpqIdeAtaDiskModel_len = 
                                               strlen(disk->cpqIdeAtaDiskModel);
                    free(value);
                }

                if ((value = get_DiskState(scsi)) != NULL) {
                    if (strcmp(value, "running") == 0)
                        disk->cpqIdeAtaDiskStatus = SAS_PHYS_STATUS_OK;
                    free(value);
                }

                disk->cpqIdeAtaDiskCapacity = get_BlockSize(scsi) >> 11;

                generic = get_ScsiGeneric(scsi);
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
            if ((disk_fd = open(disk->cpqIdeAtaDiskOsName, 
                                O_RDWR | O_NONBLOCK)) >= 0) {
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
            disk->cpqIdeAtaDiskNumber = 4 + Index;

            disk->cpqIdeAtaDiskCondition = 
                            MAKE_CONDITION(disk->cpqIdeAtaDiskCondition, 
                                           disk->cpqIdeAtaDiskStatus);
            cntlr->cpqIdeControllerOverallCondition =
                MAKE_CONDITION(cntlr->cpqIdeControllerOverallCondition,
                               disk->cpqIdeAtaDiskStatus);
            if (old == NULL) {
                rc = CONTAINER_INSERT(container, disk);
                DEBUGMSGTL(("idedisk:container:load", "entry inserted\n"));
            }
            if (((old == NULL) || (disk->OldStatus == 2)) &&
                    (disk->cpqIdeAtaDiskStatus == 3)){
                SendIdeTrap(IDE_TRAP_DISK_STATUS_CHANGE, disk);
                DEBUGMSGTL(("idedisk:container:load", "Sending Trap\n"));
            }
            free(ScsiDisklist[j]);
            SataDiskCondition |=  (1 < (disk->cpqIdeAtaDiskCondition -1));
        }
        free(ScsiDisklist);
        cntlr = ITERATOR_NEXT( it );
    }
    ITERATOR_RELEASE( it );

    if ((SataDiskCondition & (1 < (CPQ_REG_DEGRADED -1))) != 0)
        cpqHoMibHealthStatusArray[CPQMIBHEALTHINDEX] = CPQ_REG_DEGRADED;

    DEBUGMSGTL(("idedisk:container", "init\n"));
    return 1;
}

void SendIdeTrap(int trapID,
                 cpqIdeAtaDiskTable_entry *disk)
{
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

    netsnmp_variable_list *var_list = NULL;

    snmp_varlist_add_variable(&var_list, sysName,
                              sizeof(sysName) / sizeof(oid),
                              ASN_OCTET_STR,
                              (u_char *) "",
                              0);

    snmp_varlist_add_variable(&var_list, cpqHoTrapFlags,
                              sizeof(cpqHoTrapFlags) / sizeof(oid),
                              ASN_INTEGER, 0,
                              sizeof(ASN_INTEGER));

    snmp_varlist_add_variable(&var_list, cpqIdeAtaDiskControllerIndex,
                              sizeof(cpqIdeAtaDiskControllerIndex)/sizeof(oid),
                              ASN_INTEGER, 
                              (u_char *) &disk->cpqIdeAtaDiskControllerIndex,
                              sizeof(ASN_INTEGER));

    snmp_varlist_add_variable(&var_list, cpqIdeAtaDiskIndex,
                              sizeof(cpqIdeAtaDiskIndex) / sizeof(oid),
                              ASN_INTEGER, (u_char *) &disk->cpqIdeAtaDiskIndex,
                              sizeof(ASN_INTEGER));

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

    snmp_varlist_add_variable(&var_list, cpqIdeAtaDiskStatus,
                              sizeof(cpqIdeAtaDiskStatus) / sizeof(oid),
                              ASN_INTEGER, 
                              (u_char *) &disk->cpqIdeAtaDiskStatus,
                              sizeof(ASN_INTEGER));

    snmp_varlist_add_variable(&var_list, cpqIdeAtaDiskChannel,
                              sizeof(cpqIdeAtaDiskChannel) / sizeof(oid),
                              ASN_INTEGER, 
                              (u_char *) &disk->cpqIdeAtaDiskChannel,
                              sizeof(ASN_INTEGER));

    snmp_varlist_add_variable(&var_list, cpqIdeAtaDiskNumber,
                              sizeof(cpqIdeAtaDiskNumber) / sizeof(oid),
                              ASN_INTEGER, 
                              (u_char *) &disk->cpqIdeAtaDiskNumber,
                              sizeof(ASN_INTEGER));

}


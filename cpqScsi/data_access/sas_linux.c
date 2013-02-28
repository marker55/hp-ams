#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <net-snmp/agent/agent_index.h>
#include <net-snmp/library/container.h>
#include <net-snmp/library/snmp_debug.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <scsi/sg.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/errno.h>
#include <getopt.h>
#include <scsi/sg.h>

#include "cpqScsi.h"
#include "cpqSasHbaTable.h"
#include "cpqSasPhyDrvTable.h"

#include "sashba.h"
#include "sasphydrv.h"
#include "common/smbios.h"
#include "common/utils.h"
#include "common/scsi_info.h"

#include "sas_linux.h"

extern unsigned char     cpqHoMibHealthStatusArray[];

extern int file_select(const struct dirent *);
void SendSasTrap(int, cpqSasHbaTable_entry *, cpqSasPhyDrvTable_entry *);
void netsnmp_arch_sashba_init(void); 
void netsnmp_arch_sasphydrv_init(void);

extern  int alphasort();

static struct dirent **SasHostlist;
static char * SasHostDir = "/sys/class/sas_host/";
static int NumSasHost;

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

static char *current_Hba;

static int block_select(const struct dirent *entry)
{
    if (strncmp(entry->d_name,"block:sd",8) == 0)
        return(1);
    return 0;
}

static int generic_select(const struct dirent *entry)
{
    if (strncmp(entry->d_name,"scsi_generic:sg",15) == 0)
        return(1);
    return 0;
}

static int disk_select(const struct dirent *entry)
{
    if (strncmp(entry->d_name,current_Hba,strlen(current_Hba)) == 0)
        return(1);
    return 0;
}

static int hba_select(const struct dirent *entry)
{
    int      i = 0;
    for (i = 0; i < NumSasHost; i++){
        if (strncmp(entry->d_name,
                    SasHostlist[i]->d_name,
                    strlen(entry->d_name)) == 0)
            return(1);
        i++;
    }
    return 0;
}

int netsnmp_arch_sashba_container_load(netsnmp_container* container)
{

    cpqSasHbaTable_entry *entry;
    netsnmp_container *fw_container = NULL;
    netsnmp_cache *fw_cache = NULL;


    sashba_config_buf * hbaConfig = NULL;
    unsigned int vendor, device, BoardID;
    int HbaIndex, Host;
    char buffer[256], lbuffer[256], *pbuffer;
    char attribute[256];
    char *value;
    long  rc = 0;
    int i,len;

    DEBUGMSGTL(("sashba:container:load", "loading\n"));

    DEBUGMSGTL(("sashba:container:load", "Container=%p\n",container));

    HbaCondition = CPQ_REG_OK;

    /* Find all SAS HB's */
    if ((NumSasHost = 
                scandir(SasHostDir, &SasHostlist, file_select, alphasort)) <= 0)
        return -1;

    /* Now chack for those HBA's in  the SCSI hosts */
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
        strcpy(buffer, ScsiHostDir);
        strcat(buffer, ScsiHostlist[i]->d_name);

        strcpy(attribute,buffer);
        strcat(attribute, sysfs_attr[DEVICE_SUBSYSTEM_DEVICE]);
        device = get_sysfs_shex(attribute);

        strcpy(attribute,buffer);
        strcat(attribute, sysfs_attr[DEVICE_SUBSYSTEM_VENDOR]);
        vendor = get_sysfs_shex(attribute);

        BoardID = device << 16;
        BoardID += vendor;

        /*  We will need the host name later on */
        sscanf(ScsiHostlist[i]->d_name, "host%d", &Host); 

        strcpy(attribute,buffer);
        strcat(attribute, sysfs_attr[CLASS_UNIQUE_ID]);
        HbaIndex = get_sysfs_int(attribute);

        entry = cpqSasHbaTable_createEntry(container, (oid)HbaIndex);
        if (NULL == entry)
            continue;   /* error already logged by func */

        DEBUGMSGTL(("sashba:container:load", "Entry created\n"));

        sprintf(entry->host,"%d:",Host);
        entry->cpqSasHbaIndex = HbaIndex;
        entry->cpqSasHbaCondition = CPQ_REG_OTHER;
        entry->cpqSasHbaOverallCondition = CPQ_REG_OTHER;
        entry->cpqSasHbaStatus = CPQ_REG_OTHER;

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

        strcpy(attribute,buffer);
        strcat(attribute, sysfs_attr[CLASS_VERSION_FW]);
        if ((value = get_sysfs_str(attribute)) != NULL) {
            strcpy(entry->cpqSasHbaFwVersion, value);    
            entry->cpqSasHbaFwVersion_len = strlen(entry->cpqSasHbaFwVersion);
            free(value);
        }

        strcpy(attribute,buffer);
        strcat(attribute, sysfs_attr[CLASS_VERSION_BIOS]);
        if ((value = get_sysfs_str(attribute)) != NULL) {
            strcpy(entry->cpqSasHbaBiosVersion, value);    
            entry->cpqSasHbaBiosVersion_len = 
                                            strlen(entry->cpqSasHbaBiosVersion);
            free(value);
        }

        strcpy(attribute, buffer);
        strcat(attribute, sysfs_attr[CLASS_STATE]);
        if ((value = get_sysfs_str(attribute)) != NULL) {
            if (strcmp(value, "running") == 0)
                entry->cpqSasHbaStatus = SAS_HOST_STATUS_OK;
	    free(value);
        }

        entry->cpqSasHbaCondition = 
            MAKE_CONDITION(entry->cpqSasHbaCondition, entry->cpqSasHbaStatus);
        rc = CONTAINER_INSERT(container, entry);
        DEBUGMSGTL(("sashba:container:load", "container inserted\n"));
        HbaCondition |=  (1 < (entry->cpqSasHbaCondition -1));

        entry->cpqSasHbaHwLocation[0] = 0;
        entry->cpqSasHbaHwLocation_len = 0;

        memset(buffer, 0, sizeof(buffer));
        strcpy(buffer, SasHostDir);
        strcat(buffer, ScsiHostlist[i]->d_name);
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
                        sizeof(entry->cpqSasHbaSerialNumber));
                entry->cpqSasHbaSerialNumber_len = 
                    strlen(entry->cpqSasHbaSerialNumber);
                free(hbaConfig);
            }
        }
        free(ScsiHostlist[i]);
    }
    free(ScsiHostlist);

    return(rc);
}

int netsnmp_arch_sasphydrv_container_load(netsnmp_container* container)
{
    cpqSasHbaTable_entry *hba;
    cpqSasPhyDrvTable_entry *disk;
    cpqSasPhyDrvTable_entry *old;
    netsnmp_container *hba_container;
    netsnmp_iterator  *it;
    netsnmp_cache *hba_cache;
    netsnmp_container *fw_container = NULL;
    netsnmp_cache *fw_cache = NULL;

    char buffer[256];
    char attribute[256];
    char *value;
    long long size;

    int Cntlr, Bus, Index, Target;
    char * OS_name;
    char *serialnum = NULL;
    int j;
    long  rc = 0;
    int disk_fd;
    int Health = 0, Speed = 0;
    netsnmp_index tmp;
    oid oid_index[2];

    DEBUGMSGTL(("sasphydrv:container:load", "loading\n"));

    /*
     * find  the HBa container.
     */
    SasDiskCondition = CPQ_REG_OK;

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
        current_Hba = hba->host;
        DEBUGMSGTL(("sasphydrv:container:load", 
                    "Starting Loop %s\n",current_Hba));
        /* Now chack for those HBA's in  the SCSI diskss */
        if ((NumScsiDisk = scandir(ScsiDiskDir, &ScsiDisklist, 
                        disk_select, alphasort)) <= 0)
            return -1;

        for (j= 0; j< NumScsiDisk; j++) {
            memset(&buffer, 0, sizeof(buffer));
            strcpy(buffer, ScsiDiskDir);
            strcat(buffer, ScsiDisklist[j]->d_name);
            DEBUGMSGTL(("sasphydrv:container:load", "Working on disk %s\n",
                        ScsiDisklist[j]->d_name));

            sscanf(ScsiDisklist[j]->d_name,"%d:%d:%d:%d",
                    &Cntlr, &Bus, &Index, &Target);

            DEBUGMSGTL(("sasphydrv:container:load", 
                        "looking for hba=%ld,Index = %d\n", 
                        hba->cpqSasHbaIndex, Index));
            oid_index[0] = hba->cpqSasHbaIndex;
            oid_index[1] = Index;
            tmp.len = 2;
            tmp.oids = &oid_index[0];

            old = CONTAINER_FIND(container, &tmp);
            DEBUGMSGTL(("sasphydrv:container:load", "Old disk=%p\n",old));
            if (old != NULL ) {
                DEBUGMSGTL(("sasphydrv:container:load", "Re-Use old entry\n"));
                old->OldStatus = old->cpqSasPhyDrvStatus;
                disk = old;
            } else {
                disk = cpqSasPhyDrvTable_createEntry(container,
                        (oid)hba->cpqSasHbaIndex, 
                        Index);
                if (disk == NULL) 
                    continue;

                DEBUGMSGTL(("sasphydrv:container:load", "Entry created\n"));

                strcpy(attribute,buffer);
                strcat(attribute, sysfs_attr[DEVICE_REV]);
                if ((value = get_sysfs_str(attribute)) != NULL) {
                    strcpy(disk->cpqSasPhyDrvFWRev, value);
                    disk->cpqSasPhyDrvFWRev_len = 
                                                strlen(disk->cpqSasPhyDrvFWRev);
                    free(value);
                }

                strcpy(attribute,buffer);
                strcat(attribute, sysfs_attr[DEVICE_MODEL]);
                if ((value = get_sysfs_str(attribute)) != NULL) {
                    strcpy(disk->cpqSasPhyDrvModel, value);
                    disk->cpqSasPhyDrvModel_len =
                                                strlen(disk->cpqSasPhyDrvModel);
                    free(value);
                }

                strcpy(attribute,buffer);
                strcat(attribute, sysfs_attr[DEVICE_STATE]);
                if ((value = get_sysfs_str(attribute)) != NULL) {
                    if (strcmp(value, "running") == 0)
                        disk->cpqSasPhyDrvStatus = SAS_PHYS_STATUS_OK;
                }

                strcpy(attribute,buffer);
                strcat(attribute,"/device/");

                /* Look for block:sd? */
                if ((NumBlockDisk = scandir(attribute, &BlockDisklist,
                                block_select, alphasort)) > 0) {

                    strcat(attribute,BlockDisklist[0]->d_name);
                    strcat(attribute,sysfs_attr[BLOCK_SIZE]);
                    size = get_sysfs_llong(attribute);
                    /* Size is in 512 block, need mmbytes 
                       so left shift 11 bits */
                    disk->cpqSasPhyDrvSize = size>>11;
                    free(BlockDisklist[0]);
                    free(BlockDisklist);
                }

                strcpy(attribute,buffer);
                strcat(attribute,"/device/");

                /* Look for scsi_genric:sg? */
                if ((NumGenericDisk = scandir(attribute, &GenericDisklist,
                                generic_select, alphasort))  >0) {

                    DEBUGMSGTL(("sasphydrv:container:load", "generic = %s\n",
                                GenericDisklist[0]->d_name));
                    OS_name = index(GenericDisklist[0]->d_name,':');
                    OS_name++;
                    memset(disk->cpqSasPhyDrvOsName, 0, 256);
                    sprintf(disk->cpqSasPhyDrvOsName, "/dev/%s",OS_name);
                    disk->cpqSasPhyDrvOsName_len = 
                                              strlen(disk->cpqSasPhyDrvOsName);
                    free(GenericDisklist[0]);
                    free(GenericDisklist);
                }
            }  /* New Entry */

            disk_fd = open(disk->cpqSasPhyDrvOsName, O_RDWR | O_NONBLOCK);
            if (disk_fd >= 0) {
                disk->cpqSasPhyDrvUsedReallocs = get_defect_data_size(disk_fd);
                if (disk->cpqSasPhyDrvUsedReallocs == -1) 
                    disk->cpqSasPhyDrvUsedReallocs = 0;

                if ((serialnum = get_unit_sn(disk_fd)) != NULL  ) {
                    strncpy(disk->cpqSasPhyDrvSerialNumber, serialnum, 40);
                    disk->cpqSasPhyDrvSerialNumber_len = 
                        strlen(disk->cpqSasPhyDrvSerialNumber);
                    free(serialnum);
                }

                if ((Health = get_unit_health(disk_fd)) >= 0) {
                    if (Health == 0) {
                        disk->cpqSasPhyDrvStatus = 2; /* OK */
                        disk->cpqSasPhyDrvCondition = 2; /* OK */
                    } else {
                        disk->cpqSasPhyDrvStatus = 3; /* predictive failure */
                        disk->cpqSasPhyDrvCondition = 3; /*  Degraded*/
                    }
                } else {
                    disk->cpqSasPhyDrvStatus = 1; /* Other */
                    disk->cpqSasPhyDrvCondition = 1; /* Other */
                }

                disk->cpqSasPhyDrvRotationalSpeed = CPQ_REG_OTHER;
                if ((Speed = get_unit_speed(disk_fd)) >= 0) { 
                    if (Speed > ROT_SPEED_7200_MIN)
                        disk->cpqSasPhyDrvRotationalSpeed++;
                    if (Speed > ROT_SPEED_10K_MIN)
                        disk->cpqSasPhyDrvRotationalSpeed++;
                    if (Speed > ROT_SPEED_15K_MIN)
                        disk->cpqSasPhyDrvRotationalSpeed++;
                    if (Speed > ROT_SPEED_15K_MAX)
                        disk->cpqSasPhyDrvRotationalSpeed =  CPQ_REG_OTHER;
                }
                close(disk_fd);
            }
            disk->cpqSasPhyDrvMemberLogDrv= 1;
            disk->cpqSasPhyDrvPlacement = 1;
            disk->cpqSasPhyDrvHotPlug= 1;
            disk->cpqSasPhyDrvType = 2;
            disk->cpqSasPhyDrvSasAddress[0] = 0;
            disk->cpqSasPhyDrvSasAddress_len = 0;


            disk->cpqSasPhyDrvCondition = 
                MAKE_CONDITION(disk->cpqSasPhyDrvCondition, 
                        disk->cpqSasPhyDrvStatus);
            hba->cpqSasHbaOverallCondition = 
                MAKE_CONDITION(hba->cpqSasHbaOverallCondition, 
                               disk->cpqSasPhyDrvCondition); 
            if (old == NULL) {
                rc = CONTAINER_INSERT(container, disk);
                DEBUGMSGTL(("sasphydrv:container:load", "entry inserted\n"));
            }
            if (((old == NULL) || (disk->OldStatus == 2)) && 
                    (disk->cpqSasPhyDrvStatus == 3)){
                SendSasTrap(SAS_TRAP_PHYSDRV_STATUS_CHANGE, hba, disk);       
                DEBUGMSGTL(("sasphydrv:container:load", "Sending Trap\n"));
            }
            free(ScsiDisklist[j]);
            SasDiskCondition |=  (1 < (disk->cpqSasPhyDrvCondition -1));
        }
        free(ScsiDisklist);
        hba = ITERATOR_NEXT( it );
    }
    ITERATOR_RELEASE( it );

    if ((SasDiskCondition & (1 < (CPQ_REG_DEGRADED -1))) != 0)
        cpqHoMibHealthStatusArray[CPQMIBHEALTHINDEX] = CPQ_REG_DEGRADED;

    DEBUGMSGTL(("sasphydrv:container", "init\n"));
    return  1;

}

void SendSasTrap(int trapID,     
        cpqSasHbaTable_entry *hba,
        cpqSasPhyDrvTable_entry *disk)
{
    static oid compaq[] = { 1, 3, 6, 1, 4, 1, 232 };
    static oid compaq_len = 7;
    static oid sysName[] = { 1, 3, 6, 1, 2, 1, 1, 5, 0 };
    static oid cpqHoTrapFlags[] = { 1, 3, 6, 1, 4, 1, 232, 11, 2, 11, 1, 0 };
    static oid cpqSasHbaHwLocation[] = 
    { 1, 3, 6, 1, 4, 1, 232, 5, 5, 1, 1, 2 };
    static oid cpqSasPhyDrvLocationString[] = 
    { 1, 3, 6, 1, 4, 1, 232, 5, 5, 2, 1, 3 };
    static oid cpqSasPhyDrvHbaIndex[] = 
    { 1, 3, 6, 1, 4, 1, 232, 5, 5, 2, 1, 1 };
    static oid cpqSasPhyDrvIndex[] =  
    { 1, 3, 6, 1, 4, 1, 232, 5, 5, 2, 1, 2 };
    static oid cpqSasPhyDrvStatus[] = 
    { 1, 3, 6, 1, 4, 1, 232, 5, 5, 2, 1, 5 };
    static oid cpqSasPhyDrvType[] =
    { 1, 3, 6, 1, 4, 1, 232, 5, 5, 2, 1, 16 };
    static oid cpqSasPhyDrvModel[] =
    { 1, 3, 6, 1, 4, 1, 232, 5, 5, 2, 1, 4 };
    static oid cpqSasPhyDrvFWRev[] =
    { 1, 3, 6, 1, 4, 1, 232, 5, 5, 2, 1, 7 };
    static oid cpqSasPhyDrvSerialNumber[] =
    { 1, 3, 6, 1, 4, 1, 232, 5, 5, 2, 1, 10 };
    static oid cpqSasPhyDrvSasAddress[] = 
    { 1, 3, 6, 1, 4, 1, 232, 5, 5, 2, 1, 17 };

    netsnmp_variable_list *var_list = NULL, *vars = NULL;
    int status, oldstatus;

    snmp_varlist_add_variable(&var_list, sysName,
            sizeof(sysName) / sizeof(oid),
            ASN_OCTET_STR,
            (u_char *) "",
            0);

    snmp_varlist_add_variable(&var_list, cpqHoTrapFlags,
            sizeof(cpqHoTrapFlags) / sizeof(oid),
            ASN_INTEGER, 0,
            sizeof(ASN_INTEGER));

    snmp_varlist_add_variable(&var_list, cpqSasHbaHwLocation,
            sizeof(cpqSasHbaHwLocation) / sizeof(oid),
            ASN_OCTET_STR,
            (u_char *) "",
            0);

    snmp_varlist_add_variable(&var_list, cpqSasPhyDrvLocationString,
            sizeof(cpqSasPhyDrvLocationString) / sizeof(oid),
            ASN_OCTET_STR,
            (u_char *) "",
            0);

    snmp_varlist_add_variable(&var_list, cpqSasPhyDrvHbaIndex,
            sizeof(cpqSasPhyDrvHbaIndex) / sizeof(oid),
            ASN_INTEGER, (u_char *) &hba->cpqSasHbaIndex,
            sizeof(ASN_INTEGER));

    snmp_varlist_add_variable(&var_list, cpqSasPhyDrvIndex,
            sizeof(cpqSasPhyDrvIndex) / sizeof(oid),
            ASN_INTEGER, (u_char *)&disk->cpqSasPhyDrvIndex,
            sizeof(ASN_INTEGER));

    snmp_varlist_add_variable(&var_list, cpqSasPhyDrvStatus,
            sizeof(cpqSasPhyDrvStatus) / sizeof(oid),
            ASN_INTEGER,(u_char *)&disk->cpqSasPhyDrvStatus,
            sizeof(ASN_INTEGER));

    snmp_varlist_add_variable(&var_list, cpqSasPhyDrvType,
            sizeof(cpqSasPhyDrvType) / sizeof(oid),
            ASN_INTEGER,(u_char *)&disk->cpqSasPhyDrvType,
            sizeof(ASN_INTEGER));

    snmp_varlist_add_variable(&var_list, cpqSasPhyDrvModel,
            sizeof(cpqSasPhyDrvModel)/ sizeof(oid),
            ASN_OCTET_STR,
            (u_char *) disk->cpqSasPhyDrvModel,
            disk->cpqSasPhyDrvModel_len);

    snmp_varlist_add_variable(&var_list, cpqSasPhyDrvFWRev,
            sizeof(cpqSasPhyDrvFWRev) / sizeof(oid),
            ASN_OCTET_STR,
            (u_char *) disk->cpqSasPhyDrvFWRev,
            disk->cpqSasPhyDrvFWRev_len);

    snmp_varlist_add_variable(&var_list, cpqSasPhyDrvSerialNumber,
            sizeof(cpqSasPhyDrvSerialNumber) / sizeof(oid),
            ASN_OCTET_STR,
            (u_char *) disk->cpqSasPhyDrvSerialNumber,
            disk->cpqSasPhyDrvSerialNumber_len);

    snmp_varlist_add_variable(&var_list, cpqSasPhyDrvSasAddress,
            sizeof(cpqSasPhyDrvSasAddress) / sizeof(oid),
            ASN_OCTET_STR,
            (u_char *) disk->cpqSasPhyDrvSasAddress,
            disk->cpqSasPhyDrvSasAddress_len);

    switch(trapID)
    {
        case SAS_TRAP_PHYSDRV_STATUS_CHANGE:

            netsnmp_send_traps(SNMP_TRAP_ENTERPRISESPECIFIC,
                    SAS_TRAP_PHYSDRV_STATUS_CHANGE,
                    compaq,
                    compaq_len,
                    vars,
                    NULL,
                    0);
            status = disk->cpqSasPhyDrvStatus;
            oldstatus = disk->OldStatus;


            break;

        default:
            return;
    }

}


#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <net-snmp/library/container.h>
#include <net-snmp/library/snmp_debug.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <dirent.h>
#include <sys/ioctl.h>
#include "nvme.h"
#include "cpqSePCIeDiskTable.h"
#include "cpqSePciSlotTable.h"
#include "common/utils.h"
#include "common/smbios.h"

#include "common/scsi_info.h"

#define MAKE_CONDITION(total, new)  (new > total ? new : total)

static struct dirent **NVMeDevicelist;
static char * NVMeDeviceDir = "/sys/block/";
static int NumNVMeDevice;

static int nvme_select(const struct dirent *entry)
{
    if (strncmp(entry->d_name,"nvme", 4) == 0)
        return(1);
    return 0;
}
typedef struct nvme_smart_log _smartlog;

_smartlog *get_nvme_smart_log(int fd)
{
    struct nvme_admin_cmd command;
    _smartlog *smart_log;

    smart_log = malloc(sizeof(_smartlog));
    if (smart_log == NULL)
        return smart_log;

    memset(smart_log, 0, sizeof(_smartlog));
    memset(&command, 0, sizeof(command));

    command.opcode = nvme_admin_get_log_page;
    command.addr = smart_log;
    command.data_len = sizeof(_smartlog);
    command.cdw10 = 0x2 | (((sizeof(_smartlog) / 4) - 1) << 16);
    command.nsid = 0xffffffff;
    if (ioctl(fd, NVME_IOCTL_ADMIN_CMD, &command) < 0) {
        free(smart_log);
        return NULL;
    } else
        return smart_log;
}

int get_Bus(char *deviceLink)
{
    int Bus;
    char buffer[16];
    char *Bus_str;

    memset(buffer, 0, 16);

    Bus_str = strrchr(deviceLink,':');
    Bus_str -= 2;
    strncpy(buffer, Bus_str, 2);
    Bus = strtol(buffer, NULL, 16);
    return Bus;
}

int get_Dev(char *deviceLink)
{
    int Dev;
    char buffer[16];
    char *Dev_str;

    memset(buffer, 0, 16);

    Dev_str = strrchr(deviceLink, ':');
    Dev_str += 1;
    strncpy(buffer, Dev_str, 2);
    Dev = strtol(buffer, NULL, 16);
    return Dev;
}

char * get_DiskName(char *deviceLink)
{
    char *token = NULL;
    char *enddevice = NULL;
    int name_len = 0;
    DEBUGMSGTL(("nvmedrv:container:load", "get_DiskName devicelink = %s\n", 
                deviceLink));
    if ((token = strrchr(deviceLink, '/')) != NULL) {
        token += 1;
        name_len = strlen(token) + 1;
        enddevice = malloc(name_len);
        if (enddevice != NULL) {
            memset(enddevice, 0, name_len);
            strncpy(enddevice, token, name_len);
        }
    }
    DEBUGMSGTL(("nvmedrv:container:load", "get_EndDevice %s\n", enddevice));
    return (enddevice);
}

int get_Func(char *deviceLink)
{
    int Func;
    char buffer[16];
    char *Func_str;

    memset(buffer, 0, 16);

    Func_str = strrchr(deviceLink, '.');
    Func_str += 1;
    strncpy(buffer, Func_str, 1);
    Func = strtol(buffer, NULL, 10);
    return Func;
}

cpqSePCIeDiskTable_entry *nvme_add_disk(char *deviceLink, 
        char *devname,
        char *devtype,
        netsnmp_container* container)
{

    struct nvme_smart_log *smart_log;

    netsnmp_index tmp;
    oid oid_index[3];
    cpqSePCIeDiskTable_entry *old;
    cpqSePCIeDiskTable_entry *disk;
    int disk_fd = -1;
    char *serialnum = NULL;
    int Health = 0, Temp = -1, Mcot = -1, Wear = -1;
    char *Temperature;

    DEBUGMSGTL(("nvmedrv:container:load", "\nAdding new disk %s\n", devname));
    DEBUGMSGTL(("nvmedrv:container:load", "device Link %s\n", deviceLink));
    oid_index[0] = get_Bus(deviceLink);
    oid_index[1] = get_Dev(deviceLink);
    oid_index[2] = get_Func(deviceLink);
    tmp.len = 3;
    tmp.oids = &oid_index[0];

    old = CONTAINER_FIND(container, &tmp);
    DEBUGMSGTL(("nvmedrv:container:load", "Old disk=%p\n", old));
    if (old != NULL ) {
        DEBUGMSGTL(("nvmedrv:container:load", "Re-Use old entry\n"));
        old->OldCondition = old->cpqSePCIeDiskCondition;
        disk = old;
        disk_fd = open(disk->cpqSePCIeDiskOsName, O_RDWR | O_NONBLOCK);
    } else {
        cpqSePciSlotTable_entry* pci;
        netsnmp_container *pci_container;
        netsnmp_cache *pci_cache;
        netsnmp_iterator  *it;

        disk = cpqSePCIeDiskTable_createEntry(container,
                oid_index[0],
                oid_index[1],
                oid_index[2]);
        if (disk == NULL) {
            return disk;
        }
        DEBUGMSGTL(("nvmedrv:container:load", "Entry created\n"));

        pci_cache = netsnmp_cache_find_by_oid(cpqSePciSlotTable_oid,
                cpqSePciSlotTable_oid_len);
        if (pci_cache == NULL) {
            return ;
        }
        pci_container = pci_cache->magic;
        it = CONTAINER_ITERATOR(pci_container);
        pci = ITERATOR_FIRST(it);

        while ( pci != NULL ) {
            if ((oid_index[0] == pci->cpqSePciSlotBusNumberIndex) && 
                (oid_index[1] == pci->cpqSePciSlotDeviceNumberIndex)) {
                disk->cpqSePCIeDiskHwLocation_len = 
                    snprintf(disk->cpqSePCIeDiskHwLocation, 255,
                             "Cage %d Bay %d",
                            pci->cpqSePciPhysSlot >> 8,
                            pci->cpqSePciPhysSlot & 0xFF);
            }
            pci = ITERATOR_NEXT( it );
        }
        ITERATOR_RELEASE( it );

        disk->cpqSePCIeDiskPercntEndrnceUsed = -1;
        disk->cpqSePCIeDiskWearStatus = SSD_WEAR_OTHER;
        disk->cpqSePCIeDiskPowerOnHours = -1;
        disk->cpqSePCIeDiskCondition = NVME_COND_OK;

        memset(disk->cpqSePCIeDiskOsName, 0, 256);
        disk->cpqSePCIeDiskOsName_len =
            snprintf(disk->cpqSePCIeDiskOsName, 255, "%s", devname);

        disk_fd = open(disk->cpqSePCIeDiskOsName, O_RDWR | O_NONBLOCK);
        if (disk_fd >= 0) {
            char *Vendor;
            char *ProductID;
            char *Rev;
            char *Identify;
            if ((serialnum = get_unit_sn(disk_fd)) != NULL  ) {
                memset(disk->cpqSePCIeDiskSerialNumber, 0, 41);
                strncpy(disk->cpqSePCIeDiskSerialNumber, serialnum,
                        sizeof(disk->cpqSePCIeDiskSerialNumber) - 1);
                disk->cpqSePCIeDiskSerialNumber_len =
                    strlen(disk->cpqSePCIeDiskSerialNumber);
                free(serialnum);
            }
            if ((Identify = get_identify_info(disk_fd)) != NULL ) {
                memset(disk->cpqSePCIeDiskModel, 0, 256);
                memset(disk->cpqSePCIeDiskModel, ' ', 8);
                Vendor = inq_parse_vendor(Identify);
                ProductID = inq_parse_prodID(Identify);
                Rev = inq_parse_rev(Identify, 8);
                if (Vendor != NULL) {
                    DEBUGMSGTL(("nvmedrv:container:load", "Vendor = %s\n", 
                                Vendor));
                    disk->cpqSePCIeDiskModel_len =
                        snprintf(disk->cpqSePCIeDiskModel, 255, "%s    ", 
                                 Vendor);
                    free(Vendor);
                } else {
                    strncat(disk->cpqSePCIeDiskModel,"HP      ", 8);
                    disk->cpqSePCIeDiskModel_len = 8;
                }
                if (ProductID != NULL) {
                    DEBUGMSGTL(("nvmedrv:container:load", "ProductID = %s\n", 
                                ProductID));
                    strncat(disk->cpqSePCIeDiskModel, ProductID,
                            sizeof(disk->cpqSePCIeDiskModel) - 1);
                    disk->cpqSePCIeDiskModel_len =
                        strlen(disk->cpqSePCIeDiskModel);
                    free(ProductID);
                }
                if (Rev != NULL) {
                    DEBUGMSGTL(("nvmedrv:container:load", "Rev = %s\n", Rev));
                    memset(disk->cpqSePCIeDiskFwRev, 0, 41);
                    strncpy(disk->cpqSePCIeDiskFwRev, Rev,
                            sizeof(disk->cpqSePCIeDiskFwRev) - 1);
                    disk->cpqSePCIeDiskFwRev_len =
                        strlen(disk->cpqSePCIeDiskFwRev);
                    free(Rev);
                }
                free(Identify);
            }
        }

        disk->cpqSePCIeDiskCapacityMb = 
                        get_PciBlockSize((unsigned char *)deviceLink) >> 11;
        DEBUGMSGTL(("nvmedrv:container:load", "DiskSize = %lld\n", 
                    disk->cpqSePCIeDiskCapacityMb));

    }  /* New Entry */

    disk->cpqSePCIeDiskWearStatus = NVME_WEAR_STATUS_OK;

    if (disk_fd <= 0) {
        if((disk_fd = open(disk->cpqSePCIeDiskOsName, O_RDWR | O_NONBLOCK)) < 0)
            return disk;;
    }

    if ((Temperature = get_sas_temp(disk_fd)) != NULL ) {
        Temp = sas_parse_current(Temperature);
        Mcot = sas_parse_mcot(Temperature);
        disk->cpqSePCIeDiskCurrentTemperature = Temp;
        disk->cpqSePCIeDiskThresholdTemperature = Mcot;
        free(Temperature);
    } else {
        if ((Temp = get_unit_temp(disk_fd)) >= 0)
            disk->cpqSePCIeDiskCurrentTemperature = Temp;
    }
    smart_log = get_nvme_smart_log(disk_fd);
    if (smart_log != NULL) {
        disk->cpqSePCIeDiskPercntEndrnceUsed = smart_log->percent_used;
        disk->cpqSePCIeDiskPowerOnHours = smart_log->power_on_hours[0] +
            (smart_log->power_on_hours[1] << 8) +
            (smart_log->power_on_hours[1] << 16) +
            (smart_log->power_on_hours[1] << 24);
        if (smart_log->critical_warning != 0)
            disk->cpqSePCIeDiskCondition = NVME_COND_DEGRADED;
        free(smart_log);
    } else {
        Wear = get_sas_ssd_wear(disk_fd);
        DEBUGMSGTL(("nvmedrv:container:load", "Wear is %d%%\n", Wear));
        disk->cpqSePCIeDiskPercntEndrnceUsed = Wear;
        if ((Health = get_sas_health(disk_fd)) >= 0) {
            if (((Health != 0) && (Wear >= 95)) || (Temp > Mcot)) {
                disk->cpqSePCIeDiskCondition = NVME_COND_DEGRADED;
            } else {
                disk->cpqSePCIeDiskCondition = NVME_COND_OK;
            }
        } else 
            disk->cpqSePCIeDiskCondition = NVME_COND_FAILED;
        DEBUGMSGTL(("nvmedrv:container:load",
                    "Current Temp = %dC,  Maximum Temp = %dC\n", Temp, Mcot));
    }
    close(disk_fd);

    if (disk->cpqSePCIeDiskPercntEndrnceUsed >= 95)
        disk->cpqSePCIeDiskWearStatus = NVME_WEAR_STATUS_FIVEPERCENT;
    if (disk->cpqSePCIeDiskPercntEndrnceUsed >= 98)
        disk->cpqSePCIeDiskWearStatus = NVME_WEAR_STATUS_TWOPERCENT;
    if (disk->cpqSePCIeDiskPercntEndrnceUsed >= 100)
        disk->cpqSePCIeDiskWearStatus = NVME_WEAR_STATUS_WEAROUT;


    cpqSePCIeDiskTableCondition = 
        MAKE_CONDITION(cpqSePCIeDiskTableCondition,
                disk->cpqSePCIeDiskCondition);
    if (old == NULL) {
        CONTAINER_INSERT(container, disk);
        DEBUGMSGTL(("nvmedrv:container:load", "entry inserted\n"));
    }

    /* send a trap if drive is failed or degraded on the first time
       through or it it changes from OK to degraded of failed */
    /*
       if ((old == NULL) || (disk->OldCondition == NVME_COND_OK)) {
       switch (disk->cpqSePCIeDiskCondition) {
       case NVME_COND_FAILED:
       SendNvmeTrap(SAS_TRAP_PHYSDRV_STATUS_CHANGE, disk);
       break;
       default:
       break;
       }
       }
       */

    return disk;
}


void cpqnvme_add_disk(char *devpath, char *devname, char *devtype, void *data)
{
    cpqSePCIeDiskTable_entry *disk = NULL;

    if (strstr(devpath,"nvme") == NULL)
        return;
    if (devpath != NULL) {
        DEBUGMSGTL(("nvmedrv:container:load",
                    "\nNew disk inserted %s\n", devname));
        disk = nvme_add_disk(devpath, devname, devtype,
                (netsnmp_container *)data);
        /*
           if (disk != NULL) {
           disk->cpqSePCIeDiskCondition = NVME_COND_OK;
           SendSasTrap(NVME_TRAP_DRV_STATUS_CHANGE, disk);
           }
           */
    }
}

static void cpqnvme_remove_disk(char *devpath, char *devname, char *devtype, void *data)
{
    netsnmp_index tmp;
    oid oid_index[3];
    cpqSePCIeDiskTable_entry *disk;

    DEBUGMSGTL(("nvmedrv:container:load", "\nRemoving new disk %s\n", devname));
    oid_index[0] = get_Bus(devpath);
    oid_index[1] = get_Dev(devpath);
    oid_index[2] = get_Func(devpath);
    tmp.len = 3;
    tmp.oids = &oid_index[0];

    disk = CONTAINER_FIND((netsnmp_container *)data, &tmp);

    if (disk != NULL) {
#ifdef NOTDEF
        switch (disk->cpqSePCIeDiskCondition) {
            case 2:  /* ok(2) */
            case 3:  /* predictiveFailure(3) */
            case 4:  /* offline(4) */
            case 5:  /* failed(5) */
                disk->cpqSePCIeDiskCondition += 4;/* add missing */
                break;
            case 10:  /* ssdWearOut(10) */
                disk->cpqSePCIeDiskCondition =
                    NVME_COND_MISSINGWASSSDWEAROUT;
                break;
            case 12:  /* notAuthenticated(12) */
                disk->cpqSePCIeDiskCondition =
                    NVME_COND_MISSINGWASNOTAUTHENTICATED;
                break;
            default:
                break;
        }
        disk->cpqSePCIeDiskCondition = NVME_COND_FAILED;
        SendNVMeTrap(SAS_TRAP_PHYSDRV_STATUS_CHANGE, disk);

#endif
        CONTAINER_REMOVE((netsnmp_container *)data, disk);
    }
}



int netsnmp_arch_nvmedrv_container_load(netsnmp_container* container)
{
    cpqSePCIeDiskTable_entry *disk = NULL;
    int len;
    int j;

    DEBUGMSGTL(("nvmedrv:container:load", "loading\n"));

    /* Now chack for those NVME disks */
    if ((NumNVMeDevice = scandir(NVMeDeviceDir, &NVMeDevicelist,
                    nvme_select, alphasort)) <= 0) {
        return -1;
    }

    for (j= 0; j< NumNVMeDevice; j++) {
        char buffer[256], lbuffer[256];
        char *token, *devname;
        int devlen = 0;
        memset(&buffer, 0, sizeof(buffer));
        strncpy(buffer, NVMeDeviceDir, sizeof(buffer) - 1);
        strncat(buffer, NVMeDevicelist[j]->d_name,
                sizeof(buffer) - strlen(buffer) - 1);
        free(NVMeDevicelist[j]);
        if ((len = readlink(buffer, lbuffer, 253)) <= 0)
            continue;

        lbuffer[len]='\0'; /* Null terminate the string */
        /* if it's not the controller we are working on, skip it */
        if (token != NULL) {
            token = strrchr(lbuffer, '/');
            token += 1;
            devlen = strlen(token) + 6;
            devname = malloc(devlen);
            if (devname != NULL) {
                memset(devname, 0, devlen);
                strncpy(devname, "/dev/", 5);
                strncat(devname, token, strlen(token));
                disk = nvme_add_disk(lbuffer, devname, "disk", container);
                if (disk != NULL) 
                    disk->cpqSePCIeDiskOsName_len =
                        snprintf(disk->cpqSePCIeDiskOsName, 255, "%s", devname);
                free(devname);
            }
        }

        if (disk != NULL) {
            cpqSePCIeDiskTableCondition =
                MAKE_CONDITION(cpqSePCIeDiskTableCondition,
                        disk->cpqSePCIeDiskCondition);
#ifdef NOTDEF
            if (trap_fire)
                SendNVMeTrap(SAS_TRAP_PHYSDRV_STATUS_CHANGE, disk);
#endif
        }
    }
    free(NVMeDevicelist);

    DEBUGMSGTL(("nvmedrv:container", "init\n"));
    udev_register("block","add", "disk", cpqnvme_add_disk, container);
    udev_register("block","remove", "disk", cpqnvme_remove_disk, container);
    return  1;
}



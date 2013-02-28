#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <net-snmp/library/container.h>
#include <net-snmp/library/snmp_debug.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <dirent.h>
#include "cpqSePciFunctTable.h"
#include "cpqSePciSlotTable.h"
#include "cpqSePciMemoryTable.h"
#include "common/utils.h"
#include "common/smbios.h"
#include "pci_linux.h"

typedef struct _slot_info {
    int dom;
    int bus;
    int dev;
    unsigned int  type;
    int width;
    int speed;
    int used;
} slot_info;

slot_info **slots;
int slotcount = -1;
pci_node *pci_root = NULL;
vendor_node *vendor_root = NULL;
device_node *device_root = NULL;
subsystem_node *subsystem_root = NULL;
class_node *class_root = NULL;
char *pci_ids[] = {"/usr/share/hwdata/pci.ids",
		   "/usr/share/pci.ids",
		   "/usr/share/misc/pci.ids",
		   NULL};


#define SYSBUSPCIDEVICES "/sys/bus/pci/devices/"
unsigned int pci_get_caps(unsigned char *config)
{
    int cap = 0;
    int cap_index = 0;

    if (config[6] & 0x10) {
        if ((cap_index = config[0x34]) != 0) {
            while (cap_index != 0) {
                cap |= ( 1 << (config[cap_index] -1));
                cap_index = config[cap_index + 1];
            }
        }
    }
    return cap;
}


int get_pci_speed(pci_node *dev)
{
    int i;
    if (slotcount > 0) {
        for (i = 0; i < slotcount; i++) {
            if ((slots[i]->dom == dev->domain) &&
                (slots[i]->bus == dev->bus) &&
                (slots[i]->dev == dev->dev))
            return (slots[i]->speed);
        }
        return 3;
    } else {
        /* tricky stuff that we need to get from ACPI ourself */
        return 3;
    }
}

int get_pci_width(pci_node *dev)
{
    int i;
    if (slotcount > 0) {
        for (i = 0; i < slotcount; i++) {
            if ((slots[i]->dom == dev->domain) &&
                (slots[i]->bus == dev->bus) &&
                (slots[i]->dev == dev->dev))
            return (slots[i]->width - 2);
        }
        return 2;
    } else {
        /* tricky stuff that we need to get from ACPI ourself */
        return 2;
    }
}

int get_pci_slot(pci_node *dev)
{
    int i;

    if (slotcount > 0) {
        for (i = 0; i < slotcount; i++) {
            if (slots[i]->dom == dev->domain)
                if (slots[i]->bus == dev->bus)
                    if (slots[i]->dev == dev->dev)
                        return (i + 1);
        }
        return 0;
    } else {
        /* tricky stuff that we need to get from ACPI ourself */
        return 0;
    }
}

int get_pci_type(pci_node *dev)
{
    if (slotcount > 0) {
        if (dev->caps & 0x8000 )
            return PCI_SLOT_TYPE_PCIEXPRESS;
        else
            return PCI_SLOT_TYPE_UNKNOWN;
    } else {
        /* tricky stuff that we need to get from ACPI ourself */
        return PCI_SLOT_TYPE_OTHER;
    }
}

#ifdef NOT_NEEDED
int get_pci_current_speed(pci_node *dev)
{
    int i;
    if (slotcount > 0) {
        for (i = 0; i < slotcount; i++) {
            if ((slots[i]->dom == dev->domain) &&
                (slots[i]->bus == dev->bus) &&
                (slots[i]->dev == dev->dev))
            return 33;
        }
        return 0;
    } else {
        /* tricky stuff that we need to get from ACPI ourself */
        return 0;
    }
}
#endif

int get_pci_extended_info(pci_node *dev)
{
    int i;
    if (slotcount > 0) {
        for (i = 0; i < slotcount; i++) {
            if ((slots[i]->dom == dev->domain) &&
                (slots[i]->bus == dev->bus) &&
                (slots[i]->dev == dev->dev))
                if (slots[i]->type >= (u_char)sltPCIE)
                    return 2;
        }
        return 0;
    } else {
        /* tricky stuff that we need to get from ACPI ourself */
        return 0;
    }
}


int get_pci_max_Pci_Speed(pci_node *dev)
{
    int i;
    if (slotcount > 0) {
        for (i = 0; i < slotcount; i++) {
            if ((slots[i]->dom == dev->domain) &&
                (slots[i]->bus == dev->bus) &&
                (slots[i]->dev == dev->dev))
                return 133;
        }
        return 0;
    } else {
        /* tricky stuff that we need to get from ACPI ourself */
        return 0;
    }
}

#define SYSBUSPCISLOTS "/sys/bus/pci/slots/"
int get_pci_slot_info(netsnmp_container* container)
{
    struct dirent **filelist;
    cpqSePciSlotTable_entry *entry;

    int i , rc, bc, dom, bus, device;
    PSMBIOS_SYSTEM_SLOTS slotInfo;

    char buf[80];
    char fname[256];
    int slotfd = -1;
    if (slots)
        return slotcount;

    /* handle simple case */
    if ((slotcount = scandir(SYSBUSPCISLOTS,
                         &filelist, file_select, alphasort)) > 0) {
        slots = malloc(slotcount * sizeof(slot_info *));
        for (i=0;i <slotcount; i++) {
            slots[i] =  malloc(sizeof(slot_info ));
            memset(slots[i],0,sizeof(slot_info ));
            strcpy(fname,SYSBUSPCISLOTS);
            strcat(fname,filelist[i]->d_name);
            strcat(fname,"/address");
            if ((slotfd = open(fname,O_RDONLY)) != -1 ) {
                bc = read(slotfd, buf, 80); 
                buf[bc] = 0;
                DEBUGMSGTL(("pci:arch","bc=%d buf=%s\n", bc,buf));
                if (bc  > 0) 
                    sscanf(buf,"%x:%x:%x", &dom, &bus, &device);
                slots[i]->dom = dom;
                slots[i]->bus = bus;
                slots[i]->dev = device;
                close (slotfd);

                SmbGetRecordByType(SMBIOS_SYS_SLOTS, i , (void *)&slotInfo);
                
                slots[i]->width = slotInfo->bySlotDataBusWidth;
                slots[i]->speed = 1; /* Other for now */
                if (slotInfo->byCurrentUsage == sluAvailable) {
                    entry = cpqSePciSlotTable_createEntry(container, 
                                             (oid)bus, (oid)device);
                    if (NULL != entry) {
                        entry->cpqSePciSlotBusNumberIndex = bus;
                        entry->cpqSePciSlotDeviceNumberIndex = device;
                        entry->cpqSePciPhysSlot = i + 1;
                        entry->cpqSePciSlotWidth = slotInfo->bySlotDataBusWidth -2;
                            
                        if ( slotInfo->bySlotType >= SMBIOS_PCIE_GEN2)
                            entry->cpqSePciSlotType = PCI_SLOT_TYPE_PCIEXPRESS;
                        else
                            entry->cpqSePciSlotType = slotInfo->bySlotType;
                        entry->cpqSePciSlotSpeed = PCI_SLOT_SPEED_OTHER;  /* Other for now */
                        entry->cpqSePciSlotExtendedInfo = 2;
                        entry->cpqSePciMaxSlotSpeed = 133;
                        entry->cpqSePciXMaxSlotSpeed = 0;
                        entry->cpqSePciSlotCurrentMode = PCI_SLOT_TYPE_UNKNOWN;
                        entry->cpqSePciCurrentSlotSpeed = 0;

                        strcpy(entry->cpqSePciSlotBoardName,"(Empty)");                            
                        entry->cpqSePciSlotBoardName_len = 7;

                        memset(&entry->cpqSePciSlotSubSystemID, 0xff, PCISLOT_SUBSYS_SZ);
                        entry->cpqSePciSlotSubSystemID_len = PCISLOT_SUBSYS_SZ;

                        rc = CONTAINER_INSERT(container, entry);
                    }
                }
                
            }
            free(filelist[i]);
        }
        free(filelist);         
    }
    return (slotcount);
}        

vendor_node *vendor_find(vendor_node *vendor, unsigned short vendor_id)
{
    while (vendor != NULL) {
        if (vendor_id == vendor->vendor_id)
            return vendor;
        vendor = vendor->next;
    }
    return NULL;
}

device_node *device_find(device_node *device,
                           unsigned short vendor_id,
                           unsigned short device_id)
{
    while (device != NULL) {
        if ((device_id == device->device_id) &&
            (vendor_id == device->vendor_id))
            return device;
        device = device->next;
    }
    return NULL;
}

subsystem_node *subsystem_find ( subsystem_node *subsystem,
                             unsigned short vendor_id,
                             unsigned short device_id,
                             unsigned short subvendor_id,
                             unsigned short subdevice_id)
{
    while (subsystem != NULL) {
        if ((device_id == subsystem->device_id) &&
            (vendor_id == subsystem->vendor_id) &&
            (subdevice_id == subsystem->subdevice_id) &&
            (subvendor_id == subsystem->subvendor_id))
            return subsystem;
        subsystem = subsystem->next;
    }
    return NULL;
}

unsigned char *get_vname(unsigned short vendor_id)
{
    vendor_node *vendor = NULL;

    if ((vendor = vendor_find(vendor_root, vendor_id)) != NULL)
        return vendor->name;
    else
        return NULL;
}

class_node *class_search(class_node *pciclass, int device_class)
{
    class_node *class_prev;
    class_prev = pciclass;
    while (pciclass != NULL) {
        if (device_class == pciclass->device_class)
            return pciclass;
        else {
            class_prev = pciclass;
            pciclass = pciclass->next;
        }
    }
    if ((pciclass = malloc(sizeof(class_node))) == NULL )
            return NULL;
    else {
        memset(pciclass, 0, sizeof(class_node));
        if (class_prev != NULL)
            class_prev->next = pciclass;
        pciclass->device_class = device_class;
        return pciclass;
    }
}

vendor_node *vendor_search(vendor_node *vendor, unsigned short vendor_id)
{
    vendor_node *vendor_prev;
    vendor_prev = vendor;
    while (vendor != NULL) {
        if (vendor_id == vendor->vendor_id)
            return vendor;
        else {
            vendor_prev = vendor;
            vendor = vendor->next;
        }
    }
    if ((vendor = malloc(sizeof(vendor_node))) == NULL )
            return NULL;
    else {
        memset(vendor, 0, sizeof(vendor_node));
        if (vendor_prev != NULL)
            vendor_prev->next = vendor;
        vendor->vendor_id = vendor_id;
        return vendor;
    }
}

device_node *device_search(device_node *device,
                           unsigned short vendor_id,
                           unsigned short device_id)
{
    device_node *device_prev;
    device_prev = device;
    while (device != NULL) {
        if ((device_id == device->device_id) &&
            (vendor_id == device->vendor_id))
            return device;
        else {
            device_prev = device;
            device = device->next;
        }
    }
    if ((device = malloc(sizeof(device_node))) == NULL )
            return NULL;
    else {
        memset(device, 0, sizeof(device_node));
        if (device_prev != NULL)
            device_prev->next = device;
        device->device_id  = device_id;
        device->vendor_id  = vendor_id;
        return device;
    }
}

subsystem_node *subsystem_search ( subsystem_node *subsystem,
                             unsigned short vendor_id,
                             unsigned short device_id,
                             unsigned short subvendor_id,
                             unsigned short subdevice_id)
{
    subsystem_node *subsystem_prev;
    subsystem_prev = subsystem;
    if ((subdevice_id == 0) && (subvendor_id == 0))
        return NULL;
    while (subsystem != NULL) {
        if ((device_id == subsystem->device_id) &&
            (vendor_id == subsystem->vendor_id) &&
            (subdevice_id == subsystem->subdevice_id) &&
            (subvendor_id == subsystem->subvendor_id))
            return subsystem;
        else {
            subsystem_prev = subsystem;
            subsystem = subsystem->next;
        }
    }
    if ((subsystem = malloc(sizeof(subsystem_node))) == NULL )
            return NULL;
    else {
        memset(subsystem, 0, sizeof(subsystem_node));
        if (subsystem_prev != NULL)
            subsystem_prev->next = subsystem;
        subsystem->device_id    = device_id;
        subsystem->vendor_id    = vendor_id;
        subsystem->subdevice_id = subdevice_id;
        subsystem->subvendor_id = subvendor_id;
        return subsystem;
    }
}

pci_node * pci_node_create(void )
{
    pci_node * node;
    if ((node =  malloc(sizeof(pci_node))) != NULL)
        memset(node, 0, sizeof(pci_node));
    return node;
}

int
pci_dev_filter(const struct dirent * d )
{
    if ((d->d_name[4] == ':') &&
        (d->d_name[7] == ':') &&
        (d->d_name[10] == '.' ))
        return(1);
    else
        return(0);
}

int pci_node_scan(pci_node * node, char * dirname)
{
    struct dirent ** devices;
    struct dirent ** newdevices;
    int i, j;

    int n = 0;
    struct stat st;
    char *newdir;
    int newdir_sz;

    if (stat( SYSBUSPCIDEVICES, &st ) == 0 ) {
        n = scandir( dirname, &devices, pci_dev_filter, alphasort );
        if (n > 0) {
            for (i = 0;i < n; i++) {
                sscanf(devices[i]->d_name, "%4hx:%2hhx:%2hhx.%1hhx",
                       &(node->domain), &(node->bus),
                       &(node->dev), &(node->func));
                newdir_sz = strlen(dirname) + strlen(devices[i]->d_name) + 20;
                newdir = malloc(newdir_sz);
                memset(newdir, 0, newdir_sz);
                strcpy(newdir, dirname);
                strcat(newdir, devices[i]->d_name);
                strcat(newdir, "/");
                newdir_sz = strlen(newdir);

                strcat(newdir, "vendor");
                node->vendor_id = (unsigned short)get_sysfs_shex(newdir);

                newdir[newdir_sz] = (char) 0;
                strcat(newdir, "device");
                node->device_id = (unsigned short)get_sysfs_shex(newdir);

                newdir[newdir_sz] = (char) 0;
                strcat(newdir, "subsystem_vendor");
                node->subvendor_id = (unsigned short)get_sysfs_shex(newdir);

                newdir[newdir_sz] = (char) 0;
                strcat(newdir, "subsystem_device");
                node->subdevice_id = (unsigned short)get_sysfs_shex(newdir);

                newdir[newdir_sz] = (char) 0;
                strcat(newdir, "irq");
                node->irq = (unsigned short)get_sysfs_int(newdir);

                newdir[newdir_sz] = (char) 0;
                strcat(newdir, "class");
                node->device_class = (unsigned int)get_sysfs_shex(newdir);

                newdir[newdir_sz] = (char) 0;
                strcat(newdir,"config");
                if ((node->config = malloc(256)) != NULL) {
                    int config_fd;
                    if ((config_fd = open(newdir, O_RDONLY)) != -1) {
                        if (read(config_fd, node->config, 256) != 256 )
                            free(node->config);
                        close(config_fd);
                        node->caps = pci_get_caps(node->config);
                    } else {
                        free(node->config);
                        node->config = NULL;
                    }
                }
                node->vendor = vendor_search(vendor_root, node->vendor_id);
                if (vendor_root == NULL )
                    vendor_root = node->vendor;
                node->device = device_search(device_root, node->vendor_id,
                                                          node->device_id);
                if (device_root == NULL )
                    device_root = node->device;
                node->subsystem = subsystem_search(subsystem_root,
                                                   node->vendor_id,
                                                   node->device_id,
                                                   node->subvendor_id,
                                                   node->subdevice_id);
                if (subsystem_root == NULL )
                    subsystem_root = node->subsystem;

                node->pciclass = class_search(class_root, node->device_class);
                if (class_root == NULL )
                    class_root = node->pciclass;

                newdir[newdir_sz] = (char) 0;
                j = scandir( newdir, &newdevices, pci_dev_filter, alphasort);
                if (j  > 0 ) {
                    pci_node * new_node;
                    new_node = pci_node_create();
                    if (new_node != NULL) {
                        node->pci_sub = new_node;
                        pci_node_scan(new_node, newdir);
                    }
		    for (;j > 0; j--) 
			free(newdevices[j-1]);
		    free(newdevices);
                }

                free(newdir);
                if (i < (n - 1)) {
                    pci_node * new_node;
                    new_node = pci_node_create();
                    if (new_node != NULL) {
                        node->pci_next = new_node;
                        node = new_node;
                    }
                }
	        free(devices[i]);
            }
            free(devices);		
        }
    }
    return n;
}

void scan_hw_ids()
{
    char * file_name;
    int i = 0;
    FILE *hw_ids;
    struct stat st;
    char buffer[256];
    unsigned short vendor_id = 0;
    unsigned short device_id = 0;
    unsigned short subvendor_id = 0;
    unsigned short subdevice_id = 0;
    int class_id = 0;;
    vendor_node *vendor;
    device_node *device;
    subsystem_node *subsystem;
    class_node *pciclass;
    int device_looking = 0;
    int class_looking = 0;
    int subsystem_looking = 0;

    while ((file_name = pci_ids[i]) != NULL) {
	i++;
    	if (stat(file_name, &st ) == 0 ) 
            break;
    }
    if (file_name == NULL )
        return;

    hw_ids = fopen(file_name,"r");
    while (fgets(buffer, 256, hw_ids) != NULL) {
        if (buffer[0] == 'C') {
            class_id = (int)strtol(&buffer[2], NULL,16) << 16;
            class_looking = 1;
            continue;
        }

        if ((buffer[0] == '\t') &&
            (((buffer[1] >= '0') && (buffer[1] <= '9')) ||
            ((buffer[1] >= 'a') && (buffer[1] <= 'f'))) && class_looking) {
            buffer[3] = 0;
            class_id &= 0xff0000;
            class_id |= ((int)strtol(&buffer[1], NULL,16) << 8);
            for (pciclass = class_root; 
                 pciclass != NULL; pciclass = pciclass->next) {
                if (class_id == (pciclass->device_class & 0xffff00) )
                    if ((pciclass->name = malloc(strlen(&buffer[5]) + 1)) != NULL) {
                        strcpy((char *)pciclass->name, &buffer[5]);
                        pciclass->name[strlen((char *)pciclass->name) - 1] = 0;
                    }
            }
            continue;
        }
        if ((buffer[0] == '\t') && (buffer[1] == '\t') &&
            (((buffer[2] >= '0') && (buffer[2] <= '9')) ||
            ((buffer[2] >= 'a') && (buffer[2] <= 'f'))) && class_looking) {
            buffer[4] = 0;
            class_id &= 0xffff00;
            class_id |= (int)strtol(&buffer[2], NULL,16) ;
            for (pciclass = class_root; 
                 pciclass != NULL; pciclass = pciclass->next) {
                if (class_id == pciclass->device_class  )
                    if ((pciclass->prog_if_name = malloc(strlen(&buffer[6]) + 1)) != NULL) {
                        strcpy((char *)pciclass->prog_if_name, &buffer[6]);
                        pciclass->prog_if_name[strlen((char *)pciclass->prog_if_name) - 1] = 0;
                    }

            }
            continue;
        }

        if (((buffer[0] >= '0') && (buffer[0] <= '9')) ||
            ((buffer[0] >= 'a') && (buffer[0] <= 'f'))) {
/*  Hex value start this line */
            buffer[4] = 0;
            vendor_id = (unsigned short)strtol(buffer, NULL,16);
            if ((vendor = vendor_find(vendor_root, vendor_id)) != NULL) {
                if (vendor->name == NULL)
                    if ((vendor->name = malloc(strlen(&buffer[6]) + 1)) != NULL) {
                        strcpy((char *)vendor->name, &buffer[6]);
                        vendor->name[strlen((char *)vendor->name) - 1] = 0;
                    }
                device_looking = 1;
            } else
                device_looking = 0;
            continue;
        }
        if ((buffer[0] == '\t') &&
            (((buffer[1] >= '0') && (buffer[1] <= '9')) ||
            ((buffer[1] >= 'a') && (buffer[1] <= 'f'))) && device_looking) {
        /* Looking for TAB(HEX)  (NAME) */
            buffer[5] = 0;
            device_id = (unsigned short)strtol(&buffer[1], NULL,16);
            device = device_find(device_root, vendor_id, device_id);
            if (device  != NULL) {
                if (device->name == NULL)
                    if ((device->name = malloc(strlen(&buffer[7]) + 1))
                                                                    != NULL) {
                        strcpy((char *)device->name, &buffer[7]);
                        device->name[strlen((char *)device->name) - 1] = 0;
                    }
                subsystem_looking = 1;
            } else
                subsystem_looking = 0;
            continue;
        }
        if ((buffer[0] == '\t') && (buffer[1] == '\t') &&
            (((buffer[2] >= '0') && (buffer[2] <= '9')) ||
            ((buffer[2] >= 'a') && (buffer[2] <= 'f'))) && subsystem_looking) {
        /* Looking for TAB TAB(HEX) (HEX)  (NAME) */
            buffer[6] = 0;
            buffer[11] = 0;
            subvendor_id = (unsigned short)strtol(&buffer[2], NULL,16);
            subdevice_id = (unsigned short)strtol(&buffer[7], NULL,16);
            subsystem = subsystem_find(subsystem_root,
                            vendor_id, device_id, subvendor_id, subdevice_id);
            if (subsystem  != NULL) {
                if (subsystem->name == NULL)
                    if ((subsystem->name = malloc(strlen(&buffer[12]) + 1))
                                                                    != NULL) {
                        strcpy((char *)subsystem->name, &buffer[12]);
                        subsystem->name[strlen((char *)subsystem->name) - 1] = 0;
                    }
            continue;
            }
        }
    }
    fclose(hw_ids);
}

void netsnmp_arch_pci_init(void) 
{
    int count;

    if (pci_root == NULL){
        pci_root = pci_node_create();
        count = pci_node_scan(pci_root, SYSBUSPCIDEVICES);

        scan_hw_ids();
    }
}

int netsnmp_arch_pcislot_container_load(netsnmp_container* container) 
{
    int count;
    pci_node *dev;

    cpqSePciSlotTable_entry *entry;
    int cnt, rc;

    if (pci_root == NULL){
        pci_root = pci_node_create();
        count = pci_node_scan(pci_root, SYSBUSPCIDEVICES);

        scan_hw_ids();
    }

    get_pci_slot_info(container);

    cnt = 0;
    for (dev=pci_root; dev != NULL; dev=dev->pci_next) {
        if (dev->func == 0) {
            entry = cpqSePciSlotTable_createEntry(container, 
                                                 (oid)dev->bus, (oid)dev->dev);
            if (NULL == entry) 
                continue;   /* error already logged by function */

            DEBUGMSGTL(("pci:arch","pcidev->class=%x bus=%d dev=%d func=%d\n",
                   dev->device_class, dev->bus, dev->dev, dev->func));

            entry->cpqSePciPhysSlot = get_pci_slot(dev);

            if (((dev->device_class & 0xff0000) == 0x20000) && 
                ((dev->subvendor_id == 0x103c) || 
                            (dev->subvendor_id == 0x0e11))) {
                snprintf(entry->cpqSePciSlotSubSystemID, PCISLOT_SUBSYS_SZ + 1, 
                         "%04X%04x", dev->subvendor_id, dev->subdevice_id);
                if (dev->subsystem->vname == NULL) 
                    dev->subsystem->vname = get_vname(dev->subvendor_id);
                
                if (dev->subsystem->name != NULL) 
                    snprintf(entry->cpqSePciSlotBoardName, 
                             PCISLOT_BOARDNAME_SZ, "%s %s", 
                            dev->subsystem->vname, dev->subsystem->name);
                else
                    snprintf(entry->cpqSePciSlotBoardName, 
                             PCISLOT_BOARDNAME_SZ, "%s Device %4x", 
                            dev->subsystem->vname,  dev->subdevice_id);
            } else {
                snprintf(entry->cpqSePciSlotSubSystemID, PCISLOT_SUBSYS_SZ + 1,
                         "%04X%04x", dev->vendor_id,dev->device_id);
                if (dev->device->name != NULL)
                    snprintf(entry->cpqSePciSlotBoardName, 
                                    PCISLOT_BOARDNAME_SZ, "%s %s", 
                                    dev->vendor->name, dev->device->name);
                else
                    snprintf(entry->cpqSePciSlotBoardName, 
                                    PCISLOT_BOARDNAME_SZ, "%s Device %4x", 
                                    dev->vendor->name, dev->device_id);
            }
            
            entry->cpqSePciSlotSubSystemID_len = PCISLOT_SUBSYS_SZ;

            entry->cpqSePciSlotBoardName_len = strlen(entry->cpqSePciSlotBoardName);

            entry->cpqSePciSlotWidth = get_pci_width(dev); /* Unknown for now */
            entry->cpqSePciSlotSpeed = get_pci_speed(dev);
            entry->cpqSePciSlotExtendedInfo = get_pci_extended_info(dev);
            entry->cpqSePciSlotType = get_pci_type(dev); /* Unknown for now */
            entry->cpqSePciSlotCurrentMode = PCI_SLOT_TYPE_UNKNOWN;
            entry->cpqSePciMaxSlotSpeed = get_pci_max_Pci_Speed(dev);
            entry->cpqSePciXMaxSlotSpeed = 0;
            entry->cpqSePciCurrentSlotSpeed = 33;

            rc = CONTAINER_INSERT(container, entry);
        }
    }
    DEBUGMSGTL(("pci:arch"," loaded %ld slot entries\n",
                CONTAINER_SIZE(container)));

    return 0;
}

int netsnmp_arch_pcifunc_container_load(netsnmp_container* container) 
{
    int count;

    pci_node *dev;
    cpqSePciFunctTable_entry *entry;
    int rc;

    if (pci_root == NULL){
        pci_root = pci_node_create();
        count = pci_node_scan(pci_root, SYSBUSPCIDEVICES);

        scan_hw_ids();
    }

    for (dev=pci_root; dev; dev=dev->pci_next) {

        entry = cpqSePciFunctTable_createEntry(container, 
                                              dev->bus, dev->dev, dev->func);
        if (NULL == entry)
            continue;   /* error already logged by function */
        entry->cpqSePciFunctClassCode[0] = dev->device_class & 0xff;
        entry->cpqSePciFunctClassCode[1] = (dev->device_class >> 8) & 0xff;
        entry->cpqSePciFunctClassCode[2] = (dev->device_class >> 16) & 0xff;
        entry->cpqSePciFunctClassCode_len = 3;

	if (dev->pciclass->name != NULL ) {
            strncpy(entry->cpqSePciFunctClassDescription, (char *)dev->pciclass->name,
                    strlen((char *)dev->pciclass->name));
            entry->cpqSePciFunctClassDescription_len = 
                                strlen(entry->cpqSePciFunctClassDescription);
	} else {
            strcpy(entry->cpqSePciFunctClassDescription, "");
            entry->cpqSePciFunctClassDescription_len = 0;
	}

        entry->cpqSePciFunctIntLine =  dev->irq;
        
        entry->cpqSePciFunctDevStatus =  dev->config[6] | (dev->config[7] << 8);

        entry->cpqSePciFunctRevID = dev->config[8];

        if (dev->config[4] & 0x03) 
            entry->cpqSePciFunctDevStatus = PCI_DEVICE_ENABLED;
        else
            entry->cpqSePciFunctDevStatus = PCI_DEVICE_DISABLED;

        if (((dev->device_class & 0xff0000) == 0x20000) &&
            ((dev->subvendor_id == 0x103c) || (dev->subvendor_id == 0x0e11))){
            entry->cpqSePciFunctVendorID = dev->subvendor_id;
            entry->cpqSePciFunctDeviceID = dev->subdevice_id;
        } else {
            entry->cpqSePciFunctVendorID = dev->vendor_id;
            entry->cpqSePciFunctDeviceID = dev->device_id;
        }

        rc = CONTAINER_INSERT(container, entry);
        DEBUGMSGTL(("pci:arch","Inserted pcidev->bus=%x dev=%x func=%x\n",
                   dev->bus, dev->dev, dev->func));
    }
    DEBUGMSGTL(("pci:arch"," loaded %ld entries\n",
                CONTAINER_SIZE(container)));

    return 0;
}


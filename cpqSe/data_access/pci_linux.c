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
#include "common/utils.h"
#include "common/smbios.h"
#include "common/nl_udev.h"
#include "cpqNic/data_access/nic_linux.h"
#include "pci_linux.h"

typedef struct _slot_info {
    int dom;
    int bus;
    int dev;
    unsigned int  type;
    int width;
    int speed;
    int used;
    int id;
} slot_info;

PSMBIOS_SYSTEM_SLOTS   getPCIslot_rec(int bus);
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

int is_HP(unsigned short Vendor)
{
    if ((Vendor == 0x103c) || (Vendor == 0x1590) || (Vendor == 0x0e11))
        return 1;
    else
        return 0;
}    

int get_pci_class(unsigned int classW)
{
    return (classW & 0xff0000) >> 16;
}

void debug_tags(vpd_node *node)
{
    DEBUGMSGTL(("pci:vpd","New Node\n"));
    while (node != NULL) {
        DEBUGMSGTL(("pci:vpd", "[%c%c] = %s\n", 
                    node->tag[0], node->tag[1],node->data));
        node = node->next;
    }
}

unsigned char* pci_get_vpdname(unsigned char *vpd)
{
    size_t length;
    unsigned char *productname = NULL;

    if (vpd[0] == 0x82) {
        length = (unsigned short) vpd[1];
        DEBUGMSGTL(("pci:vpd"," length = %d\n", (int)length));
        if ((productname = malloc(length + 1)) != NULL) 
            memset(productname, 0, length + 1);
            strncpy((char *)productname, (char *)&vpd[3], length);
    }
    DEBUGMSGTL(("pci:vpd","productname = %s\n", productname));
    return productname;
}

void pci_free_vpdtags(vpd_node* vpd_tag)
{
    vpd_node* node;
    vpd_node* last = pci_root->vpd_data;

    for (node = vpd_tag; node->next != NULL; node = node->next) 
        last = node;
    for (node = last; node->prev != NULL; node = node->prev) {
        free(node->data);
        free(node);
    }
    return;
}

vpd_node* pci_get_vpdtags(unsigned char *vpd)
{
    size_t length = 0;;
    vpd_node *vpd_tag = NULL;
    vpd_node *vpd_tag_root = NULL;
    unsigned int rw_offset = 0;

    unsigned char *tag = NULL;
    unsigned char tag_length;
    unsigned short ro_tag_length = 0;;
    unsigned short rw_tag_length = 0;;
    if (vpd[0] == 0x82) {
        length = (unsigned short) vpd[1];
        
//        DEBUGMSGTL(("pci:vpd"," length = %d\n", length));
        if (vpd[length + 3] == 0x90) {
            unsigned char *ro_tag = NULL;
            unsigned char *ro_tag_end = NULL;
            if ((vpd_tag_root = malloc(sizeof(vpd_node))) == NULL) 
                return NULL;
            ro_tag = vpd + length + 3;
            ro_tag_length = ro_tag[1] + (ro_tag[2] << 8);
            ro_tag_end = ro_tag + ro_tag_length;
            tag = ro_tag + 3;
            vpd_tag = vpd_tag_root;
            while (tag < ro_tag_end) {
                vpd_node *vpd_tag_next = NULL;
            DEBUGMSGTL(("pci:vpd","tag = %p, tag_end = %p\n", tag, ro_tag_end));
                tag_length = tag[2];
                DEBUGMSGTL(("pci:vpd","tag_length = %d, tag = %p, tag_end = %p\n", tag_length, tag, ro_tag_end));
                if (tag_length) { 
                    strncpy((char *)vpd_tag->tag, (char *)tag, 2);
                    vpd_tag->data = malloc(tag_length + 1);
                    memset(vpd_tag->data, 0, tag_length + 1);
                    strncpy((char *)vpd_tag->data, (char *)tag + 3, tag_length);
                    vpd_tag->next = malloc(sizeof(vpd_node));
                    memset(vpd_tag->next, 0, sizeof(vpd_node));
                    vpd_tag_next = vpd_tag->next;
                    vpd_tag_next->prev = vpd_tag;
                    vpd_tag = vpd_tag->next;
                } 
                tag += (tag_length + 3);
            }
        }             
        rw_offset = length + ro_tag_length + 6;
        if (vpd[rw_offset] == 0x91) {
            unsigned char *rw_tag = NULL;
            unsigned char *rw_tag_end = NULL;

            rw_tag = vpd + rw_offset;
            rw_tag_length = rw_tag[1] + (rw_tag[2] << 8);
            rw_tag_end = rw_tag + rw_tag_length;
            tag = rw_tag + 3;
            while (tag < rw_tag_end) {
                tag_length = tag[2];
                DEBUGMSGTL(("pci:vpd",
                            "tag_length = %d, tag = %p, tag_end = %p\n", 
                            tag_length, tag, rw_tag_end));
                if (tag_length) { 
                    strncpy((char *)vpd_tag->tag, (char *)tag, 2);
                    vpd_tag->data = malloc(tag_length + 1);
                    memset(vpd_tag->data, 0, tag_length + 1);
                    strncpy((char *)vpd_tag->data, (char *)tag + 3, tag_length);
                    vpd_tag->next = malloc(sizeof(vpd_node));
                    memset(vpd_tag->next, 0, sizeof(vpd_node));
                    vpd_tag = vpd_tag->next;
                } 
                tag += (tag_length + 3);
            }
        }
    }
    return vpd_tag_root;;
}

unsigned int pci_get_revision(unsigned char *config)
{
    return (unsigned int) config[8];
}

unsigned int pci_get_lnkwidth(unsigned char *config)
{
    int cap_index = 0;

    if (config[6] & 0x10) {
        if ((cap_index = config[0x34]) != 0) {
            while (cap_index != 0) {
                unsigned char s;
                DEBUGMSGTL(("pci:cap", "cap_index = %0x, value = %x\n", 
                            cap_index, config[cap_index]));
                if (config[cap_index] == 0x10) {
                    s = ((unsigned short)config[cap_index + 0x12] >> 4) 
                                                            & 0x003f;
                    switch (s) {
                        case 1:
                            return 6;
                        case 2:
                            return 7;
                        case 4:
                            return 8;
                        case 8:
                            return 9;
                        case 12:
                            return 10;
                        case 16:
                            return 11;
                        case 32:
                            return 12;
                        default:
                            return 2;
                    }
                }
                cap_index = config[cap_index + 1];
            }
        }
    }
    return 2;
}

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
                        return (slots[i]->id);
        }
        return 0;
    } else {
        /* tricky stuff that we need to get from ACPI ourself */
        return 0;
    }
}

int get_pci_type(pci_node *dev)
{
    if (dev->caps & 0x8000 )
        return PCI_SLOT_TYPE_PCIEXPRESS;
    else
        return PCI_SLOT_TYPE_UNKNOWN;
}

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

void empty_slot(int bus, int device, int width, cpqSePciSlotTable_entry *entry)
{
    entry->cpqSePciSlotBusNumberIndex = bus;
    entry->cpqSePciSlotDeviceNumberIndex = device;
    entry->cpqSePciSlotWidth = width;
    memset(entry->cpqSePciSlotBoardName,
           0, sizeof(entry->cpqSePciSlotBoardName));
    strncpy(entry->cpqSePciSlotBoardName, "(Empty)",
           sizeof(entry->cpqSePciSlotBoardName - 1));
    entry->cpqSePciSlotBoardName_len = 7;
    entry->cpqSePciSlotExtendedInfo = 0;
    entry->cpqSePciMaxSlotSpeed = 0;
    entry->cpqSePciXMaxSlotSpeed = -1;
    entry->cpqSePciCurrentSlotSpeed = 0;

    memset(&entry->cpqSePciSlotSubSystemID,
           0xff,
           PCISLOT_SUBSYS_SZ);

    entry->cpqSePciSlotCurrentMode = PCI_SLOT_TYPE_UNKNOWN;

    entry->cpqSePciSlotSubSystemID_len = PCISLOT_SUBSYS_SZ;
}

#define SYSBUSPCISLOTS "/sys/bus/pci/slots/"
int get_pci_slot_info(netsnmp_container* container)
{
    struct dirent **filelist;
    cpqSePciSlotTable_entry *entry;

    int i, dom = 0, bus = 0, device = 0;
    PSMBIOS_SYSTEM_SLOTS slotInfo;

    char *buf;
    char fname[256];
    if (slots)
        return slotcount;

    /* handle simple case */
    if ((slotcount = scandir(SYSBUSPCISLOTS,
                    &filelist, file_select, alphasort)) > 0) {
        slots = malloc(slotcount * sizeof(slot_info *));
        for (i = 0; i < slotcount; i++) {
            int slotid = 0;
            if (filelist[i]->d_name[0] == 0x30) {
                /* Slots starting with 0 are for internal devices typically 
                   NVME SFF drives in the enclosure */
                slotid = 0x101;
                if (filelist[i]->d_name[1] == '-') 
                    slotid += (filelist[i]->d_name[2] - 0x30);
            } else 
            slotid = atoi(filelist[i]->d_name);
            slots[i] =  malloc(sizeof(slot_info ));
            memset(slots[i], 0, sizeof(slot_info ));
            strncpy(fname, SYSBUSPCISLOTS, 255);
            strncat(fname, filelist[i]->d_name, 255 - strlen(fname));
            strncat(fname, "/address", 255 - strlen(fname));
            if ((buf = get_sysfs_str(fname)) != NULL) {
                    sscanf(buf,"%4x:%2x:%2x", &dom, &bus, &device);
                slots[i]->dom = dom;
                slots[i]->bus = bus;
                slots[i]->dev = device;
                slots[i]->id  = slotid;
                slots[i]->width = 2;
                free(buf);

                        entry = cpqSePciSlotTable_createEntry(container, 
                                (oid)bus, (oid)device);
                if ((slotInfo = getPCIslot_rec(bus)) != NULL) {
                    slots[i]->width = slotInfo->bySlotDataBusWidth - 2;
                    if (slotInfo->byCurrentUsage == sluAvailable) {
                        if (entry != NULL) {
                            entry->cpqSePciPhysSlot = slotInfo->wSlotID;

                            if ( slotInfo->bySlotType >= SMBIOS_PCIE_GEN2)
                                entry->cpqSePciSlotType = 
                                                    PCI_SLOT_TYPE_PCIEXPRESS;
                            else
                                entry->cpqSePciSlotType = slotInfo->bySlotType;
    
                        }
                    }
                } else {   /* no SMBIOS  Slot Info check for NVME Slots */
                    strncpy(fname, SYSBUSPCISLOTS, 255);
                    strncat(fname, filelist[i]->d_name, 255 - strlen(fname));
                    strncat(fname, "/power", 255 - strlen(fname));
                    if (get_sysfs_int(fname) == 0) {
                        if (NULL != entry) {
                            entry->cpqSePciSlotType = PCI_SLOT_TYPE_PCIEXPRESS;
                            entry->cpqSePciPhysSlot = slotid;
                        }
                }
            }

                if (NULL != entry) 
                    empty_slot(bus, device, slots[i]->width, entry);
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

void vpd_node_destroy(vpd_node * node )
{
    vpd_node * temp = node;
    vpd_node * next = node->next;
    while (next != NULL) {
        next = temp->next;
        if (temp->data != NULL)
            free(temp->data);
        free (temp);
        temp = next;
    }
    if (temp->data != NULL)
        free(temp->data);
    free (temp);
}

void pci_node_destroy(pci_node * node )
{
    pci_node * temp;
    if (node->pci_prev) {
        temp = node->pci_prev;
        temp->pci_next = node->pci_next;
    }
    if (node->pci_next) {
        temp = node->pci_next;
        temp->pci_prev = node->pci_prev;
    }
    if (node->pci_sub)  {
        temp = node->pci_sub;
        temp->pci_top = node->pci_top;
    }
    if (node->pci_top) {
        temp = node->pci_top;
        temp->pci_sub = node->pci_sub;
    }
    if (node->config)
        free(node->config);
    if (node->vpd)
        free(node->vpd);
    if (node->vpd_productname)
        free(node->vpd_productname);
    if (node->vpd_data)
        vpd_node_destroy(node->vpd_data);
    free(node);
}

pci_node * pci_node_create(void )
{
    pci_node * node;
    if ((node =  malloc(sizeof(pci_node))) != NULL)
        memset(node, 0, sizeof(pci_node));
    return node;
}

int pci_dev_filter(const struct dirent * d )
{
    if (strstr(d->d_name, ":pcie") != NULL)
        return 0;
    if ((d->d_name[4] == ':') &&
            (d->d_name[7] == ':') &&
            (d->d_name[10] == '.' ))
        return(1);
    else
        return(0);
}

pci_node*  pci_node_find(pci_node* node, int bus, int dev, int fun)
{
    pci_node* pci_sub;
    for (; node != NULL; node = node->pci_next) {
        if (node->pci_sub != NULL) 
            if ((pci_sub = pci_node_find(node->pci_sub, bus, dev, fun)) != NULL)  
                return pci_sub;
        if ((bus == node->bus) && (dev == node->dev) && (fun == node->func))
            return node;
    }
    return NULL;
}

void bufprep (char *buffer, char* dir) {
    memset(buffer, 0, 256);
    strncpy(buffer, dir,255);
}

void fill_pci_node(pci_node *node, char *dir)
{
    char buffer[256];

    int remain = 256 - strlen(dir);
    int config_fd = -1;

    bufprep(buffer, dir);
    DEBUGMSGTL(("pci:arch", "PCI Device = %s\n", buffer));
    strncat(buffer, "vendor", remain);
    node->vendor_id = (unsigned short)get_sysfs_shex(buffer);

    bufprep(buffer, dir);
    strncat(buffer, "device", remain);
    node->device_id = (unsigned short)get_sysfs_shex(buffer);

    bufprep(buffer, dir);
    strncat(buffer, "subsystem_vendor", remain);
    node->subvendor_id = (unsigned short)get_sysfs_shex(buffer);

    bufprep(buffer, dir);
    strncat(buffer, "subsystem_device", remain);
    node->subdevice_id = (unsigned short)get_sysfs_shex(buffer);

    bufprep(buffer, dir);
    strncat(buffer, "irq", remain);
    node->irq = (unsigned short)get_sysfs_int(buffer);

    bufprep(buffer, dir);
    strncat(buffer, "class", remain);
    node->device_class = (unsigned int)get_sysfs_ihex(buffer);

    bufprep(buffer, dir);
    strncat(buffer,"config", remain);
    if ((config_fd = open(buffer, O_RDONLY)) != -1) {
                if ((node->config = malloc(256)) != NULL) {
                        if (read(config_fd, node->config, 256) == 256 ) {
                            node->caps = pci_get_caps(node->config);
                            node ->revision = pci_get_revision(node->config);
                node->linkwidth = pci_get_lnkwidth(node->config);
                    } else {
                        free(node->config);
                        node->config = NULL;
                    }
                }
        close(config_fd);
    }

    bufprep(buffer, dir);
    strncat(buffer,"vpd", remain);
                if ((node->vpd = malloc(4096)) != NULL) {
                    int vpd_fd;
        if ((vpd_fd = open(buffer, O_RDONLY)) != -1) {
            DEBUGMSGTL(("pci:vpd","buffer = %s\n", buffer));
                        if (read(vpd_fd, node->vpd, 4096) > 0  ) {
                            node->vpd_productname = pci_get_vpdname(node->vpd);
                            node->vpd_data = pci_get_vpdtags(node->vpd);
                            debug_tags(node->vpd_data);
                        }
                        free(node->vpd);
                        close(vpd_fd);
                    } else {
                        free(node->vpd);
                    }
                    node->vpd = NULL;
                }

                node->vendor = vendor_search(vendor_root, node->vendor_id);
                if (vendor_root == NULL )
                    vendor_root = node->vendor;
    node->device = device_search(device_root, node->vendor_id, node->device_id);
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
}

int pci_node_scan(pci_node * node, char * dirname)
{
    struct dirent ** devices;
    struct dirent ** newdevices;
    int i, j;

    int n = 0;
    char *newdir;
    int newdir_sz; 
    n = scandir( dirname, &devices, pci_dev_filter, alphasort );
    for (i = 0;i < n; i++) {
        ssize_t remain;

        memset(node->sysdev, 0, 16);
        strncpy((char *)node->sysdev, devices[i]->d_name, 15);

        sscanf(devices[i]->d_name, "%4hx:%2hhx:%2hhx.%1hhx",
                &(node->domain), &(node->bus),
                &(node->dev), &(node->func));

        newdir_sz = strlen(dirname) + strlen(devices[i]->d_name) + 20;
        newdir = malloc(newdir_sz);
        memset(newdir, 0, newdir_sz);
        remain = newdir_sz - 1;
        strncpy(newdir, dirname, remain);
        remain -= strlen(dirname);
        strncat(newdir, "/", --remain);
        strncat(newdir, devices[i]->d_name, remain);
        remain -= strlen(devices[i]->d_name);
        strncat(newdir, "/", --remain);
        newdir_sz = strlen(newdir);
                newdir[newdir_sz] = (char) 0;

        fill_pci_node(node, newdir); 

                j = scandir( newdir, &newdevices, pci_dev_filter, alphasort);
                if (j  > 0 ) {
            pci_node * new_node = pci_node_create();
                    if (new_node != NULL) {
                        node->pci_sub = new_node;
                new_node->pci_top = node;
                        pci_node_scan(new_node, newdir);
                    }
                    for (;j > 0; j--) 
                        free(newdevices[j-1]);
                    free(newdevices);
                }

                free(newdir);
                if (i < (n - 1)) {
            pci_node * new_node = pci_node_create();
                    if (new_node != NULL) {
                        node->pci_next = new_node;
                new_node->pci_prev = node;
                        node = new_node;
                    }
                }
                free(devices[i]);
            }
            free(devices);		
    return n;
}

void scan_hw_ids()
{
    int i = 0;
    int flen = 0;
    FILE *hw_ids = NULL;
    char buffer[256];
    unsigned short vendor_id = 0;
    unsigned short device_id = 0;
    unsigned short subvendor_id = 0;
    unsigned short subdevice_id = 0;
    int class_id = 0;
    unsigned char class_id_str[80];
    vendor_node *vendor;
    device_node *device;
    subsystem_node *subsystem;
    class_node *pciclass;
    int device_looking = 0;
    int class_looking = 0;
    int subsystem_looking = 0;

    while (pci_ids[i] != NULL) {
        hw_ids = fopen(pci_ids[i++], "r");
        if (hw_ids != NULL)
            break;
    }
    if (hw_ids == NULL)
        return;

    while (fgets(buffer, 256, hw_ids) != NULL) {
        flen = strlen(buffer);
        if (buffer[flen - 1] == '\n')
            buffer[flen - 1] = 0; /* strip off trailing LF */

        if (buffer[0] == 'C') {
            class_id = (int)strtol(&buffer[2], NULL, 16) << 16;
            class_looking = 1;
            memset(&class_id_str[0], 0, 80);
            strncpy((char *)&class_id_str[0], &buffer[6], 79);
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
                if (class_id == (pciclass->device_class & 0xffff00) ) {
                    if ((pciclass->super = 
                            malloc(strlen((char *)&class_id_str[0]) + 1))
                         != NULL) {
                        memset(pciclass->super, 0, strlen((char *)&class_id_str[0]) + 1);
                        strncpy((char *)pciclass->super, 
                                (char *)&class_id_str[0], 
                                strlen((char *)&class_id_str[0]));
                    }
                    if ((pciclass->name = malloc(strlen(&buffer[5]) + 1)) 
                         != NULL) 
                        memset(pciclass->name, 0, strlen(&buffer[5]) + 1);
                        strncpy((char *)pciclass->name, 
                                &buffer[5], 
                                strlen(&buffer[5]));
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
                    if ((pciclass->prog_if_name = 
                            malloc(strlen(&buffer[6]) + 1)) != NULL) {
                        memset(pciclass->prog_if_name, 0, strlen(&buffer[6]) + 1);
                        strncpy((char *)pciclass->prog_if_name, 
                                &buffer[6], 
                                strlen(&buffer[6]));
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
                    if ((vendor->name = malloc(strlen(&buffer[6]) + 1)) 
                        != NULL) {
                        memset(vendor->name, 0, strlen(&buffer[6]) + 1);
                        strncpy((char *)vendor->name, 
                                &buffer[6],
                                strlen(&buffer[6]));
                        vendor->name[strlen((char *)vendor->name)] = 0;
                    }
                device_looking = 1;
            } else
                device_looking = 0;
            continue;
        }
        if ((buffer[0] == '\t') && device_looking &&
                (((buffer[1] >= '0') && (buffer[1] <= '9')) ||
                 ((buffer[1] >= 'a') && (buffer[1] <= 'f')))) {
            /* Looking for TAB(HEX)  (NAME) */
            buffer[5] = 0;
            device_id = (unsigned short)strtol(&buffer[1], NULL,16);
            device = device_find(device_root, vendor_id, device_id);
            if (device  != NULL) {
                if (device->name == NULL)
                    if ((device->name = malloc(strlen(&buffer[7]) + 1))
                            != NULL) {
                        memset(device->name, 0, strlen(&buffer[7]) + 1);
                        strncpy((char *)device->name, 
                                &buffer[7],
                                strlen(&buffer[7]));
                        device->name[strlen((char *)device->name)] = 0;
                    }
                subsystem_looking = 1;
            } else
                subsystem_looking = 0;
            continue;
        }
        if ((buffer[0] == '\t') && (buffer[1] == '\t') && subsystem_looking &&
                (((buffer[2] >= '0') && (buffer[2] <= '9')) ||
                 ((buffer[2] >= 'a') && (buffer[2] <= 'f')))) {
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
                        memset(subsystem->name, 0, strlen(&buffer[12]) + 1);
                        strncpy((char *)subsystem->name, 
                                &buffer[12],
                                strlen(&buffer[12]));
                        subsystem->name[strlen((char *)subsystem->name)] = 0;
                    }
                continue;
            }
        }
    }
    fclose(hw_ids);
}

void netsnmp_arch_pci_init(void) 
{

    if (pci_root == NULL){
        pci_root = pci_node_create();
        pci_node_scan(pci_root, SYSBUSPCIDEVICES);

        scan_hw_ids();
    }
}

void add_cpqSePciFuncTable_entry(cpqSePciFunctTable_entry *entry, 
                                 netsnmp_container* container)
{
    pci_node *node;

    DEBUGMSGTL(("cpqpci:container:load", "Re-Use old entry\n"));
    node = pci_node_find(pci_root, 
                         entry->cpqSePciFunctBusNumberIndex, 
                         entry->cpqSePciFunctDeviceNumberIndex, 
                         entry->cpqSePciFunctIndex);
    memset(entry->cpqSePciFunctClassDescription, 
           0, sizeof(entry->cpqSePciFunctClassDescription));
    entry->cpqSePciFunctClassDescription_len = 0;
    entry->cpqSePciFunctClassCode[0] = node->device_class & 0xff;
    entry->cpqSePciFunctClassCode[1] = (node->device_class >> 8) & 0xff;
    entry->cpqSePciFunctClassCode[2] = (node->device_class >> 16) & 0xff;
    entry->cpqSePciFunctClassCode_len = 3;

    if (node->pciclass->name != NULL ) {
        if (node->pciclass->prog_if_name != NULL ) 
            entry->cpqSePciFunctClassDescription_len = 
                snprintf(entry->cpqSePciFunctClassDescription, 80, 
                        "%s - %s (%s)", 
                        (char *)node->pciclass->super,
                        (char *)node->pciclass->name,
                        (char *)node->pciclass->prog_if_name);
        else
            entry->cpqSePciFunctClassDescription_len = 
                snprintf(entry->cpqSePciFunctClassDescription, 80, 
                        "%s - %s", 
                        (char *)node->pciclass->super,
                        (char *)node->pciclass->name);
    }

    entry->cpqSePciFunctIntLine =  node->irq;
    entry->cpqSePciFunctDevStatus =  node->config[6] | (node->config[7] << 8);
    entry->cpqSePciFunctRevID = node->config[8];

    if (node->config[4] & 0x03) 
        entry->cpqSePciFunctDevStatus = PCI_DEVICE_ENABLED;
    else
        entry->cpqSePciFunctDevStatus = PCI_DEVICE_DISABLED;

    if ((get_pci_class(node->device_class) == 2) && 
            is_HP(node->subvendor_id)) {
        entry->cpqSePciFunctVendorID = node->subvendor_id;
        entry->cpqSePciFunctDeviceID = node->subdevice_id;
    } else {
        entry->cpqSePciFunctVendorID = node->vendor_id;
        entry->cpqSePciFunctDeviceID = node->device_id;
    }
}

void update_cpqSePciSlotTable_entry(pci_node *node, 
                                    cpqSePciSlotTable_entry *slot) 
{
    nic_hw_db *nic_db;

    char *devname = "";
    slot->cpqSePciPhysSlot = get_pci_slot(node);
    slot->cpqSePciSlotWidth = node->linkwidth; /* Unknown for now */
    slot->cpqSePciSlotExtendedInfo = get_pci_extended_info(node);
    slot->cpqSePciSlotType = get_pci_type(node); /* Unknown for now */
    
    snprintf(slot->cpqSePciSlotSubSystemID, PCISLOT_SUBSYS_SZ + 1,
                    "%04X%04x", node->vendor_id, node->device_id);
    slot->cpqSePciSlotSubSystemID_len = PCISLOT_SUBSYS_SZ;

            /* use VPD product name if it starts with HP */
    if ((node->vpd_productname) && 
        !strncmp((char *)node->vpd_productname, "HP", 2))
        slot->cpqSePciSlotBoardName_len = 
            snprintf(slot->cpqSePciSlotBoardName, 
                    PCISLOT_BOARDNAME_SZ, "%s", node->vpd_productname);
            else {
        vpd_node* vnode = node->vpd_data;
        while (vnode != NULL){
            if ((vnode->data != NULL) && 
                !strncmp((char *)vnode->data, "HP", 2))
                slot->cpqSePciSlotBoardName_len = 
                    snprintf(slot->cpqSePciSlotBoardName, 
                            PCISLOT_BOARDNAME_SZ, "%s", vnode->data);
            vnode = vnode->next;
                }
            }
    /*         */
    if (!slot->cpqSePciSlotBoardName_len || 
            !strcmp(slot->cpqSePciSlotBoardName, "(Empty)")) {
        if (node->device->name != NULL)
                    /* if there is a good device name then use the 
                node Vendor and device name */
            slot->cpqSePciSlotBoardName_len = 
                snprintf(slot->cpqSePciSlotBoardName, 
                        PCISLOT_BOARDNAME_SZ, "%s %s", 
                        node->vendor->name, node->device->name);
            else
                    /* just print out the Vendor and the 
                       hexadeciamla device name */
            slot->cpqSePciSlotBoardName_len = 
                snprintf(slot->cpqSePciSlotBoardName, 
                        PCISLOT_BOARDNAME_SZ, "%s Device %4x", 
                        node->vendor->name, node->device_id);

                /* go back and check HP/Compaq storage controllers and use 
                   Subvendor and subdevice ID's if we have them */
        if (is_HP(node->subvendor_id) &&  
            ((get_pci_class(node->device_class) == 1) || 
             (get_pci_class(node->device_class) == 2))) {  

            if (node->subsystem->vname == NULL) 
                node->subsystem->vname = get_vname(node->subvendor_id);

            /* Use different format for Storage devices */
            if (get_pci_class(node->device_class) == 1)  
                devname = node->device->name;
            DEBUGMSGTL(("pci:arch:hp",
                                "dev->subsys->vname=%s"
                        " dev->subsys->name=%s" 
                        " dev->device->name=%s\n", 
                        node->subsystem->vname, 
                        node->subsystem->name, 
                        devname));
            if (node->subsystem->name != NULL) {
                memset(slot->cpqSePciSlotBoardName, 0, PCISLOT_BOARDNAME_SZ);
                slot->cpqSePciSlotBoardName_len = 
                    snprintf(slot->cpqSePciSlotBoardName, 
                            PCISLOT_BOARDNAME_SZ, "%s%s %s", 
                            node->subsystem->vname, 
                            node->subsystem->name, devname);
                snprintf(slot->cpqSePciSlotSubSystemID, 
                                PCISLOT_SUBSYS_SZ + 1, 
                                "%04X%04x", 
                        node->subvendor_id, node->subdevice_id);
            } else {
               nic_db = get_nic_hw_info(node->vendor_id, node->device_id,
                    node->subvendor_id, node->subdevice_id);
                if (nic_db != NULL) {
                    memset(slot->cpqSePciSlotBoardName, 0, PCISLOT_BOARDNAME_SZ);
                    slot->cpqSePciSlotBoardName_len = 
                        snprintf(slot->cpqSePciSlotBoardName, 
                            PCISLOT_BOARDNAME_SZ, "%s", 
                            nic_db->pname);
                    snprintf(slot->cpqSePciSlotSubSystemID, 
                        PCISLOT_SUBSYS_SZ + 1, 
                        "%04X%04x", 
                        node->subvendor_id, node->subdevice_id);
            }
                    }
                }
            }
}

void add_cpqSePciSlotTable_entry(oid bus, oid dev, netsnmp_container* container)
{
    netsnmp_index tmp;
    oid oid_index[2];

    cpqSePciSlotTable_entry *entry;
    pci_node *node;

    oid_index[0] = bus;
    oid_index[1] = dev;
    tmp.len = 2;
    tmp.oids = &oid_index[0];

    entry = CONTAINER_FIND(container, &tmp);
    DEBUGMSGTL(("cpqpci:container:load", "Old pci=%p\n", entry));
    if (entry != NULL ) {
        DEBUGMSGTL(("cpqpci:container:load", "Re-Use old entry\n"));

        memset(entry->cpqSePciSlotBoardName, 0, 
                sizeof(entry->cpqSePciSlotBoardName));
        entry->cpqSePciSlotBoardName_len = 0;
            entry->cpqSePciSlotCurrentMode = PCI_SLOT_TYPE_UNKNOWN;
            entry->cpqSePciMaxSlotSpeed = 0;
            entry->cpqSePciXMaxSlotSpeed = -1;
            entry->cpqSePciCurrentSlotSpeed = 0;

        node = pci_node_find(pci_root, bus, dev, 0);
        if (node != NULL) 
            update_cpqSePciSlotTable_entry(node, entry);
        }
}

static void cpqpci_remove_function(char *devpath, char *devname, char *devtype, void *data)
{
    int  Bus, Dev, Fun;
    char *path;
    netsnmp_index tmp;
    oid oid_index[3];
    cpqSePciFunctTable_entry *func;

    DEBUGMSGTL(("pci:arch:hp", "cpqpci_remove_function devpath=%s, devname = %s, devtype = %s\n", devpath, devname, devtype));
    path = strrchr(devpath, '/');
    if (path == NULL)
        return;

    if (sscanf(path, "/0000:%2x:%2x.%d", &Bus, &Dev, &Fun) != 3) 
        return;
    DEBUGMSGTL(("pci:arch:hp", "cpqpci_remove_function path=%s, bus = %d, dev = %d fun = %d\n", path, Bus, Dev, Fun));
    oid_index[0] = Bus;
    oid_index[1] = Dev;
    oid_index[2] = Fun;
    tmp.len = 3;

    tmp.oids = &oid_index[0];

    func = CONTAINER_FIND((netsnmp_container*)data, &tmp);
    if (func != NULL) {
        CONTAINER_REMOVE(data, func);
        SNMP_FREE(func);
    }
}

static void cpqpci_add_function(char *devpath, char *devname, char *devtype, void *data)
{
    int  Bus, Dev, Fun;
    cpqSePciFunctTable_entry *entry;
    char *path;

    DEBUGMSGTL(("pci:arch:hp", "cpqpci_add_function devpath=%s, devname = %s, devtype = %s\n", devpath, devname, devtype));
    path = strrchr(devpath, '/');
    if (path == NULL)
        return;

    if (sscanf(path, "/0000:%2x:%2x.%d", &Bus, &Dev, &Fun) != 3) 
        return;

    DEBUGMSGTL(("pci:arch:hp", "cpqpci_add_function path=%s, bus = %d, dev = %d fun = %d\n", path, Bus, Dev, Fun));
    entry = cpqSePciFunctTable_createEntry(data, Bus, Dev, Fun);
    if (NULL == entry)
        return;   /* error already logged by function */

    add_cpqSePciFuncTable_entry(entry, data);
    DEBUGMSGTL(("pci:arch","Inserted bus=%x dev=%x func=%x\n",
                Bus, Dev, Fun));

}

static void cpqpci_remove_device(char *devpath, char *devname, char *devtype, void *data)
{
    int  Bus, Dev;
    char *path;
    netsnmp_index tmp;
    oid oid_index[2];
    cpqSePciSlotTable_entry *slot;
    pci_node *node;

    path = strrchr(devpath, '/');
    if (path == NULL)
        return;

    if (sscanf(path, "/0000:%2x:%2x", &Bus, &Dev) != 2) 
        return;
    if ((node = pci_node_find(pci_root, Bus, Dev, 0)) == NULL)
        return;
    else {
        oid_index[0] = Bus;
        oid_index[1] = Dev;
        tmp.len = 2;

        tmp.oids = &oid_index[0];
    
        slot = CONTAINER_FIND((netsnmp_container*)data, &tmp);
        if (slot != NULL) {
            empty_slot(Bus, Dev, node->linkwidth, slot);
        }
        pci_node_destroy(node);
    }
}

static void cpqpci_add_device(char *devpath, char *devname, char *devtype, void *data)
{
    int  Bus, Dev, PBus, PDev;
    char *path, *parent;
    netsnmp_index tmp;
    oid oid_index[2];
    cpqSePciSlotTable_entry *slot;
    pci_node *node, *new_node;
    char *new_dir;
    char *buf;

    if ((buf = strdup(devpath)) == NULL)
        return;

    path = strrchr(buf, '/');
    if (path == NULL) {
        free(buf);
        return;
    }

    if (sscanf(path, "/0000:%2x:%2x", &Bus, &Dev) != 2) {
        free(buf);
        return;
    }

    *path = 0;
    parent = strrchr(buf, '/');
    if (parent == NULL) {
        free(buf);
        return;
    }
        
    if (sscanf(parent, "/0000:%2x:%2x", &PBus, &PDev) != 2) {
        free(buf);
        return;
    }

    if ((node = pci_node_find(pci_root, PBus, PDev, 0)) == NULL) {
        free(buf);
        return;
    }
    if ((new_dir = malloc(strlen(buf) + 5)) == NULL) {
        free(buf);
        return;
    }
    if ((new_node = pci_node_create()) == NULL) {
        free(buf);
        free(new_dir);
        return;
    }
    
    memset(new_dir, 0, strlen(buf + 5));
    strncpy(new_dir, "/sys", 5);
    strncat(new_dir, buf, strlen(buf));
    node->pci_sub = new_node;
    new_node->pci_top = node;
    usleep(2000);
    pci_node_scan(new_node, new_dir);

    oid_index[0] = Bus;
    oid_index[1] = Dev;
    tmp.len = 2;

    tmp.oids = &oid_index[0];

    slot = CONTAINER_FIND((netsnmp_container*)data, &tmp);
    
    DEBUGMSGTL(("cpqpci:container:load", "Old pci=%p\n", slot));
    if (slot != NULL ) {
        DEBUGMSGTL(("cpqpci:container:load", "Re-Use old entry\n"));
        update_cpqSePciSlotTable_entry(new_node, slot);
    }
    free(buf);
    free(new_dir);
}

int netsnmp_arch_pcislot_container_load(netsnmp_container* container) 
{
    pci_node *dev;

    cpqSePciSlotTable_entry *entry;

    if (pci_root == NULL){
        pci_root = pci_node_create();
        pci_node_scan(pci_root, SYSBUSPCIDEVICES);

        scan_hw_ids();
    }

    get_pci_slot_info(container);

    for (dev=pci_root; dev != NULL; dev=dev->pci_next) {
        if (dev->func == 0) {
            entry = cpqSePciSlotTable_createEntry(container, 
                    (oid)dev->bus, (oid)dev->dev);
        if (NULL == entry)
            continue;   /* error already logged by function */

            DEBUGMSGTL(("pci:arch","pcidev->class=%x bus=%d dev=%d func=%d\n",
                        dev->device_class, dev->bus, dev->dev, dev->func));

            add_cpqSePciSlotTable_entry(dev->bus, dev->dev, container);
        }
    }
    DEBUGMSGTL(("pci:arch"," loaded %ld slot entries\n",
                CONTAINER_SIZE(container)));

    udev_register("pci", "add", NULL, cpqpci_add_device, container);
    udev_register("pci", "remove", NULL, cpqpci_remove_device, container);

    return 0;
}

int netsnmp_arch_pcifunc_container_load(netsnmp_container* container) 
{
    pci_node *dev;
    cpqSePciFunctTable_entry *entry;

    if (pci_root == NULL){
        pci_root = pci_node_create();
        pci_node_scan(pci_root, SYSBUSPCIDEVICES);

        scan_hw_ids();
        }

    for (dev=pci_root; dev; dev=dev->pci_next) {

        entry = cpqSePciFunctTable_createEntry(container, 
                dev->bus, dev->dev, dev->func);
        if (NULL == entry)
            continue;   /* error already logged by function */
        add_cpqSePciFuncTable_entry(entry, container);

        DEBUGMSGTL(("pci:arch","Inserted pcidev->bus=%x dev=%x func=%x\n",
                    dev->bus, dev->dev, dev->func));
    }

    DEBUGMSGTL(("pci:arch"," loaded %ld function entries\n",
                CONTAINER_SIZE(container)));

    udev_register("pci", "add", NULL, cpqpci_add_function, container);
    udev_register("pci", "remove", NULL, cpqpci_remove_function, container);

    return 0;
}


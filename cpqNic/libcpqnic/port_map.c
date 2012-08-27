// (c) Copyright 2006 Hewlett-Packard Development Company, L.P.
// All rights reserved
//
// $Id: port_map.c,v 1.3 2007/10/16 11:41:30 mjs Exp $
//
////////////////////////////////////////////////////////////////////////////////
/// \file port_map.c
///
/// This file contains the code to read SMBIOS table for nic port mapping
/// info.
///
////////////////////////////////////////////////////////////////////////////////

#include <pci/pci.h>
#include "common_utilities.h"

/// DEFINE NON-MACRO

#define TEMP_LEN	1024

#define NUM_LS_PCI_ITEMS	3	/* bus/device/function are 3 itmes to
                                           be read from lspci command. */
#define NUM_PORT_MAP_ITEMS	4	/* there are 4 items to be read
                                           from hpasmcli command. */
#define MAX_PHYSICAL_SLOTS 	128 	/* max number of physical slots */

#define ETHERNET_CONTROLLER	0x020000

/// number of /sys/bus/pci/devices files to read
#define SYS_PCI_FILES 4
/// Globale Variables
static const char *hplog_utils = "/opt/compaq/utils/hplog";
static const char *hplog_sbin = "/sbin/hplog";
static const char *hpasmcli_utils = "/opt/compaq/utils/hpasmcli";
static const char *hpasmcli_sbin = "/sbin/hpasmcli";

static const char *pci_devices_dir = "/sys/bus/pci/devices";

/* Network Adapter list. The list is sorted in ascending B/D/F order. */
static nct_network_adapter_p net_adapter_list_head = NULL;

struct pci_access *pacc;

extern int file_select(const struct dirent *);

////////////////////////////////////////////////////////////////////////////////
/// \brief Get the PCI ID of the NIC give the PCI bus/device/function
///
/// Get the PCI ID for the device, given the the PCI bus/device/function,
/// from the /sys/bus/pci/devices/*/* files.
/// The PCI bus info string looks like "0000:02:0e.0"
///
/// unit tested by main
///
/// \author Tony Cureington
/// \param[in] pbus_info ptr to bus information
/// \param[out] ppci_ven
/// \param[out] ppci_dev
/// \param[out] ppci_sub_ven
/// \param[out] ppci_sub_dev
/// \retval int32_t 0 on success, non-zero on failure
///
/// \note The pbus_info must be filled in prior to the call to this
///       function, pbus_info can be filled in by the get_ethtool_info() call.
///       If pbus_info is a structure memebr, it can be initialised to
///       STRUCT_MEMBER_NOT_CONFIGURED or STRUCT_MEMBER_UNKNOWN if the
///       value is unknown.
///
/// \see get_ethtool_info()
///
////////////////////////////////////////////////////////////////////////////////
int32_t get_pci_info(char *pbus_info,
        ushort *ppci_ven,
        ushort *ppci_dev,
        ushort *ppci_sub_ven,
        ushort *ppci_sub_dev)
{
    uint domain,bus,device,function;
    struct pci_dev *dev;

    if (((char*)0 == pbus_info) ||
            (strlen(pbus_info) ==0)) {
        return(1);
    }

    sscanf(pbus_info, "%x:%x:%x.%x",
                    &domain, &bus, &device, &function);

    if (pacc == NULL){
        pacc = pci_alloc();
        pci_init(pacc);    
        pci_scan_bus(pacc); 
    }

    for (dev=pacc->devices; dev; dev=dev->next) {
        pci_fill_info(dev, PCI_FILL_IDENT | PCI_FILL_BASES | PCI_FILL_CLASS);
        if ((dev->domain == domain) && 
            (dev->bus == bus) && 
            (dev->dev == device) && 
            (dev->func == function )) {
            
            *ppci_ven = dev->vendor_id;
            *ppci_dev = dev->device_id;

            *ppci_sub_ven = pci_read_word(dev,PCI_SUBSYSTEM_VENDOR_ID);
            *ppci_sub_dev = pci_read_word(dev,PCI_SUBSYSTEM_ID);

            return(0);
        }
    }
    return (1);
}

/* Adapter is partitioned */
static int32_t is_adapter_partitioned(uint32_t bus, uint32_t device)
{
    int count = 0;
    if (pacc == NULL){
        pacc = pci_alloc();
        pci_init(pacc);    
        pci_scan_bus(pacc); 
    }

    return((count == 8) ? 1:0);
}


static int32_t assign_port_value(uint32_t vendor_id, 
                                 uint32_t device_id,
                                 uint32_t subsystem_device, 
                                 uint32_t subsystem_vendor)
{
    static int count = 0;
    int32_t mlx_port = -1;
    if ((vendor_id == 0x15B3) && 
        (device_id == 0x5A44) && 
        (subsystem_device == 0x3107) &&
        (subsystem_vendor == 0x103C)) {
        mlx_port = count++;
        if ( count == 2) 
            count = 0;
        
    }
    return(mlx_port);
}


static void add_net_adapter(uint32_t bus, uint32_t device, 
                            uint32_t function, int32_t port)
{
    nct_network_adapter_p adapter;
    nct_network_adapter_p prev = NULL;

    adapter = (nct_network_adapter_p) malloc(sizeof(nct_network_adapter_t));
    CHECK_MALLOC(adapter);
    memset(adapter, 0 , sizeof(nct_network_adapter_t));

    /* populate adapter info */

    adapter->slot = UNKNOWN_SLOT;
    adapter->slot_port = UNKNOWN_PORT;
    adapter->bus = bus;
    adapter->device = device;
    adapter->function = function;
    adapter->port = port;
    adapter->nic_port_map = NULL;
    adapter->prev = NULL;
    adapter->next = NULL;
    if(is_adapter_partitioned(bus,device)) 
        adapter->virtual_port = (function + 2)/2;
    else
        adapter->virtual_port = UNKNOWN_PORT;

    if (net_adapter_list_head == NULL) {
        net_adapter_list_head = adapter;	
        adapter->prev = adapter;
        adapter->next = adapter;
    }
    else {
        prev = net_adapter_list_head->prev;

        net_adapter_list_head->prev = adapter;
        adapter->next = net_adapter_list_head;
        adapter->prev = prev;
        prev->next = adapter;
    }
}

static int32_t get_slot_number(uint32_t bus, uint32_t device)
{
    FILE *output_stream = NULL;
    struct stat stat_buf;
    char temp_buf[TEMP_LEN];
    static const char *hplog_cmd = NULL;
    int32_t slot_num;
    uint32_t bus_num, device_num;
    int n;
    char * dummy;

    if (hplog_cmd == NULL) {
        if (stat(hplog_utils, &stat_buf) != -1) {
            hplog_cmd = hplog_utils;
        }
        else if (stat(hplog_sbin, &stat_buf) != -1) {
            hplog_cmd = hplog_sbin;
        }
        else
        {
            DPRINTF("ERROR: could not find hplog utility %s,%d\n",
                    __FILE__, __LINE__);
            return UNKNOWN_SLOT;
        }
    }
    /* Build command - awk command will filter out only following columns
       from hplog Utility:
       BusNumber DeviceNumber SlotNumber */
    sprintf(temp_buf,
            "%s -i 2>/dev/null | awk '{print $1\" \"$2\" \"$6;}'",
            hplog_cmd);
#ifdef NCT_PORT_MAP_DEBUG
    printf ("Executing Command: %s\n", temp_buf);
#endif
    output_stream = popen(temp_buf, "r");
    if (output_stream == NULL) {
        fprintf(stderr,
                "ERROR: could not execute command: %s at %s,%d\n",
                temp_buf, __FILE__,__LINE__);
        return UNKNOWN_SLOT;

    }
    /* read the first blank line */
    dummy = fgets(temp_buf, TEMP_LEN, output_stream);

    /* read past the header  "BusNu...." */
    dummy = fgets(temp_buf, TEMP_LEN, output_stream);

    while (fgets(temp_buf, TEMP_LEN, output_stream)) {
        n = sscanf(temp_buf, "%x %x %x",
                &bus_num, &device_num, &slot_num);
        if (n == 3) {
            if ((bus == bus_num) && (device == device_num)) {
                /* We found the match */
#ifdef NCT_PORT_MAP_DEBUG
                printf("Bus: %02x, device: %02x, slot: %x\n",
                        bus_num, device_num, slot_num);
#endif
                pclose(output_stream);
                return slot_num;
            }
        }
    }
    pclose(output_stream);

    /* We are here because we did not find a match */
    fprintf(stderr,
            "ERROR: did not find match for pci bus/device %d,%d, %s,%d\n",
            bus, device, __FILE__, __LINE__);

    return UNKNOWN_SLOT;

}

static int32_t update_net_adapter_slot_info()
{
    int port_count[MAX_PHYSICAL_SLOTS]; /* number of ports for a given
                                           slot */
    nct_network_adapter_p adapter = net_adapter_list_head;

    memset(port_count, 0, MAX_PHYSICAL_SLOTS* sizeof(int));
    do {
        // for each entry in net_adpater_pci_info get slto number
        adapter->slot = get_slot_number(adapter->bus, adapter->device);
        if (adapter->slot < 0) {
            /* failed to get slot number, move on to next adapter */
            continue;
        }

        /* Set slot port number. 
Assumption: The port order on the MEZZ/LOM devices in 
PCI Bus/Device/Function order.

The adapter list is already in sorted order.

port is zero based, so assign and inc. 
         */
        if (is_adapter_partitioned(adapter->bus,adapter->device) && 
                (adapter->function >= 2)) {
            adapter->slot_port = -1;
        } else {
            adapter->slot_port = port_count[adapter->slot];
            port_count[adapter->slot]++;
        }

    } while ((adapter = adapter->next) != net_adapter_list_head);
    return SUCCESS;	
}

    static uint32_t

pci_class(char *name)
{
    uint32_t val;
    FILE *fd;
    char buf[TEMP_LEN];
    int dummy;

    sprintf(buf, "%s/%s/class", pci_devices_dir, name);
    if ((fd = fopen(buf, "r")) == NULL) {
        fprintf(stderr,
                "ERROR: could not open file %s:  %s,%d\n",
                buf, __FILE__,__LINE__);
        return 0;
    }

    dummy = fscanf(fd, "%x", &val);
    fclose(fd);
    return val;
}


////////////////////////////////////////////////////////////////////////////////
/// \brief get list of network adapters.
///
/// This function goes through the list of available PCI devices under 
/// /sys/bus/pci/devices/ to find devices of class ETHERNET_CONTROLLER.
///
/// \author Mahesh Salgaonkar
/// \return values:
/// \	SUCCESS		Success (zero).
/// \   -1		Failed to get network adapters.	
///
////////////////////////////////////////////////////////////////////////////////
    static int32_t
get_network_adapters()
{
    uint32_t temp;
    struct dirent **namelist;
    uint32_t bus, device, function;
    ushort vendor_id, device_id, subsystem_device, subsystem_vendor;
    int32_t port;
    int file_count, i;

    file_count = scandir(pci_devices_dir, &namelist, file_select, alphasort);
    if (file_count < 0) {
        /*
           fprintf(stderr,
           "ERROR: Failed to scan dir %s: %s,%d\n",
           pci_devices_dir, __FILE__,__LINE__);

           Fix for QXCR1000431312: Errors in cma.log after installing 7.9.0 Agents on
           ML350 G5 with ESX 3.1

           The VMware OS does not have /sys filesystem support. Hence 
           Printing as "INFO" instead of "ERROR".
         */
        fprintf(stderr,
                "INFO: Directory '/sys' not found, Skipping scan dir %s: %s,%d\n",
                pci_devices_dir, __FILE__,__LINE__);
        return -1;
    }

    for (i = 0; i < file_count; i++) {
#ifdef NCT_PORT_MAP_DEBUG
        printf("Checking class for %s\n", namelist[i]->d_name);
#endif
        if (pci_class(namelist[i]->d_name) == ETHERNET_CONTROLLER) {
            // get pci info
            sscanf(namelist[i]->d_name, "%x:%x:%x.%x", 
                    &temp, &bus, &device, &function); 
            get_pci_info(namelist[i]->d_name, &vendor_id, &device_id, 
                         &subsystem_device, &subsystem_vendor);
            port = assign_port_value(vendor_id, device_id, subsystem_device, 
                                     subsystem_vendor);
            add_net_adapter(bus, device, function, port);
        }
        free(namelist[i]);
    }
    free(namelist);

    if (net_adapter_list_head != NULL) {
        update_net_adapter_slot_info();
    }

    return SUCCESS;
}

static nct_network_adapter_p get_net_adapter_by_slot_and_port(int32_t slot, 
                                                              int32_t slot_port)
{
    nct_network_adapter_p adapter = net_adapter_list_head;

    if (adapter == NULL) {
        return NULL;
    }

    do {
        if ((adapter->slot == slot) &&
                (adapter->slot_port == slot_port)) {
            return adapter;
        }
    } while ((adapter = adapter->next) != net_adapter_list_head);

    /* No match found */
    return NULL;
}

static nct_network_adapter_p get_net_adapter_by_BusDevFun(uint32_t bus, 
        uint32_t device, 
        uint32_t function, 
        int32_t port)
{
    nct_network_adapter_p adapter = net_adapter_list_head;

    if (adapter == NULL) {
        return NULL;
    }

    do {
        if ((adapter->bus == bus) && 
            (adapter->device == device) && 
            (adapter->function == function) && 
            (adapter->port == port)) {
            return adapter;
        }
    } while ((adapter = adapter->next) != net_adapter_list_head);

    /* No match found */
    return NULL;
}

static int32_t update_adapter_info(int32_t slot_num, int32_t slot_port,
                                   int32_t bay, int32_t bay_port)
{
    nct_network_adapter_p adapter;
    nct_nic_port_map_info_p port_map_info;

    adapter = get_net_adapter_by_slot_and_port(slot_num, slot_port);
    if (adapter == NULL) {
        /* Oops... There is really something wrong happened.

           Poosible Reason would be 'hplog -i' command did not give us
           the slot number info properly for one of the adapters.

           if this happens then, nic port mapping info will be wrong
           for one/few of the adapters 
         */
        return -1;
    }

    if (adapter->nic_port_map) {
        /* Why are we here? No way this can happen. 
           Is SMBIOS data is corrupted?? OR we made a  mistake in
           updating adapter->slot_port value.
         */
        return -1;
    }

    port_map_info = (nct_nic_port_map_info_p)
        malloc(sizeof(nct_nic_port_map_info_t));
    CHECK_MALLOC(port_map_info);
    memset(port_map_info, 0, sizeof(nct_nic_port_map_info_t));

    port_map_info->slot = slot_num;
    port_map_info->slot_port = slot_port;
    port_map_info->SWM_bay = bay;
    port_map_info->SWM_bay_port = bay_port;

    adapter->nic_port_map = port_map_info;

    return SUCCESS;
}

static int32_t get_smbios_nic_port_map()
{
    FILE *output_stream = NULL;
    struct stat stat_buf;
    char temp_buf[TEMP_LEN];
    static const char *hpasmcli_cmd = NULL;
    int32_t n;
    int32_t slot_num, slot_port, bay, bay_port;
    char * dummy;

    if (hpasmcli_cmd == NULL) {
        if (stat(hpasmcli_utils, &stat_buf) != -1) {
            hpasmcli_cmd = hpasmcli_utils;
        }
        else if (stat(hpasmcli_sbin, &stat_buf) != -1) {
            hpasmcli_cmd = hpasmcli_sbin;
        } else {
            DPRINTF("ERROR: could not find hpasmcli utility %s,%d\n",
                    __FILE__,__LINE__);
            return -1;
        }
    }
    /* Build command */
    sprintf(temp_buf,
            "%s -s \"show portmap\" 2>/dev/null", hpasmcli_cmd);
#ifdef NCT_PORT_MAP_DEBUG
    printf ("Executing Command: %s\n", temp_buf);
#endif
    output_stream = popen(temp_buf, "r");
    if (output_stream == NULL) {
        fprintf(stderr,
                "ERROR: could not execute command: %s at %s,%d\n",
                temp_buf, __FILE__,__LINE__);
        return UNKNOWN_SLOT;

    }
    /* read the first two lines, table info  */
    dummy = fgets(temp_buf, TEMP_LEN, output_stream);

    dummy = fgets(temp_buf, TEMP_LEN, output_stream);

    /* read past the header  "SlotNumber...." */
    dummy = fgets(temp_buf, TEMP_LEN, output_stream);

    while (fgets(temp_buf, TEMP_LEN, output_stream)) {
        n = sscanf(temp_buf, "%d %d %d %d",
                &slot_num, &slot_port, &bay, &bay_port);
        if (n == NUM_PORT_MAP_ITEMS) {
#ifdef NCT_PORT_MAP_DEBUG
            printf("SMBIOS data\n");
            printf("Slot: %d, Slot Port: %d, Bay: %d,"
                    "Bay Port: %d\n",
                    slot_num, slot_port, bay, bay_port);
#endif
            update_adapter_info(slot_num, slot_port, bay, bay_port);
        }
    }
    pclose(output_stream);

    return 0;

}

#ifdef NCT_PORT_MAP_DEBUG
static void print_adapters()
{
    nct_network_adapter_p adapter = net_adapter_list_head;

    if (adapter == NULL)
        return;

    printf("%-10s%-10s%-10s%-10s%-10s%-14s%-10s%-10s\n",
            "BusNumber", "DevNumber", "Function",
            "Slot", "SlotPort", "VirtualPort", "Bay", "BayPort");
    do {
        printf("%-10x%-10x%-10x%-10d%-10d%-14d",
                adapter->bus, adapter->device,
                adapter->function, 
                adapter->slot,
                adapter->slot_port,adapter->virtual_port);
        if (adapter->nic_port_map) {
            printf("%-10d%-10d\n",
                    adapter->nic_port_map->SWM_bay,
                    adapter->nic_port_map->SWM_bay_port);
        }
        else {
            printf("%-10d%-10d\n", UNKNOWN_BAY, UNKNOWN_PORT);
        }
    } while ((adapter = adapter->next) != net_adapter_list_head);
}
#endif

////////////////////////////////////////////////////////////////////////////////
/// \brief Intialize port map table
///
/// Returns the slot, slot_port, bay and bay_port info for an interface
/// specified by bus/device/fundtion.
/// The information is pulled from nic port map table.
///
/// \author Mahesh Salgaonkar
/// \param[in] pif ptr to interface structure to populate
/// \retval int32_t 0 on success, non-zero on failure
///
////////////////////////////////////////////////////////////////////////////////
    static int32_t
init_port_map_table()
{
    get_network_adapters();
    get_smbios_nic_port_map();
#ifdef NCT_PORT_MAP_DEBUG
    print_adapters();
#endif
    return 0;	
}

////////////////////////////////////////////////////////////////////////////////
/// \brief get slot and bay info
///
/// Returns the slot, slot_port, bay and bay_port info for an interface
/// specified by bus/device/fundtion.
/// The information is pulled from nic port map table.
///
/// \author Mahesh Salgaonkar
/// \param[in] pif ptr to interface structure to populate
/// \retval int32_t 0 on success, non-zero on failure
/// \return values:
/// \	E_NCT_INVALID	Invalid input parameters
/// \	E_NCT_BAD_SLOT	Could not find slot and bay info for the specified 
/// \			network adapter.
/// \	SUCCESS		Success (zero).
/// \			1. On C-class blades slot and bay info
/// \			   is populated upon SUCCESS.
/// \			2. On non c-class system only slot info gets populated 
/// \			   upon SUCCESS and Bay info is populated with -1
/// \			   values.
///
////////////////////////////////////////////////////////////////////////////////
int32_t get_slot_bay_info(int32_t bus, int32_t device, int32_t function,
        int32_t* slot, int32_t* slot_port,
        int32_t* bay, int32_t* bay_port, 
        int32_t* virtual_port,		
        int32_t port)
{
    static int need_init = 1;
    nct_network_adapter_p adapter,base_adapter;
    int32_t ret = SUCCESS;

    if (need_init == 1) {
        need_init = 0;
        init_port_map_table();
    }
    *bay = UNKNOWN_BAY;
    *bay_port = UNKNOWN_PORT;
    adapter = get_net_adapter_by_BusDevFun(bus, device, function, port);
    if (adapter) {
        *slot = adapter->slot;
        *slot_port = (adapter->slot_port == UNKNOWN_PORT)
            ?UNKNOWN_PORT:(adapter->slot_port + 1);
        *virtual_port = adapter->virtual_port;

        if (*slot == UNKNOWN_SLOT) 
            ret = E_NCT_BAD_SLOT;

        if (adapter->nic_port_map) {
            *bay = adapter->nic_port_map->SWM_bay;
            *bay_port = adapter->nic_port_map->SWM_bay_port;
        } else if (adapter->virtual_port != UNKNOWN_PORT) {
            base_adapter = get_net_adapter_by_BusDevFun(bus, device, 
                                                        (function%2), port);
            if (base_adapter) {
                if(base_adapter->nic_port_map) { 
                    *bay = base_adapter->nic_port_map->SWM_bay;
                    *bay_port = base_adapter->nic_port_map->SWM_bay_port;
                }
            } else 
                ret=E_NCT_INVALID;
        }    
    } else 
        /* No adapter found. Invalid Input */
        ret = E_NCT_INVALID;

    return ret;	
}


////////////////////////////////////////////////////////////////////////////////
/// \brief Unit test for this file
///
/// \author Tony Cureington
/// \param[in] none
/// \retval none
///
////////////////////////////////////////////////////////////////////////////////
#ifdef UNIT_TEST

int main()
{
    int bus, device, function;
    int slot = 0, slot_port = 0, bay = 0, bay_port = 0, virtual_port = 0,port = -1;
    int ret;

    printf("Testing NIC port mapping function\n");
    //printf("Enter Bus/Device/function values\n");
    bus = 0x03;
    device = 0x0;
    function = 0x0;
    ret = is_adapter_partitioned(bus,device);
    //	printf ("Return Value is %d\n", ret);

    init_port_map_table();
    for(function=0;function<8;function++) {
        ret =  get_slot_bay_info(bus, device, function,
                &slot, &slot_port, &bay, &bay_port,&virtual_port,port);
        printf ("bus %x device %x function %x port %x slot: %d, slot port: %d, bay: %d, bay port: %d virtual_port %d\n",bus,device,function,
                port,slot, slot_port, bay, bay_port,virtual_port);
    }
    //init_port_map_table();
    return(0);
}

#endif // UNIT_TEST

/******************************************************************************
 *         Copyright (c) 2011  Hewlett Packard Corporation                
 ******************************************************************************
 *                                                                        
 *   Description:                                                         
 *     This file contains the routines to gather the information the NIC  
 *     drivers place in the ethN.info files in the /proc/net/ directory;  
 *     Please see the design document for the structure to proc variable  
 *     mapping algorithm;                                                 
 *                                                                        
 ******************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <net/if.h>
#include <netinet/in.h>
#include <linux/pci.h>
#include <sys/time.h>
#include <dlfcn.h>
#include <dirent.h>
#include <linux/types.h>
typedef __u64 u64;
typedef __u32 u32;
typedef __u16 u16;
typedef __u8 u8;
#include <linux/ethtool.h>
#include <fcntl.h>
#include "utils.h"
#include "nic_linux.h"
#include "nic_db.h"

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <net-snmp/agent/agent_index.h>
#include <net-snmp/library/container.h>
#include <net-snmp/library/snmp_debug.h>

#define SYSBUSPCISLOTS "/sys/bus/pci/slots/"

nic_hw_db * get_nic_hw_info(ushort pci_ven, ushort pci_dev,
        ushort pci_sub_ven, ushort pci_sub_dev)
{
    int32_t i;
    for (i=0; i<NIC_DB_ROWS; i++) {
        /* yes, brute force this search - there is no need to get fancy
           since there are less than 100 entries... */
        if ((pci_ven     == nic_hw[i].pci_ven)     &&
                (pci_dev     == nic_hw[i].pci_dev)     &&
                (pci_sub_ven == nic_hw[i].pci_sub_ven) &&
                (pci_sub_dev == nic_hw[i].pci_sub_dev)) {
            return(&nic_hw[i]);
        }
    }
    return(NULL);
}
/******************************************************************************
 *   Description: Get the MIB-II interface index from the interface name. 
 *                The index is associated with the index of               * 
 *                interfaces.ifTable.ifEntry.ifDescr.x for example.       
 *                                                                        
 *   Parameters : pointer to string, i.e. "eth0", "bond0", etc.           
 *                                                                        
 *   Returns    : Index on success, -1 on failure                         
 ******************************************************************************/
int name2indx(char *name)
{
    char buffer[80];

    snprintf(buffer, 80, "/sys/class/net/%s/ifindex", name);
    return (get_sysfs_int(buffer));
}

int name2pindx(char *name)
{
    char buffer[80];
    char *dev;
    char * dot;

    if ((dev = malloc(strlen(name) + 1)) !=  NULL) {
        memset(dev, 0, strlen(name) + 1);
        strncpy(dev, name, strlen(name));

        if ((dot = strchr(dev, '.')) != 0)
            *dot = '\0';
        snprintf(buffer, 80, "/sys/class/net/%s/ifindex", dev);
        free(dev);
        return (get_sysfs_int(buffer));
    }
    return -1;
}


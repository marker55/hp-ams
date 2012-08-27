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

int32_t get_pcislot(char *bus_info)
{
    struct dirent **filelist;
    int slotcount;
    char fname[256] = "/sys/bus/pci/slots/";
    int sysdirlen;
    char buf[80];
    int bc;
    int slotfd = -1;
    int i;
    int ret = -1;


    /* the bus_info var is in "domain:bus:slot.func". 
     * sometimes the domain may be absent, in which case 
     * we have "bus:slot.func". */
    DEBUGMSGTL(("cpqNic:slotinfo","bus_info = %s\n", bus_info));
    if ((slotcount = scandir(fname, &filelist, file_select, alphasort)) > 0) {
        sysdirlen = strlen(fname);
        for (i = 0; i < slotcount; i++) {
            strcpy(&fname[sysdirlen], filelist[i]->d_name);
            strcat(fname, "/address");
            if ((slotfd = open(fname, O_RDONLY)) != -1 ) {
                bc = read(slotfd, buf, 80);
                buf[bc] = 0;
                DEBUGMSGTL(("cpqNic:slotinfo","bc=%d buf=%s\n", bc, buf));
                if (bc  > 0) {
                    if (!strncmp(buf, bus_info, bc - 1))
                        ret = i + 1;
                }
                close(slotfd);
            }
            free (filelist[i]);
        }
        free(filelist);
    } 
    return ret;
}



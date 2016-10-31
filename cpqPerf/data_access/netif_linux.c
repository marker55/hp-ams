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
#include <dirent.h>
#include <unistd.h>
#include <getopt.h>

#include "cpqLinOsNetworkInterfaceTable.h"

#include "common/utils.h"


int netsnmp_arch_netif_container_load(netsnmp_container* container)
{
    cpqLinOsNetworkInterfaceTable_entry *entry;
    netsnmp_index tmp;
    oid oid_index[2];
    oid  index = 0;
    long  rc = 0;

    struct timeval curr_time;
    struct timeval prev_time;
    struct timeval e_time;
    unsigned long long interval = 0;
    int i;

    read_line_t *netiflines = NULL;

    gettimeofday(&prev_time, NULL);
    gettimeofday(&curr_time, NULL);

    DEBUGMSGTL(("linosnetif:container:load", "loading\n"));

    if ((netiflines = ReadFileLines("/proc/net/dev")) != NULL) {
        DEBUGMSGTL(("linosnetif:container:load", "linecount = %d\n",netiflines->count));
        for (i = 2; i < netiflines->count; i++) {
            int num_if_elem;
            char iface[16];
            unsigned long long rxbytes = 0, rxpackets = 0, rxerrs = 0, rxdrop = 0, 
                            rxfifo = 0, rxframe = 0, rxcomp = 0, rxmulti = 0;
            unsigned long long txbytes = 0, txpackets = 0, txerrs = 0, txdrop = 0, 
                            txfifo = 0, txcoll = 0, txcar = 0, txcomp = 0;
        DEBUGMSGTL(("linosnetif:container:load", "line = %s\n",netiflines->line[i]));
            num_if_elem = sscanf(netiflines->line[i],
                                "%15[^:]:%llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
                                iface,
                                &rxbytes, &rxpackets, &rxerrs, &rxdrop, 
                                &rxfifo, &rxframe, &rxcomp, &rxmulti,
                                &txbytes, &txpackets, &txerrs, &txdrop, 
                                &txfifo, &txcoll, &txcar, &txcomp);
    
            DEBUGMSGTL(("linosnetif:container:load", "parsed = %d\n",num_if_elem));
            oid_index[0] = index;
            tmp.len = 1;
            tmp.oids = &oid_index[0];
    
            entry = CONTAINER_FIND(container, &tmp);
            if (entry) {
                prev_time.tv_sec = entry->curr_time.tv_sec;
                prev_time.tv_usec = entry->curr_time.tv_usec;
    
                entry->prev_txbytes = entry->curr_txbytes;
                entry->prev_txpkts = entry->curr_txpkts;
                entry->prev_rxbytes = entry->curr_rxbytes;
                entry->prev_rxpkts = entry->curr_rxpkts;
    
                entry->curr_txbytes = txbytes;
                entry->curr_txpkts = txpackets;
                entry->curr_rxbytes = rxbytes;
                entry->curr_rxpkts = rxpackets;
    
                entry->cpqLinOsNetworkInterfaceTxBytes = 
                    entry->curr_txbytes - entry->prev_txbytes;
                entry->cpqLinOsNetworkInterfaceTxPackets = 
                    entry->curr_txpkts - entry->prev_txpkts;
                entry->cpqLinOsNetworkInterfaceRxBytes =
                    entry->curr_rxbytes - entry->prev_rxbytes;
                entry->cpqLinOsNetworkInterfaceRxPackets = 
                    entry->curr_rxpkts - entry->prev_rxpkts;
    
                entry->curr_time.tv_sec = curr_time.tv_sec;
                entry->curr_time.tv_usec = curr_time.tv_usec;
                entry->prev_time.tv_sec = prev_time.tv_sec;
                entry->prev_time.tv_usec = prev_time.tv_usec;
                timersub(&entry->curr_time, &entry->prev_time, &e_time);
                interval = (e_time.tv_sec*1000 + e_time.tv_usec/1000)/10;
                if (interval) {
                    entry->cpqLinOsNetworkInterfaceTxBytesPerSec = 
                        entry->cpqLinOsNetworkInterfaceTxBytes/interval;
                    entry->cpqLinOsNetworkInterfaceTxPacketsPerSec =
                        entry->cpqLinOsNetworkInterfaceTxPackets/interval;
                    entry->cpqLinOsNetworkInterfaceRxBytesPerSec =
                        entry->cpqLinOsNetworkInterfaceRxBytes/interval;
                    entry->cpqLinOsNetworkInterfaceRxPacketsPerSec =
                        entry->cpqLinOsNetworkInterfaceRxPackets/interval;
                }
            } else {
                char * start;
                entry = cpqLinOsNetworkInterfaceTable_createEntry(container, (oid)index);
                if ((start = strrchr(iface, ' ')) != (char *)NULL)
                    strncpy(entry->cpqLinOsNetworkInterfaceName, ++start,
                            sizeof(entry->cpqLinOsNetworkInterfaceName) - 1);
                else 
                    strncpy(entry->cpqLinOsNetworkInterfaceName, iface,
                            sizeof(entry->cpqLinOsNetworkInterfaceName) - 1);
                entry->cpqLinOsNetworkInterfaceName_len = 
                    strlen(entry->cpqLinOsNetworkInterfaceName);
    
                entry->prev_rxbytes = 0;
                entry->prev_rxpkts = 0;
                entry->prev_txbytes = 0;
                entry->prev_txpkts = 0;
    
                entry->curr_rxbytes = rxbytes;
                entry->curr_rxpkts = rxpackets;
                entry->curr_txbytes = txbytes;
                entry->curr_txpkts = txpackets;
    
                entry->curr_time.tv_sec = curr_time.tv_sec;
                entry->curr_time.tv_usec = curr_time.tv_usec;
                entry->prev_time.tv_sec = prev_time.tv_sec;
                entry->prev_time.tv_usec = prev_time.tv_usec;
                rc = CONTAINER_INSERT(container, entry);
            }
            index++;
        }
        free(netiflines->read_buf);
        free(netiflines);
    }
    return (rc);
}







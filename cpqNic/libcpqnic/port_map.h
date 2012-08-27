// (c) Copyright 2006 Hewlett-Packard Development Company, L.P.
// All rights reserved
//
////////////////////////////////////////////////////////////////////////////////
/// \file port_map.h
///
/// This file contains C declarations, no definitions (i.e. memory allocation) 
/// are in this file.
///
////////////////////////////////////////////////////////////////////////////////


#ifndef PORT_MAP_H
#define PORT_MAP_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
//#include <linux/list.h>

#include "common_structs.h"

////////////////////////////////////////////////////////////////////////////////
// DEFINES - NON MACRO
////////////////////////////////////////////////////////////////////////////////

/// unknown slot nic is in
#define UNKNOWN_SLOT -1

/// unknown port of nic or bay
#define UNKNOWN_PORT -1

/// unknown bay port is in
#define UNKNOWN_BAY  -1

/// Invalid input parameters
#define E_NCT_INVALID   -1

/// Could not find slot and bay info
#define E_NCT_BAD_SLOT  -2


////////////////////////////////////////////////////////////////////////////////
// DEFINES - MACROS
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// STRUCTS/ENUMS
////////////////////////////////////////////////////////////////////////////////

/// nic port map info
typedef struct nct_nic_port_map_info {
   /// ---
	int32_t slot;		        /// slot number
	int32_t slot_port;	     /// 0-based port number on MEZZ
	int32_t SWM_bay; 	        /// Switch bay (SWM) number
	int32_t SWM_bay_port;     /// Switch bay (SWM) port number
	
} nct_nic_port_map_info_t, *nct_nic_port_map_info_p;

/// network adapter info
typedef struct nct_network_adapter {
   /// ---
	int32_t slot;		   /// slot number
	int32_t slot_port; 	/// 0-based port number on Mezz/LOM
 	int32_t virtual_port;	/// virtual port when nic is partitioned	
	uint32_t bus;        /// pci bus
	uint32_t device;     /// pci device
	uint32_t function;   /// pci function
        int32_t port;
	
	nct_nic_port_map_info_p nic_port_map; /// port map struct
	struct nct_network_adapter *prev;     /// previous adapter
	struct nct_network_adapter *next;     /// next adapter

} nct_network_adapter_t, *nct_network_adapter_p;

#endif /* STRUCTS_H */

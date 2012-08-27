// (c) Copyright 2006 Hewlett-Packard Development Company, L.P.
// All rights reserved
//
////////////////////////////////////////////////////////////////////////////////
/// \file common_utilities.h
///
/// This file contains header files for external applications to include. This
/// is the only file that needs to be included by external applications wishing
/// to link with the libcommon_nic.a library.
///
////////////////////////////////////////////////////////////////////////////////


#ifndef COMMON_UTILTIES_H
#define COMMON_UTILTIES_H

/// \warning This is a hack to prevent linux/in.h and netinet/in.h from 
///          redeclaration types that are included in both header files.
///          There is no way I've found around this issue, other system header
///          files included one or the other and end up breaking things.
/// \todo    See the linux/in.h and netinet/in.h duplicate declarations has
///          been fixed.
#ifndef  _NETINET_IN_H
#define  _NETINET_IN_H  1
/* Internet address.  */
typedef unsigned int in_addr_t;
#endif

// allow C++ to link against this header file - don't mangle names
#ifdef __cplusplus
extern "C" {
#endif

/// enable GNU extensions, like getline function
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

////////////////////////////////////////////////////////////////////////////////
// SYSTEM HEADER FILES
////////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <malloc.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <linux/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <dirent.h>


////////////////////////////////////////////////////////////////////////////////
// LOCAL HEADER FILES
////////////////////////////////////////////////////////////////////////////////
#include "common_structs.h"
#include "port_map.h"
#include "common_prototypes.h"

#ifdef __cplusplus
} // closing brace for extern "C"
#endif

#define SYSFS_PATH_MAX          256

#define PATH_TO_NET "/sys/class/net/"

#endif /* COMMON_UTILTIES_H */

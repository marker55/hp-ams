#define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/mman.h>

#include "package.h"

#include <strings.h>

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <net-snmp/library/container.h>

#include <net-snmp/output_api.h>
#include "../cpqHost/cpqHoSwVerTable.h"

extern oid       cpqHoSwVerTable_oid[];
extern size_t    cpqHoSwVerTable_oid_len;

extern pkg_entry **packages;

int getPackage()
{
    int timelen;
    netsnmp_container *pkg_container = NULL;
    netsnmp_cache *pkg_cache = NULL;
    netsnmp_iterator  *it;
    cpqHoSwVerTable_entry *entry;
    int i;
    int pkgcount = 0;

    pkg_cache = netsnmp_cache_find_by_oid(cpqHoSwVerTable_oid, 
					  cpqHoSwVerTable_oid_len);
    if (pkg_cache == NULL)
        return  0;

    pkg_container = pkg_cache->magic;
    if (pkg_container == NULL)
        return 0;

    pkgcount = (oid)CONTAINER_SIZE(pkg_container);

    if ((packages = (pkg_entry**)malloc(pkgcount * sizeof(pkg_entry *)))
                    == NULL) {
        return 0;
    }

    it = CONTAINER_ITERATOR( pkg_container );
    entry = ITERATOR_FIRST( it );
    i = 0;
    while (entry != NULL) {

        if ((packages[i] = (pkg_entry*)malloc(sizeof(pkg_entry))) == NULL) {
            continue;
        }
        memset(packages[i], 0, sizeof(pkg_entry));

        if (entry->cpqHoSwVerName_len != 0 ) {
            packages[i]->sname = malloc(entry->cpqHoSwVerName_len + 1);
            memset(packages[i]->sname, 0, entry->cpqHoSwVerName_len + 1);
            strcpy(packages[i]->sname, entry->cpqHoSwVerName);

            packages[i]->name = malloc(entry->cpqHoSwVerName_len + 1);
            memset(packages[i]->name, 0, entry->cpqHoSwVerName_len + 1);
	    strcpy(packages[i]->name,  entry->cpqHoSwVerName);
        } else {
            packages[i]->sname = NULL;
            packages[i]->name = NULL;
        }

        if (entry->cpqHoSwVerVersion_len != 0 ) {
            packages[i]->version = malloc(entry->cpqHoSwVerVersion_len + 1);
            memset(packages[i]->version, 0, entry->cpqHoSwVerVersion_len + 1);
	    strcpy(packages[i]->version, entry->cpqHoSwVerVersion);
        } else
            packages[i]->version = NULL;

        if (entry->vendor_len != 0 ) {
            packages[i]->vendor = malloc(entry->vendor_len + 1);
            memset(packages[i]->vendor, 0, entry->vendor_len + 1);
	    strcpy(packages[i]->vendor, entry->vendor);
        } else
            packages[i]->vendor = NULL;

        if (entry->timestamp != 0) {
            packages[i]->timestamp = malloc(80);
            memset(packages[i]->timestamp, 0, 80);
            timelen = strftime(packages[i]->timestamp, 80,
                               "%Y-%m-%d %H:%M:%S %z",
                               gmtime(&entry->timestamp));
        } else
            packages[i]->timestamp = NULL;
                                 
        i++;
        entry = ITERATOR_NEXT( it );
    }
    ITERATOR_RELEASE( it );

    return pkgcount;
}


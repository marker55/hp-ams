#include <net-snmp/net-snmp-config.h>

#include <rpm/header.h>
#include <fcntl.h>
#ifdef HAVE_RPM_RPMLIB_H
#include <rpm/rpmlib.h>
#endif
#ifdef HAVE_RPM_RPMLIB_H
#include <rpm/header.h>
#endif
#ifdef HAVE_RPMGETPATH          /* HAVE_RPM_RPMMACRO_H */
#include <rpm/rpmmacro.h>
#endif
#ifdef HAVE_RPM_RPMTS_H
#include <rpm/rpmts.h>
#include <rpm/rpmdb.h>
#endif
#include <sys/stat.h>
#include <rpm/rpmlib.h>

#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <net-snmp/library/container.h>
#include <net-snmp/library/snmp_debug.h>

#include "../cpqHoSwVerTable.h"

#define SWAP_BYTES(x)  ((unsigned short)(((x) & 0xff)<<8) | (((x)>>8) & 0xff))

/*
 * Location of RPM package directory.
 * Used for:
 *    - reporting hrSWInstalledLast* objects
 *    - detecting when the cached contents are out of date.
 */
extern char pkg_directory[];

void
netsnmp_cpqHoSwVer_arch_init(void)
{
    return;
}

void
netsnmp_cpqHoSwVer_arch_shutdown(void)
{
    /* Nothing to do */
    return;
}



int
cpqhost_arch_cpqHoSwVer_container_load( netsnmp_container *container)
{
    rpmts                 ts;
    rpmdbMatchIterator    mi;
    Header                h;
    const char           *n, *v, *r, *vendor, *description;
    uint32_t              *t;
    int                   rc = 0, i = 0;
    struct tm            *td;
    cpqHoSwVerTable_entry *entry;

    ts = rpmtsCreate();
    rpmtsSetVSFlags( ts, (_RPMVSF_NOSIGNATURES|_RPMVSF_NODIGESTS));

    mi = rpmtsInitIterator( ts, RPMDBI_PACKAGES, NULL, 0);
    if (mi == NULL)
        NETSNMP_LOGONCE((LOG_ERR, "rpmdbOpen() failed\n"));

    while (NULL != (h = rpmdbNextIterator( mi ))) {

#ifdef NEWRPM
        struct rpmtd_s tag_data;
#endif
        h = headerLink( h );
#ifdef NEWRPM
        headerGet(h, RPMTAG_VENDOR, &tag_data, HEADERGET_DEFAULT);
        vendor = rpmtdGetString(&tag_data);
        rpmtdFreeData(&tag_data);
#else
        headerGetEntry(h, RPMTAG_VENDOR, NULL, (void **) &vendor, NULL);
#endif
        if ((vendor != NULL) &&
                ((strncmp(vendor, "Hewlett", 7) == 0) || 
                 (strcmp(vendor, "HP") == 0))) {
            entry = cpqHoSwVerTable_createEntry( container,(oid) i++ );
            if (NULL == entry) {
#ifdef NEWRPM
                rpmtdFreeData(&tag_data);
#endif
                headerFree( h );
                continue;   /* error already logged by function */
            }

#ifdef NEWRPM
            headerGet(h, RPMTAG_NAME,        &tag_data, HEADERGET_DEFAULT);
            n = rpmtdGetString(&tag_data);
            rpmtdFreeData(&tag_data);

            headerGet(h, RPMTAG_VERSION,     &tag_data, HEADERGET_DEFAULT);
            v = rpmtdGetString(&tag_data);
            rpmtdFreeData(&tag_data);

            headerGet(h, RPMTAG_RELEASE,     &tag_data, HEADERGET_DEFAULT);
            r = rpmtdGetString(&tag_data);
            rpmtdFreeData(&tag_data);

            headerGet(h, RPMTAG_SUMMARY,     &tag_data, HEADERGET_DEFAULT);
            description = rpmtdGetString(&tag_data);
            rpmtdFreeData(&tag_data);

            headerGet(h, RPMTAG_INSTALLTIME, &tag_data, HEADERGET_DEFAULT);
            t = rpmtdGetUint32(&tag_data);
            rpmtdFreeData(&tag_data);
#else
            headerGetEntry( h, RPMTAG_NAME,        NULL, (void**)&n, NULL);
            headerGetEntry( h, RPMTAG_VERSION,     NULL, (void**)&v, NULL);
            headerGetEntry( h, RPMTAG_RELEASE,     NULL, (void**)&r, NULL);
            headerGetEntry( h, RPMTAG_INSTALLTIME, NULL, (void**)&t, NULL);
            headerGetEntry( h, RPMTAG_SUMMARY,     NULL,
                    (void **) &description, NULL);
#endif

            strncpy(entry->cpqHoSwVerName, n, sizeof(entry->cpqHoSwVerName) - 1);
            entry->cpqHoSwVerName_len = strlen(entry->cpqHoSwVerName);

            entry->cpqHoSwVerType = 5;     /*  application    */
            if (strstr(n, "-firmware-"))
                entry->cpqHoSwVerType = 7;  /* need to add new type */
            if (!strncmp(n, "kmod-", 5) || !strstr(n, "-kmp-"))
                entry->cpqHoSwVerType = 2; 
            if (entry->cpqHoSwVerType == 5) {
                if (strcasestr(description, "agent"))
                     entry->cpqHoSwVerType = 3;
                if (strcasestr(description, "tool") || 
                    strcasestr(description, "util"))
                     entry->cpqHoSwVerType = 4;
            }

            if (description != NULL ) {
                DEBUGMSGTL(("cpqHoSwVerTable:load:arch", "description = %s\n",
                            description));
                strncpy(entry->cpqHoSwVerDescription, description,
                        sizeof(entry->cpqHoSwVerDescription) - 1);
                entry->cpqHoSwVerDescription_len = 
                    strlen(entry->cpqHoSwVerDescription);
            }

            entry->timestamp = 0;
            memset(&entry->cpqHoSwVerDate[0], 0, 8);
            entry->cpqHoSwVerDate_len = 0;
            if ( t ) {
                DEBUGMSGTL(("cpqHoSwVerTable:load:arch", "date = %d\n", *t));
                if ((td = localtime((time_t *)t)) != NULL) {
                    entry->cpqHoSwVerDate[0] = (char)
                        (SWAP_BYTES(td->tm_year + 1900));
                    entry->cpqHoSwVerDate[1] = (char) (td->tm_year + 1900);
                    entry->cpqHoSwVerDate[2] = (char)(td->tm_mon + 1);
                    entry->cpqHoSwVerDate[3] = (char)td->tm_mday;
                    entry->cpqHoSwVerDate[4] = (char)td->tm_hour;
                    entry->cpqHoSwVerDate[5] = (char)td->tm_min;
                    entry->cpqHoSwVerDate[6] = (char)td->tm_sec;
                    entry->cpqHoSwVerDate_len = 7;
                    entry->timestamp = *t;
                }
            }

            if (v != NULL ) {
                DEBUGMSGTL(("cpqHoSwVerTable:load:arch", "version = %s\n", v));
                entry->cpqHoSwVerVersion_len = strlen(v);
                strncpy(entry->cpqHoSwVerVersion, v,
                        sizeof(entry->cpqHoSwVerVersion) - 1);
                entry->cpqHoSwVerVersion_len = 
                    strlen(entry->cpqHoSwVerVersion);

                if (r != NULL ) {
                    DEBUGMSGTL(("cpqHoSwVerTable:load:arch", "release = %s\n", r));
                    entry->cpqHoSwVerVersionBinary_len = snprintf(entry->cpqHoSwVerVersionBinary, 
                            sizeof(entry->cpqHoSwVerVersionBinary),
                            "%s-%s", v, r);
                } else
                    entry->cpqHoSwVerVersionBinary_len = snprintf(entry->cpqHoSwVerVersionBinary, 
                            sizeof(entry->cpqHoSwVerVersionBinary),
                            "%s", v);
            }

            strncpy(entry->vendor, vendor, sizeof(entry->vendor) - 1);
            entry->vendor_len = strlen(entry->vendor);

            entry->cpqHoSwVerLocation[0] = '\0';
            entry->cpqHoSwVerLocation_len = 0;

            entry->cpqHoSwVerStatus = 2; /* OK */
            rc = CONTAINER_INSERT(container, entry);

        }
        headerFree( h );
    }
    rpmdbFreeIterator( mi );
    rpmtsFree( ts );

    cpqHoSwVerNextIndex = (oid)CONTAINER_SIZE(container);
    DEBUGMSGTL(("cpqHoSwVerTable:load:arch", "loaded %d entries\n",
                (int)cpqHoSwVerNextIndex));

    return rc;
}


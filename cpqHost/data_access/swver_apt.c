#include <net-snmp/net-snmp-config.h>

#include <fcntl.h>
#include <strings.h>
#include <sys/stat.h>

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
static char apt_fmt[SNMP_MAXBUF];

/* ---------------------------------------------------------------------
 */
void
netsnmp_cpqHoSwVer_arch_init(void)
{
    snprintf(apt_fmt, SNMP_MAXBUF, "%%%d[^#]#%%%d[^#]#%%%d[^#]#%%%d[^\n]s",
        SNMP_MAXBUF, SNMP_MAXBUF, SNMP_MAXBUF, SNMP_MAXBUF);
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
    FILE *p = popen("dpkg-query --show --showformat '${Package}#${Version}#${Essential}#${Description}\n' 'hp*'|grep -v ^\\ \n", "r");
    char package[SNMP_MAXBUF];
    char version[SNMP_MAXBUF];
    char essential[SNMP_MAXBUF];
    char description[SNMP_MAXBUF];
    char buf[BUFSIZ];
    int                   rc = 0, i = 0;
    cpqHoSwVerTable_entry *entry;

    if (p == NULL) {
        snmp_perror("dpkg-list");
        return 1;
    }

    while (fgets(buf, BUFSIZ, p)) {
        DEBUGMSG(("cpqHoSwVerTable", "entry: %s\n", buf));
        entry = cpqHoSwVerTable_createEntry( container,(oid) i++ );
        if (NULL == entry) {
            continue;   /* error already logged by function */
        }

        sscanf(buf, apt_fmt, package, version, essential, description);

	strcpy(entry->cpqHoSwVerName, package);
        entry->cpqHoSwVerName_len = strlen(package);

        entry->cpqHoSwVerType = (strcmp(essential, "yes") == 0)
                        ? 2      /* operatingSystem */
                        : 5;     /*  application    */

        entry->cpqHoSwVerDescription_len = strlen(description);
        strcpy(entry->cpqHoSwVerDescription, description);

        entry->cpqHoSwVerDate_len = 7;
        memset(&entry->cpqHoSwVerDate[0], 0, 8);

        entry->cpqHoSwVerVersion_len = strlen(version);
        strcpy(entry->cpqHoSwVerVersion, version);

        entry->cpqHoSwVerVersionBinary_len = strlen(version);
        strcpy(entry->cpqHoSwVerVersionBinary, version);

        entry->cpqHoSwVerLocation[0] = '\0';
        entry->cpqHoSwVerLocation_len = 0;

        entry->cpqHoSwVerStatus = 2; /* OK */
	entry->timestamp = 0;
        rc = CONTAINER_INSERT(container, entry);

    }

    cpqHoSwVerNextIndex = (oid)CONTAINER_SIZE(container);
    DEBUGMSGTL(("cpqHoSwVerTable:load:arch", "loaded %d entries\n",
            (int)cpqHoSwVerNextIndex));

    return rc;
}


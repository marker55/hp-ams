#include <net-snmp/net-snmp-config.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <mntent.h>
#include <sys/statvfs.h>

#include <dirent.h>
#include <fcntl.h>

#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <net-snmp/library/container.h>
#include <net-snmp/library/snmp_debug.h>

#include "../cpqHoFileSysTable.h"

#define LNXMNTTAB       "/etc/mtab"

#define BLKSIZE         512
#define MAX_DISP        100     /* max management display buf size */
#define MT_ERROR        0x100   /* ms 4.2 file system define */
#define ONEMEG         (1024*1024)

/************************************************************************
 * name: isnetworkfs
 * returns: whether or not filesystem is on a network
 ************************************************************************/
static int isnetworkfs(struct mntent *mnt)
{
    if (strcmp(mnt->mnt_type, "nfs") == 0)
        return 1;
    if (strcmp(mnt->mnt_type, "smb") == 0)
        return 1;
    if (strcmp(mnt->mnt_type, "mvfs") == 0)
        return 1;

    return 0;
}

int
cpqhost_arch_fsys_container_load( netsnmp_container *container)
{
    struct mntent *mnt;
    struct statvfs statfsbuf;

    FILE *mfp;
    char device[BUFSIZ], filsys[BUFSIZ];
    int num_fs;
    int rc = 0;

    cpqHoFileSysTable_entry *entry;

    DEBUGMSGTL(("cpqHost:fsys:container:load", "loading\n"));
    DEBUGMSGTL(("cpqHost:fsys:container:load", "Container=%p\n",container));

    if ((mfp = setmntent(LNXMNTTAB, "r")) == (FILE *) NULL) {
        snmp_log( LOG_ERR, "Failed to open mount table" );
        return -1;
    }

    /*
     * for each entry in the mount table
     *   if entry is not found in filsys queue
     *      create it
     *   update filsys data
     *   mark entry as found
     */
    for (num_fs = 0;; num_fs++) {
        if ((mnt = getmntent(mfp)) == NULL)
            break;
        //NOTE: at the behest of Dreamworks (who mounts hundreds of NFS shares)
        //this is taken out (to prevent unnecessary network traffic).
        if (isnetworkfs(mnt))
            continue;
        strncpy(device, mnt->mnt_fsname, sizeof(device) - 1);
        strncpy(filsys, mnt->mnt_dir, sizeof(filsys) - 1);
        
        if (statvfs(filsys, &statfsbuf) != 0)
            return -1;

        entry = cpqHoFileSysTable_createEntry(container, (oid)num_fs);
        if (NULL == entry) {
		continue;
        }

        entry->cpqHoFileSysDesc_len = 
            snprintf(entry->cpqHoFileSysDesc, 255, "%s on %s", device, filsys);

       /*
        * space total (in megabytes)
        */
        entry->cpqHoFileSysSpaceTotal =
        (int)(((statfsbuf.f_frsize == 0) ? 512.0 :
               (double)statfsbuf.f_frsize * (double)statfsbuf.f_blocks) /
              ONEMEG);

        /*
        * space used (in megabytes)
        */
        if (statfsbuf.f_blocks == 0)
            entry->cpqHoFileSysSpaceUsed = 0;
        else
            entry->cpqHoFileSysSpaceUsed = entry->cpqHoFileSysSpaceTotal -
            (int)(((statfsbuf.f_frsize == 0) ? 512.0 :
                   (double)statfsbuf.f_frsize * (double)statfsbuf.f_bfree) /
                  ONEMEG);

       /*
        * space percentage used
        */
        if (entry->cpqHoFileSysSpaceTotal == 0 || entry->cpqHoFileSysSpaceUsed == 0)
            entry->cpqHoFileSysPercentSpaceUsed = 0;
        else
            entry->cpqHoFileSysPercentSpaceUsed =
                (statfsbuf.f_blocks -
                statfsbuf.f_bfree) * 100.0 / statfsbuf.f_blocks + 0.5;

        /*
        * units Total (number of inodes)
        */
        entry->cpqHoFileSysAllocUnitsTotal = statfsbuf.f_files;
    
        /*
        * units used (number of inodes used)
        */
        if (statfsbuf.f_files == 0)
            entry->cpqHoFileSysAllocUnitsUsed = 0;
        else
            entry->cpqHoFileSysAllocUnitsUsed = statfsbuf.f_files - statfsbuf.f_ffree;

	entry->cpqHoFileSysStatus = 1;

	memset(entry->cpqHoFileSysShortDesc, 0, sizeof(entry->cpqHoFileSysShortDesc));
	entry->cpqHoFileSysShortDesc_len = 0;

        rc = CONTAINER_INSERT(container, entry);

    }
    endmntent(mfp);             

    DEBUGMSGTL(("cpqHost:fsys:container:load"," loaded %d entries\n",
                (int)CONTAINER_SIZE(container)));

    return(rc);
}


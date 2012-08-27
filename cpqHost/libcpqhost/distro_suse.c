#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/statvfs.h>
#include <dirent.h>

#include "cpqhost.h"
#include "common/utils.h"
static char *linux_distro_name = "SuSE";

char *distroinfo = "/etc/SuSE-release";

char *null_str = "";

distro_id_t  *getDistroInfo()
{
    int idx;
    char *tokens[20];
    read_line_t *distrolines = NULL;
    static distro_id_t *distro_id = NULL;
    int machine_len = 0;
    char *str;

    if (distro_id != NULL)
        return distro_id;
    
    if ((distro_id = malloc(sizeof(distro_id_t))) == NULL)
        return NULL;

    uname(&distro_id->utsName);
    distro_id->KernelVersion = strdup((char *)&distro_id->utsName.release);
    machine_len = strlen(distro_id->utsName.machine);
    if (atoi(&distro_id->utsName.machine[machine_len - 2]) == 64)
        distro_id->Bits = 64;
    else
        distro_id->Bits = 32;

    snprintf(distro_id->sysDescr, sizeof(distro_id->sysDescr),
            "%s %s %s %s %s",
            distro_id->utsName.sysname, distro_id->utsName.nodename,
            distro_id->utsName.release, distro_id->utsName.version,
            distro_id->utsName.machine);

    distro_id->sysDescr[ sizeof(distro_id->sysDescr)-1 ] = 0;

    if ((distrolines = ReadFileLines(distroinfo)) == NULL)
        return distro_id;

    distro_id->Vendor = (unsigned char *)linux_distro_name;
    distro_id->LongDistro = malloc (strlen(distrolines->buf[0]) + 1);
    strncpy((char *)distro_id->LongDistro, 
	    (char *)distrolines->buf[0], strlen(distrolines->buf[0]) + 1);
    distro_id->Code =(unsigned char *)null_str;
    idx = 0;
    for ( str = distrolines->buf[0];; str = NULL )   {
        if (( tokens[idx++] = strtok(str, " ")) == NULL)
            break;
    }
    distro_id->Description =
        malloc( strlen(tokens[0]) +
                strlen(tokens[1]) +
                strlen(tokens[2]) +
                strlen(tokens[3]) + 4);
    sprintf((char *)distro_id->Description,"%s %s %s %s",
         tokens[0], tokens[1], tokens[2], tokens[3]);

    distro_id->Role = (unsigned char *)tokens[4];
    sscanf(distrolines->buf[1],"VERSION = %d", &distro_id->Version);
    if (distrolines->buf[2] != NULL) 
        sscanf(distrolines->buf[2], "PATCHLEVEL = %d", 
                &distro_id->SubVersion);
    else
        distro_id->SubVersion = 0;
    distro_id->VersionString = malloc(8);
    sprintf((char *)distro_id->VersionString, "%d SP%d", 
            distro_id->Version,
            distro_id->SubVersion);

    if (distrolines != NULL) {
        int i = 0;
        while (i < distrolines->count)
            free(distrolines->buf[i++]);
        if(distrolines->read_buf != NULL) 
            free(distrolines->read_buf);
        free(distrolines);
    }
    return distro_id;

}

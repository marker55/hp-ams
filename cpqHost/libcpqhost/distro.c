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
typedef enum
{
    rh_distro = 0,
    suse_distro,
    debian_distro,
    ubuntu_distro,
    unknown_distro,
} linux_distro_name_t;

/* text associated with linux_distro_name_t  */
static char *(linux_distro_name[]) =
{
    "Red Hat",
    "SuSE",
    "Debian",
    "Ubuntu",
    "unknown",
};

char *distroinfo[] = {
    "/etc/redhat-release",
    "/etc/SuSE-release",
    "/etc/debian-version",
    "/etc/lsb-release",
    0};

char *null_str = "";

distro_id_t  *getDistroInfo()
{
    int index, idx;
    char *tokens[20];
    read_line_t *distrolines = NULL;
    static distro_id_t *distro_id = NULL;
    int bits;


    if (distro_id != NULL)
        return distro_id;
    
    if ((distro_id = malloc(sizeof(distro_id_t))) == NULL)
        return NULL;

    distro_id->Bits = 32;

    uname(&distro_id->utsName);
    distro_id->KernelVersion = strdup((char *)&distro_id->utsName.release);
    bits = atoi(&distro_id->utsName.machine[strlen(distro_id->utsName.machine) - 2]);
    if (bits == 64) 
        distro_id->Bits = 64;

    snprintf(distro_id->sysDescr, sizeof(distro_id->sysDescr),
            "%s %s %s %s %s", distro_id->utsName.sysname,
            distro_id->utsName.nodename, distro_id->utsName.release, distro_id->utsName.version,
            distro_id->utsName.machine);
    distro_id->sysDescr[ sizeof(distro_id->sysDescr)-1 ] = 0;

    for (index = 0 ;distroinfo[index] != NULL;index++) {
        struct stat statbuf;
        if (stat(distroinfo[index],&statbuf) != 0 )
            continue;

        distrolines = ReadFileLines(distroinfo[index]);
        switch(index) {
            int version,subv;
            char *str;

        case rh_distro:
            distro_id->Vendor = (unsigned char *)linux_distro_name[rh_distro];
            distro_id->LongDistro = (unsigned char *)strdup(distrolines->line[0]);
            distro_id->Code = (unsigned char *)strtok(distrolines->line[0],"()");
            distro_id->Code = (unsigned char *)strtok(NULL,"()");
            idx = 0;
            for ( str = distrolines->line[0];; str = NULL )   {
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
            if ((distro_id->VersionString = malloc(strlen(tokens[6]))) != NULL)
                strcpy(distro_id->VersionString, tokens[6]);
            sscanf( tokens[6],"%d.%d", &version,&subv);
            distro_id->Version = version;
            distro_id->SubVersion = subv;

            if (distrolines != NULL) {
                if(distrolines->read_buf != NULL) 
                    free(distrolines->read_buf);
                free(distrolines);
            }
            return distro_id;

        case suse_distro:
            distro_id->Vendor = (unsigned char *)linux_distro_name[suse_distro];
            distro_id->LongDistro =(unsigned char *)strdup(distrolines->line[0]);
            distro_id->Vendor = (unsigned char *)linux_distro_name[suse_distro];
            distro_id->Code =(unsigned char *)null_str;
            idx = 0;
            for ( str = distrolines->line[0];; str = NULL )   {
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
            sscanf(distrolines->line[1],"VERSION = %d", &distro_id->Version);
            if (distrolines->line[2] != NULL) 
                sscanf(distrolines->line[1], "PATCHLEVEL = %d", 
                        &distro_id->SubVersion);
            else
                distro_id->SubVersion = 0;
            distro_id->VersionString = malloc(64);
            sprintf((char *)distro_id->VersionString, "%d SP%d", 
                    distro_id->Version,
                    distro_id->SubVersion);

            if (distrolines != NULL) {
                if(distrolines->read_buf != NULL) 
                    free(distrolines->read_buf);
                free(distrolines);
            }
            return distro_id;

        case debian_distro:
        case ubuntu_distro:
        case unknown_distro:
        default:
            if (distrolines != NULL) {
                if(distrolines->read_buf != NULL) 
                    free(distrolines->read_buf);
                free(distrolines);
            }
            return NULL;
        }
    }
    return NULL;
}

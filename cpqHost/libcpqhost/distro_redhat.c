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

/* text associated with linux_distro_name_t  */
static char *linux_distro_name[] = { 
    "Fedora",
    "Oracle",
    "Red Hat",
    "CentOS",
    NULL,
};
char *distroinfo[] = {
    "/etc/fedora-release",
    "/etc/enterprise-release",
    "/etc/redhat-release",
    NULL,
};

char *null_str = "";
struct stat statbuf;

distro_id_t  *getDistroInfo()
{
    int index, idx;
    char *tokens[20];
    read_line_t *distrolines = NULL;
    static distro_id_t *distro_id = NULL;
    int machine_len = 0;
    char *str;
    int description_len = 0;
    int i = 0;;
    int subv, version;
    char *Role = "Server";
    char *Code_start;
    char *Code_end;
    size_t Code_len;
    char *Dup = "";

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

    distro_id->sysDescr[ sizeof(distro_id->sysDescr) - 1 ] = 0;

    i = 0;
    while (distroinfo[i] != NULL) {
        if (stat(distroinfo[i], &statbuf) == 0 ) 
            if ((distrolines = ReadFileLines(distroinfo[i])) != NULL)
                break;
        i++;
    }
    if (distroinfo[i] == NULL)
        return NULL;

    if (distrolines == NULL)
        return NULL;

    Dup = strdup(distrolines->line[0]);

    if (strncmp((unsigned char *)linux_distro_name[i],
                distrolines->line[0],
                strlen(linux_distro_name[i]))) 
    i++;
        distro_id->Vendor = (unsigned char *)linux_distro_name[i];
    switch (i) {
    case 1:
        distro_id->cpqHostOsType = OS_TYPE_ORACLE;
        if (distro_id->Bits == 64) 
            distro_id->cpqHostOsType++;
        break;
    case 2:
        distro_id->cpqHostOsType = OS_TYPE_RHEL;
        if (distro_id->Bits == 64) 
            distro_id->cpqHostOsType++;
        break;
    case 3:
        distro_id->cpqHostOsType = OS_TYPE_CENTOS;
        if (distro_id->Bits == 64) 
            distro_id->cpqHostOsType++;
        break;
    case 0:
    default:
        distro_id->cpqHostOsType = OS_TYPE_LINUX;
        if (distro_id->Bits == 64) 
            distro_id->cpqHostOsType = OS_TYPE_LINUX_64;
    }

    distro_id->LongDistro = malloc (strlen(distrolines->line[0]) + 1);
    strncpy((char *)distro_id->LongDistro, (char *)distrolines->line[0], 
                                           strlen(distrolines->line[0]) + 1);

    Code_start = strtok((char *)Dup,"()") + 1;
    Code_end = strtok(NULL,"()");
    Code_len = Code_end - Code_start;

    if ((Code_len > 0) && (distro_id->Code = malloc(Code_len)) != NULL) {
        memset(distro_id->Code, 0, Code_len);
        strncpy((char *)distro_id->Code, Code_start, Code_len - 1);
    }

    free(Dup);
    Dup = strdup(distrolines->line[0]);

    idx = 0;
    for ( str = Dup;; str = NULL, idx++ )   {
        if (( tokens[idx] = strtok(str, " ")) == NULL)
            break;
    }

    /* find out how long our description string needs to be */
    for (index = 0; index < idx; index++) {
        if (strcmp(tokens[index], "release") == 0) 
            break;
        description_len += strlen(tokens[index]);
    }
    description_len += index + 1 ;

    if ((distro_id->Description = malloc(description_len)) != NULL) {
        memset(distro_id->Description, 0, description_len);
        description_len = 0;
    }
    for (i = 0; i < index; i++) {
        strcat((char *)distro_id->Description, tokens[i]);
        strcat((char *)distro_id->Description, " ");
    }

    if ((distro_id->VersionString = malloc(strlen(tokens[index + 1]) + 1)) 
                                                                      != NULL)
        strcpy((char *)distro_id->VersionString, tokens[index + 1]);

    sscanf( tokens[index + 1],"%d.%d", &version,&subv);
    distro_id->Version = version;
    distro_id->SubVersion = subv;

    distro_id->Role = malloc(strlen(Role) + 1);
    strcpy((char *)distro_id->Role, Role);

    free(Dup);
    if (distrolines != NULL) {
        if(distrolines->read_buf != NULL) 
            free(distrolines->read_buf);
        free(distrolines);
    }
    
    return distro_id;
}

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
static char *linux_distro_name[] = {"Ubuntu",
                                    "Debian GNU/Linux",
                                    0};

char *distroinfo[] = {"/etc/lsb-release",
                      "/etc/debian_version",
                      0};

char *null_str = "";

distro_id_t  *getDistroInfo()
{
    int i = 0;
    read_line_t *distrolines = NULL;
    static distro_id_t *distro_id = NULL;
    int machine_len = 0;
    int version, subv;
    char *str;
    char buffer[256];
    char * release_tag = "DISTRIB_RELEASE";
    char * distro_tag = "DISTRIB_DESCRIPTION";
    char * code_tag = "DISTRIB_CODENAME";

    if (distro_id != NULL)
        return distro_id;
    
    if ((distro_id = malloc(sizeof(distro_id_t))) == NULL)
        return NULL;
    memset(distro_id, 0, sizeof(distro_id_t));


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

    for (i = 0;  distroinfo[i] != NULL; i++) {
        if ((distrolines = ReadFileLines(distroinfo[i])) != NULL) {

            distro_id->Vendor = (unsigned char *)linux_distro_name[i];
            distro_id->Role = (unsigned char *)"Server";

            switch (i) {
                int idx = 0;
                int len = 0;
            case 0: /* Ubuntu */
            for (str = distrolines->buf[idx]; str != NULL; idx++ )   {
	        if (strncmp(str, release_tag, strlen(release_tag)) == 0) {
	            sscanf(str, "DISTRIB_RELEASE=%s", &buffer[0]);
	            distro_id->VersionString = malloc( strlen(buffer) + 1);
                    memset ( distro_id->VersionString, 0, strlen(buffer) + 1);
                    strncpy( distro_id->VersionString, buffer, strlen(buffer));

	            sscanf(str, "DISTRIB_RELEASE=%d.%d", &version, &subv);
	            distro_id->Version = version;
    	            distro_id->SubVersion = subv;
                } else 
	        if (strncmp(str, distro_tag, strlen(distro_tag)) == 0) {
	            char *item;
	            item = str + strlen(distro_tag) + 2;
	            distro_id->LongDistro = malloc( strlen(item));
                    memset ( distro_id->LongDistro, 0, strlen(item) );
	            strncpy( distro_id->LongDistro, item, strlen(item) - 1);
     
	            distro_id->Description = malloc( strlen(item));
                    memset ( distro_id->Description, 0, strlen(item));
	            strncpy( distro_id->Description, item, strlen(item) - 1);
                }
	        str = distrolines->buf[idx];
            }
	    goto cleanup;
            break;

            case 1: /* Debian */
                str = distrolines->buf[idx];
	        distro_id->VersionString = malloc( strlen(str) + 1);
                memset ( distro_id->VersionString, 0, strlen(str) + 1);
                strncpy( distro_id->VersionString, str, strlen(str));

                sscanf(str, "%d.0.%d", &version, &subv);
	        distro_id->Version = version;
    	        distro_id->SubVersion = subv;

                len = strlen((unsigned char *)linux_distro_name[i]) + 
                      strlen(str) + 2;
	        distro_id->LongDistro = malloc(len);
                memset (distro_id->LongDistro, 0, len);
	        strncpy(distro_id->LongDistro, 
                        (unsigned char *)linux_distro_name[i],
                        strlen((unsigned char *)linux_distro_name[i]));
                strcat(distro_id->LongDistro, " ");
                strcat(distro_id->LongDistro, distro_id->VersionString);

	        distro_id->Description = malloc(len);
                memset ( distro_id->Description, 0, len);
	        strcpy( distro_id->Description,  distro_id->LongDistro);
	    goto cleanup;
            break;

	    }
        }
    }
cleanup:
    if (distrolines != NULL) {
        if(distrolines->read_buf != NULL) 
            free(distrolines->read_buf);
        free(distrolines);
    }
    return distro_id;
}

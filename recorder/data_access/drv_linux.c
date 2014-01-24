#define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include "cpqHost/libcpqhost/cpqhost.h"

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/output_api.h>

#include "driver.h"

char * get_sysfs_str(char *);
extern drv_entry **drivers;

int getDriver()
{
    int i = 0;
    int modcount = 0;

    char kernel[256];
    char * offset = NULL;
    int modules_dep = -1;
    char * moddep_ptr = NULL;
    struct stat buf;
    int mapsize = 0;
    char buffer[1024];
    FILE* fp;
    /* need to get the modules.dep file */
    distro_id_t *myDistro = getDistroInfo();


    /* Let's get our blob */
    if ((fp =  fopen("/proc/modules","r")) == 0) {
        DEBUGMSGTL(("record:log", "Unable to open /proc/modules\n"));
        return 0;
    }

    while (fgets(buffer, sizeof(buffer), fp) > 0)
        modcount++;
    clearerr(fp);
    rewind(fp);

    if ((drivers = malloc(modcount * sizeof(drv_entry *))) == NULL) {
        DEBUGMSGTL(("record:log", "Unable to malloc() %ld bytes\n", 
                                modcount * sizeof(drv_entry)));
        fclose(fp);
        return 0;
    }

    strcpy(buffer, "/lib/modules/");
    strcat(buffer,  myDistro->KernelVersion);
    strcat(buffer, "/");
    strncpy(kernel, buffer, sizeof(kernel));
    offset = &kernel[strlen(kernel)];

    strcat(buffer, "modules.dep");
    if (stat(buffer, &buf) == 0 ) { 
        mapsize = buf.st_size;
        if ((modules_dep = open(buffer, O_RDONLY)) >=0)
            moddep_ptr = (char *)mmap(0, mapsize, PROT_READ, MAP_SHARED,
                                        modules_dep, 0);
    }

    while (fgets(buffer, sizeof(buffer), fp) > 0) {
        int j = 0;
        char module[81];
        char *found;
        char *begin;
        char *end;
        char sysfs[80];
        int timecount = 0;
        char * version;

        sscanf(buffer, "%s ", module);

        strcpy(buffer, "/");
        strcat(buffer, module);
        strcat(buffer, ".ko:" );
        found = strstr(moddep_ptr, buffer);
        if (found == NULL) {
            for (j = 0; buffer[j] != 0; j++)
                if (buffer[j] == '_')
                    buffer[j] = '-';
            found = strstr(moddep_ptr, buffer);
        }
        if (found != NULL) {
            if ((drivers[i] = malloc(sizeof(drv_entry))) == NULL) {
                continue;
            }
            memset(drivers[i], 0, sizeof(drv_entry));
            drivers[i]->name = malloc(strlen(module) + 1);
            memset(drivers[i]->name, 0, strlen(module) +1 ); 
            strncpy(drivers[i]->name, module, strlen(module));
            begin = found;
            for (begin = found;
                (*begin != 0x0a) && (begin >= moddep_ptr);
                 begin--) {
            }
            begin++;
            end = index(begin, ':');
            drivers[i]->filename = malloc((long) end - (long) begin + 1 );
            memset(drivers[i]->filename, 0, (long) end - (long) begin + 1 ); 
            strncpy(drivers[i]->filename, begin, (long) end - (long) begin);
            drivers[i]->timestamp = malloc(80);
            memset(drivers[i]->timestamp, 0, 80);
            strcpy(offset, drivers[i]->filename);
            if (stat(kernel, &buf) == 0 ) {
                timecount = strftime(drivers[i]->timestamp, 80, 
                                     "%Y-%m-%d %H:%M:%S %z",
                                     gmtime(&buf.st_mtime));
            } 

            memset(sysfs,0,sizeof(sysfs));
            strcpy(sysfs, "/sys/module/");
            strcat(sysfs, module);
            strcat(sysfs,"/version");
            if ((version = get_sysfs_str(sysfs)) == NULL ) {
                drivers[i]->version = malloc(16);
                memset(drivers[i]->version, 0, 16);
            } else
                drivers[i]->version = version;

            DEBUGMSGTL(("record:log", "i = %d ", i));
            DEBUGMSGTL(("record:log", "module = %s ", drivers[i]->name));
            DEBUGMSGTL(("record:log", "filename = %s ", drivers[i]->filename));
            DEBUGMSGTL(("record:log", "timestamp = %s \n", drivers[i]->timestamp));
            DEBUGMSGTL(("record:log", "version = %s\n", drivers[i]->version));

            i++;
        }
    }
    fclose(fp);

    if (moddep_ptr != NULL) {
        munmap(moddep_ptr, mapsize);
        close(modules_dep);
    }
    return i;
}


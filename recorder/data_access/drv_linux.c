#define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>

#include "cpqHost/libcpqhost/cpqhost.h"

#include "net-snmp/net-snmp-config.h"
#include "net-snmp/library/snmp_impl.h"
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/output_api.h>
#include "driver.h"

extern int pci_select(const struct dirent *);

char * get_sysfs_str(char *);
extern drv_entry **drivers;
static char * moddep_ptr = NULL;
    char kernel[256];

int LoadModDep() {
    int modules_dep = -1;
    char * offset = NULL;
    struct stat buf;
    int mapsize = 0;
    char buffer[1024];
    static distro_id_t *myDistro = NULL;

    if (myDistro == NULL)
        myDistro = getDistroInfo();

    strcpy(buffer, "/lib/modules/");
    strncat(buffer,  myDistro->KernelVersion, 
            sizeof(buffer) - strlen(buffer) - 1);
    strcat(buffer, "/");
    strncpy(kernel, buffer, sizeof(kernel) - strlen(kernel) - 1);
    offset = &kernel[strlen(kernel)];

    strcat(buffer, "modules.dep");
    if (stat(buffer, &buf) != 0 ) {
        return 0;
    } else {
        mapsize = buf.st_size;
        if ((modules_dep = open(buffer, O_RDONLY)) >=0) 
            moddep_ptr = (char *)mmap(0, mapsize, PROT_READ, MAP_SHARED,
                                        modules_dep, 0);
        else {
            return 0;
        }
    }
    return 1;
}

drv_entry* getDriver(char *module)
{
    drv_entry *driver;

    struct dirent **devlist;
    int pcicount;
    int k = 0;
    int j = 0;
    char buffer[1024];
    char *found;
    char *begin;
    char *end;
    char sysfs[80];
    int timecount = 0;
    char * version;
    struct stat buf;
    int dsize = 0;

    /* need to get the modules.dep file */

    DEBUGMSGTL(("rec:logdrvdata", "getDriver(): Looking for module = %s\n", module));
    if (moddep_ptr == NULL)
        if (LoadModDep() == 0)
            return NULL;

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
        if ((driver = malloc(sizeof(drv_entry))) == NULL) 
            return NULL;
        memset(driver, 0, sizeof(drv_entry));

        if ((driver->name = malloc(strlen(module) + 1)) == NULL)
            return NULL;
        memset(driver->name, 0, strlen(module) + 1 );
        strncpy(driver->name, module, strlen(module));

        begin = found;
        for (begin = found;
            (*begin != 0x0a) && (begin >= moddep_ptr);
             begin--) {
        }
        begin++;
        end = index(begin, ':');
        driver->filename = malloc((long) end - (long) begin + 1 );
        memset(driver->filename, 0, (long) end - (long) begin + 1 );
        strncpy(driver->filename, begin, (long) end - (long) begin);
        driver->timestamp = malloc(80);
        memset(driver->timestamp, 0, 80);
        if (stat(kernel, &buf) == 0 ) {
            timecount = strftime(driver->timestamp, 80,
                                 "%Y-%m-%d %H:%M:%S %z",
                                 gmtime(&buf.st_mtime));
        }

        memset(sysfs,0,sizeof(sysfs));
        strcpy(sysfs, "/sys/module/");
        strcat(sysfs, module);
        strcat(sysfs,"/version");
        if ((version = get_sysfs_str(sysfs)) == NULL ) {
            driver->version = malloc(16);
            memset(driver->version, 0, 16);
        } else
            driver->version = version;

        memset(sysfs,0,sizeof(sysfs));
        strcpy(sysfs, "/sys/bus/pci/drivers/");
        strcat(sysfs, module);

        /* Let's get our blob */
        pcicount = scandir(sysfs, &devlist, pci_select, alphasort);
        if (pcicount > 0)
            dsize = pcicount*16;
        else
            dsize = 16;
        driver->pci_devices = malloc(dsize);
        memset(driver->pci_devices, 0, dsize );
        for (k = 0; k <pcicount; k++ ) {
            if (k != 0)
                strcat(driver->pci_devices, " ");
            strcat(driver->pci_devices, devlist[k]->d_name);
            free(devlist[k]);
        }
        if (pcicount > 0) free(devlist);

        return driver;
    }
    return NULL;
}


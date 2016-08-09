#define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include "cpqHost/libcpqhost/cpqhost.h"
#include "ams_rec.h"
#include "recorder.h"
#include "data.h"
#include "data_access/driver.h"

/* routines needed for debug output */
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/output_api.h>

int rec_init();
int doCodeDescriptor(unsigned int, unsigned int, unsigned char, unsigned char,
                     unsigned char, unsigned char, unsigned char, char *);

extern drv_entry* getDriver(char *);

static field fld[]= {
    {REC_SIZE_WORD,   REC_TYP_NUMERIC, "Flags"},
    {REC_BINARY_DATA, REC_TYP_TEXT,    "Name"},
    {REC_BINARY_DATA, REC_TYP_TEXT,    "Filename"},
    {REC_BINARY_DATA, REC_TYP_TEXT,    "Version"},
    {REC_BINARY_DATA, REC_TYP_TEXT,    "Timestamp"},
    {REC_BINARY_DATA, REC_TYP_TEXT,    "Vendor"},
    {REC_BINARY_DATA, REC_TYP_TEXT,    "PCI Devices"},

};

int getDriverCount()
{
    FILE* fp;
    int modcount = 0;
    char buffer[1024];
    /* Let's get our blob */
    if ((fp =  fopen("/proc/modules","r")) == 0) {
        DEBUGMSGTL(("rec:logdrvdata", "Unable to open /proc/modules\n"));
        return 0;
    }

    while (fgets(buffer, sizeof(buffer), fp) > 0)
        modcount++;
    clearerr(fp);
    fclose(fp);

    return modcount;
}


int LOG_DRIVER_SETUP(int flag)
{
    int i = 0;
    int rc;
    int num_descript;
    size_t toc_sz = sizeof(RECORDER_API4_RECORD);;
    RECORDER_API4_RECORD *toc;
    unsigned char code_descr = REC_CODE_AMS_DRIVER;
    unsigned char *code_descr_str = REC_CODE_AMS_DRIVER_STR;
    
    if (flag) {
        code_descr = REC_CODE_AMS_DRIVER_CHANGE;
        code_descr_str = REC_CODE_AMS_DRIVER_CHANGE_STR;
    }

    if (!rec_init() )
        return 0;

   /* do Code Descriptor */
    if ((rc = doCodeDescriptor(s_ams_rec_handle,
                               s_ams_rec_class,
                               code_descr,
                               0,
                               REC_SEV_CONFIG,
                               REC_BINARY_DATA,
                               REC_VIS_CUSTOMER,
                               code_descr_str)) != RECORDER_OK) {
        DEBUGMSGTL(("rec:rec_log_driver","Driver register descriptor failed %d\n", rc));
        return 0;
    }

    /* build descriptor toc */
    num_descript = sizeof(fld)/sizeof(field);
    DEBUGMSGTL(("rec:logdrv","Driver num_descript = %d\n", num_descript));
    if ((toc_sz *= num_descript) > RECORDER_MAX_BLOB_SIZE) {
        DEBUGMSGTL(("rec:logdrv",
                    "Driver Descriptor too large %ld\n", toc_sz));
        return 0;
    }
    DEBUGMSGTL(("rec:logdrv","Driver toc_sz = %ld\n", toc_sz));
    if ((toc = malloc(toc_sz)) == NULL) {
        DEBUGMSGTL(("rec:logdrv",
                    "Driver Unable to malloc() %ld bytes\n", toc_sz));
        return 0;
    }

    /* now do the field descriptor */
    memset(toc, 0, toc_sz);
    for ( i = 0; i < num_descript; i++) {
        toc[i].flags = REC_FLAGS_API4 | REC_FLAGS_DESC_FIELD;
        toc[i].classs = s_ams_rec_class;
        toc[i].code = code_descr;
        toc[i].field = i;
        toc[i].severity_type = REC_DESC_SEVERITY_CONFIGURATION | fld[i].tp;
        toc[i].format = fld[i].sz;
        toc[i].visibility =  REC_DESC_VISIBILITY_CUSTOMER;
        strncpy(toc[i].desc, fld[i].nm, strlen(fld[i].nm));
        DEBUGMSGTL(("rec:logdrv", "toc[i] = %p\n", &toc[i]));
        dump_chunk("rec:rec_log", "descriptor", (const u_char *)&toc[i], 96);
    }

    dump_chunk("rec:rec_log", "toc", (const u_char *)toc, toc_sz);

    if ((rc = rec_api4_field(s_ams_rec_handle, toc_sz, toc))
            != RECORDER_OK) {
        DEBUGMSGTL(("rec:logdrv","Driver register descriptor failed %d\n", rc));
        return 0;
    }
    free(toc);
    return 1;
}

void LOG_DRIVER( char *module, unsigned short flags, unsigned char code)
{
    int rc;
    size_t blob_sz = sizeof(REC_AMS_DriverData);
    REC_AMS_DriverData *DriverData;
    size_t string_list_sz = 0;
    drv_entry *driver;
    char *blob;
    char *end;

    struct timeval the_time[2];

    init_etime(&the_time[0]);

    DEBUGMSGTL(("rec:logdrvdata", "LOG_DRIVER(): module = %s\n", module));

    if ((driver = getDriver(module)) == NULL)
        return;
        
    blob_sz = sizeof(REC_AMS_DriverData);
    string_list_sz = strlen(driver->name) +
                    strlen(driver->filename) +
                    strlen(driver->version) +
                    strlen(driver->timestamp) + 
                    strlen(driver->pci_devices) +
                    6; /* there are 6 eols including the Null Vendor */
    if ((blob_sz += string_list_sz) > RECORDER_MAX_BLOB_SIZE) {
        DEBUGMSGTL(("rec:logdrv",
                    "Driver Data too large %ld\n", blob_sz));
        goto free;
    }
    if ((blob = malloc(blob_sz)) == NULL ) {
        DEBUGMSGTL(("rec:logdrv",
                    "Driver Unable to malloc() %ld blob\n", blob_sz));
        goto free;
    }
    memset(blob, 0, blob_sz);

    /* Got the Blob, so let's stuff in the data */
    DriverData = (REC_AMS_DriverData *)blob;

    end = blob + sizeof(REC_AMS_DriverData);
    strcpy(end, driver->name);
    end += strlen(driver->name) + 1;

    strcpy(end, driver->filename);
    end += strlen(driver->filename) + 1;

    strcpy(end, driver->version);
    end += strlen(driver->version) + 1;

    strcpy(end, driver->timestamp);
    /* Skip over Vendor field */
    end += strlen(driver->timestamp) + 2;

    strcpy(end, driver->pci_devices);

    DEBUGMSGTL(("rec:logdrv", "Module name %s, Driver filename  %s\n",
                driver->name, driver->filename));
    DEBUGMSGTL(("rec:logdrv", 
                "Module version %s, Driver timestamp  %s\n",
                driver->version, driver->timestamp));
    DEBUGMSGTL(("rec:logdrv","PCI devices %s\n", 
                driver->pci_devices));

    DEBUGMSG(("rec_drv:timer", "Setup time for Log Driver %s %dms\n", driver->name, get_etime(&the_time[0])));
    DriverData->Flags = flags;
    // Log the record
    if ((rc = rec_api6(s_ams_rec_handle, (const char*)blob, blob_sz)) !=
                    RECORDER_OK) {
        DEBUGMSGTL(("rec:logdrv", "LogRecorderData failed (%d)\n",rc));
    }

    DEBUGMSG(("rec_drv:timer", "Log time for Log Driver %s %dms\n", driver->name, get_etime(&the_time[0])));
    DEBUGMSGTL(("rec:logdrv", "Logged record for code %d\n",
                REC_CODE_AMS_DRIVER));
free:
    free(blob);
    if (driver->pci_devices != NULL) free(driver->pci_devices);
    if (driver->timestamp != NULL) free(driver->timestamp);
    if (driver->version != NULL) free(driver->version);
    if (driver->filename != NULL) free(driver->filename);
    if (driver->name != NULL) free(driver->name);
    free(driver);
}

void rec_driver_load_dynamic(char *devpath, char *devname, void *data)
{
    int rc;
    unsigned short flags = REC_AMS_Driver_Load;
    DEBUGMSGTL(("rec:logdrvdata", "Dynamic Load module devpath = %s, devname = %s\n", devpath, devpath + 8));

    /* Let's go ahead and set the code for */
    if ((rc = rec_api3(s_ams_rec_handle, REC_CODE_AMS_DRIVER_CHANGE)) != RECORDER_OK) {
        DEBUGMSGTL(("rec:logdrv", "SetRecorderFeederCode failed (%d)\n", rc));
        return;
    }

    LOG_DRIVER(devpath + 8, flags, REC_CODE_AMS_DRIVER_CHANGE);
}

void rec_driver_unload_dynamic(char *devpath, char *devname, void *data)
{
    int rc;
    unsigned short flags = REC_AMS_Driver_Unload;
    DEBUGMSGTL(("rec:logdrvdata", "Dynamic Unload module devpath = %s, devname = %s\n", devpath, devpath + 8));

    /* Let's go ahead and set the code for */
    if ((rc = rec_api3(s_ams_rec_handle, REC_CODE_AMS_DRIVER_CHANGE)) != RECORDER_OK) {
        DEBUGMSGTL(("rec:logdrv", "SetRecorderFeederCode failed (%d)\n", rc));
        return;
    }

    LOG_DRIVER(devpath + 8, flags, REC_CODE_AMS_DRIVER_CHANGE);
}

void LOG_DRIVERS(
        /*REC_AMS_OsType Type,
          REC_AMS_OsVendor Vendor,
          UINT16 Bits,
          const char* NameString,
          const char* VersionString,
          const UINT16 Version[4],
          UINT16 ServicePack*/)
{
    int rc;
    int drvcount, i;
    char buffer[1024];
    unsigned short flags;
    FILE* fp;
    struct timeval the_time[2];

    init_etime(&the_time[0]);

    if (!LOG_DRIVER_SETUP(0))
        return;
    /* Let's get our blob */
    drvcount = getDriverCount();

    DEBUGMSGTL(("rec:logdrvdata", "Driver Count = %d\n", drvcount));
    if ((fp =  fopen("/proc/modules","r")) == 0) {
        DEBUGMSGTL(("rec:logdrvdata", "Unable to open /proc/modules\n"));
        return;
    }

    /* Let's go ahead and set the code for */
    if ((rc = rec_api3(s_ams_rec_handle, REC_CODE_AMS_DRIVER)) != RECORDER_OK) {
        DEBUGMSGTL(("rec:logdrv", "SetRecorderFeederCode failed (%d)\n", rc));
        return;
    }

    DEBUGMSG(("rec_drv:timer", "Setup time for Log Drivers %dms\n", get_etime(&the_time[0])));

    for (i = 0; i < drvcount; i++ ) {
        char * end;
        memset(buffer, 0, sizeof(buffer));

        if (fgets(buffer, sizeof(buffer), fp) > 0) {
            DEBUGMSGTL(("rec:logdrvdata", "buffer = %s\n", buffer));
            if ((end = index(buffer, ' ')) == NULL)
                continue;

            *end = 0;
        
            DEBUGMSGTL(("rec:logdrvdata", "module = %s\n", buffer));
            flags = REC_AMS_Driver_Load;
    
            if (i == 0)
                flags = REC_AMS_Driver_First | REC_AMS_Driver_Load;
            if (i == (drvcount - 1))
                flags |= REC_AMS_Driver_Last | REC_AMS_Driver_Load;
            //
            // Log the record
            LOG_DRIVER(buffer, flags, REC_CODE_AMS_DRIVER);
            DEBUGMSG(("rec_drv:timer", "Interval time for Log Driver %s %dms\n", buffer, get_etime(&the_time[0])));
        }
    }
    udev_register("module","add", rec_driver_load_dynamic, NULL);
    udev_register("module","remove", rec_driver_unload_dynamic, NULL);
    LOG_DRIVER_SETUP(1);
}


#ifndef REC_DRIVER_H
#define REC_DRIVER_H
typedef struct _drv_entry {
    char *name;
    char *filename;
    char *version;
    char *timestamp;
    char *vendor;
    char *pci_devices;
} drv_entry;

#endif

typedef struct _vpd_node{
    unsigned char    tag[2];
    unsigned char   *data;
    void*   next;
    void*   prev;
} vpd_node;

/* Structure to map PCI vendor ID to name */
typedef struct _vendor_node {
    unsigned short vendor_id;
    unsigned char *name;
    void *  next;
    void *  prev;
} vendor_node;

/* Structure to map PCI vendor ID/Device ID to Device name */
typedef struct _device_node {
    unsigned short vendor_id;
    unsigned short device_id;
    unsigned char *name;
    void *  next;
    void *  prev;
} device_node;

/* Structure to map PCI vendor ID/Device ID to name 
   sub-vendor ID/sub-device ID to sub-vendor and sub-device name */
typedef struct _subsystem_node {
    unsigned short vendor_id;
    unsigned short device_id;
    unsigned short  subvendor_id;
    unsigned short  subdevice_id;
    unsigned char *vname;
    unsigned char *name;
    void *  next;
    void *  prev;
} subsystem_node;

typedef struct _class_node {
    unsigned int device_class;
    unsigned char *prog_if_name;
    unsigned char *name;
    unsigned char *super;
    void *  next;
    void *  prev;
} class_node;

typedef struct _pci_node {
    unsigned char   sysdev[16];
    unsigned short  domain;
    unsigned char   bus;
    unsigned char   dev;
    unsigned char   func;
    unsigned short  vendor_id;
    unsigned short  device_id;
    unsigned short  subvendor_id;
    unsigned short  subdevice_id;
    unsigned int    device_class;
    unsigned short  status;
    unsigned short command;
    unsigned int    caps;
    unsigned char   revision;
    unsigned char   linkwidth;
    unsigned char  *config;
    vendor_node    *vendor;
    device_node    *device;
    subsystem_node *subsystem;
    class_node     *pciclass;
    unsigned char  *vpd;
    unsigned char  *vpd_productname;
    vpd_node       *vpd_data;

    int             irq;
    void  *      pci_next;
    void  *      pci_prev;
    void  *      pci_sub;
    void  *      pci_top;
    char  *      pci_name;
} pci_node;



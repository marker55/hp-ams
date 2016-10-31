#ifndef _NL_UDEV_H
#define _NL_UDEV_H

#define UDEV_MONITOR_KERNEL 1
#define UDEV_MONITOR_UDEV   2

#define SUBSYSTEM_SZ 64
#define ACTION_SZ 16
#define DEVTYPE_SZ 48
struct udev_callback_header {
    char subsystem[SUBSYSTEM_SZ];
    char action[ACTION_SZ];
    char devtype[DEVTYPE_SZ];
    void (* ss_callback)(char *, char*, char *, void *);
    void * ss_container;
    void *hdr_prev;
    void *hdr_next;
};
int udev_register(char *subsystem,
                  char *action,
                  char *devtype,
                  void (*func) (char *, char*, char *, void *),
                  void *container);

#endif

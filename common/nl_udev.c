#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <net-snmp/library/container.h>
#include <net-snmp/library/snmp_debug.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/errno.h>
#include <getopt.h>
#include <linux/socket.h>
#include <linux/version.h>

#include <linux/netlink.h>

#include "nl_udev.h"
struct udev_callback_header *udev_cb = NULL;

static void udev_event_process(int fd, void *data) {
    struct msghdr sock_msg;
    struct iovec iov;
    char cred_msg[CMSG_SPACE(sizeof(struct ucred))];
 
    int                buflen, bufpos;
    char               buf[16384];
    char *action = NULL;
    char *subsystem = NULL;
    char *devpath = NULL;
    char *devname = NULL;
    char *devtype = NULL;

    struct udev_callback_header *tmp_cb;

    DEBUGMSGTL(("netlink:udev", "udev_event_process()\n"));
    memset(buf, 0x00, sizeof(buf));
    iov.iov_base = &buf;
    iov.iov_len = sizeof(buf);
    memset (&sock_msg, 0x00, sizeof(struct msghdr));
    sock_msg.msg_iov = &iov;
    sock_msg.msg_iovlen = 1;
    sock_msg.msg_control = cred_msg;
    sock_msg.msg_controllen = sizeof(cred_msg);

    buflen = recvmsg(fd, &sock_msg, 0);
    DEBUGMSGTL(("netlink:udev", "recvmsg()\n"));
    if (buflen < 0) {
        if (errno != EINTR)
            snmp_log(LOG_ERR,"udev_event_process: Receive failed.\n");
        return;
    }

    /* if we haven't registered any callbacks, no need to process */
    if (udev_cb == NULL)
        return;

    if (buflen < 32 || (size_t)buflen >= sizeof(buf)) {
        snmp_log(LOG_ERR, "invalid message length\n");
        return;
    }

    /* kernel message with header */
    bufpos = strlen(buf) + 1;
    if ((size_t)bufpos < sizeof("a@/d") || bufpos >= buflen) {
        snmp_log(LOG_ERR, "invalid message length\n");
        return;
    }

    /* check message header */
    if (strstr(buf, "@/") == NULL) {
        snmp_log(LOG_ERR, "unrecognized message header\n");
        return ;
    }

    while (bufpos < buflen) {
        char *key;
        size_t keylen;

        key = &buf[bufpos];
        if (!strncmp(key, "ACTION=", 7)) 
             action = key + 7;
        else if (!strncmp(key, "DEVTYPE=", 8))
             devtype = key + 8;
        else if (!strncmp(key, "DEVPATH=", 8))
             devpath = key + 8;
        else if (!strncmp(key, "DEVNAME=", 8))
             devname = key + 8;
        else if (!strncmp(key, "SUBSYSTEM=", 10))
             subsystem = key + 10;
        
        keylen = strlen(key);
        if (keylen == 0)
            break;
        bufpos += keylen + 1;
    }
    DEBUGMSGTL(("netlink:udev", "ACTION = %s, DEVTYPE = %s, DEVPATH = %s, DEVNAME = %s, SUBSYSTEM=%s\n", action, devtype, devpath, devname, subsystem));
  
    tmp_cb = udev_cb;
    do {
        if (subsystem && (!strcmp(subsystem, tmp_cb->subsystem)))  {
            if (action && strstr(tmp_cb->action, action)) {
                if ((tmp_cb->devtype == NULL) || 
                    (*tmp_cb->devtype == 0) ||
                    (devtype && strstr(tmp_cb->devtype, devtype))) {
                DEBUGMSGTL(("netlink:udev", 
                                "calling %s callback for %s on %s for %s\n", 
                                action, subsystem, devpath, devtype));
                    tmp_cb->ss_callback(devpath, devname, devtype, tmp_cb->ss_container);
                } 
            }
        }
        tmp_cb = tmp_cb->hdr_next;
    } while (tmp_cb != NULL);
}

/* taken from netlink patch for if-mib */
static int udev_netlink_init(__u32 nlgroup)
{
    struct sockaddr_nl localaddrinfo;
    int fd;
    DEBUGMSGTL(("netlink:udev", "udev_netlink_init()\n"));
    fd = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
    if (fd < 0) {
       snmp_log(LOG_ERR, "udev_netlink_init: Cannot create socket.\n");
        return -1;
    }

    memset(&localaddrinfo, 0, sizeof(struct sockaddr_nl));

    localaddrinfo.nl_family = AF_NETLINK;
    localaddrinfo.nl_groups = nlgroup;

    if (bind(fd, (struct sockaddr*)&localaddrinfo, sizeof(localaddrinfo)) < 0) {
        snmp_log(LOG_ERR,"udev_netlink_init: Bind failed.\n");
        close(fd);
        return -1;
    }

    DEBUGMSGTL(("netlink:udev", "Bind succeeded\n"));
    if (register_readfd(fd, udev_event_process, udev_cb) != 0) {
        snmp_log(LOG_ERR,
                 "udev_netlink_init: error registering netlink socket\n");
        close(fd);
        return -1;
    }
    return 0;
}

int udev_register(char *subsystem, 
                  char *action, 
                  char *devtype,
                  void (*func) (char *, char*, char *, void *), 
                  void *container) 
{
    struct udev_callback_header *my_cb;

    if (udev_cb == NULL) 
        udev_netlink_init(UDEV_MONITOR_KERNEL);

    if ((my_cb = malloc(sizeof(struct udev_callback_header))) == NULL) {
        snmp_log(LOG_ERR, "udev_netlink_init: Cannot malloc.\n");
        return -1;
    }
 
    memset(my_cb, 0, sizeof(struct udev_callback_header));
    my_cb->ss_callback = func;
    my_cb->ss_container = container;
    strncpy(my_cb->subsystem, subsystem, SUBSYSTEM_SZ - 1);
    if (action != NULL)
        strncpy(my_cb->action, action, ACTION_SZ - 1);
    if (devtype != NULL)
        strncpy(my_cb->devtype, devtype, DEVTYPE_SZ - 1);
    if (udev_cb == NULL) 
        udev_cb = my_cb;
    else {
        struct udev_callback_header *tmp_cb;
        tmp_cb = udev_cb;
        while (tmp_cb->hdr_next != NULL) 
            tmp_cb = tmp_cb->hdr_next;
        tmp_cb->hdr_next = my_cb;
    }
    return 1;
}

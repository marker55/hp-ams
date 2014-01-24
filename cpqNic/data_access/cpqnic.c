#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/library/snmp_debug.h>

#include <stdlib.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <string.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <linux/sockios.h>
#include <linux/types.h>
typedef __u64 u64;
typedef __u32 u32;
typedef __u16 u16;
typedef __u8 u8;
#include <linux/ethtool.h>

#include "cpqnic.h"

link_status_t get_logical_interface_status(char *if_name)
{
    struct ifreq ifr;
    int32_t sock = -1;


    if (sock < 0) {
        sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
        if (sock < 0) {
            DEBUGMSGTL(("cpqNic:ethtool", "socket() failed: errno=%d (%s)\n",
                        errno, strerror(errno)));
            return(link_unknown);
        }
    }

    ifr.ifr_data = (char*)0;

    strcpy(ifr.ifr_name, if_name);

    /* get interface up/down status */
    if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0) {
        if (errno != ENODEV) {
            DEBUGMSGTL(("cpqNic:ethtool",
                        "INFO: ioctl(SIOCGIFFLAGS) for %s: errno=%d, %s\n",
                        if_name, errno, strerror(errno)));
        } else {
            DEBUGMSGTL(("cpqNic:ethtool", "INFO: ioctl(SIOCGIFFLAGS) for %s: errno=%d, %s\n",
                        if_name, errno, strerror(errno)));
        }
        close(sock);
        return(link_unknown);
    }

    close(sock);
    return((ifr.ifr_flags & IFF_UP)?link_up:link_down);
}

void free_dynamic_interface_info(dynamic_interface_info *dyn_if_info)
{
    if (dyn_if_info->ip_addr) {
        free(dyn_if_info->ip_addr);
        dyn_if_info->ip_addr =  NULL;
    }
    if (dyn_if_info->netmask) {
        free(dyn_if_info->netmask);
        dyn_if_info->netmask =  NULL;
    }
    if (dyn_if_info->broadcast) {
        free(dyn_if_info->broadcast);
        dyn_if_info->broadcast=  NULL;
    }
    if (dyn_if_info->current_mac) {
        free(dyn_if_info->current_mac);
        dyn_if_info->current_mac =  NULL;
    }
    if (dyn_if_info->ipv6_addr) {
        free(dyn_if_info->ipv6_addr);
        dyn_if_info->ipv6_addr = NULL;
    }
    if (dyn_if_info->ipv6_shorthand_addr) {
        free(dyn_if_info->ipv6_shorthand_addr);
        dyn_if_info->ipv6_shorthand_addr = NULL;
    }
    return;
}


int32_t get_dynamic_interface_info(char* if_name,
        dynamic_interface_info *dyn_if_info)
{
    int32_t rc = 0;
    struct ifreq ifr;
    int32_t sock = -1;
    FILE *pf;
    char ipv6_addr[32];
    char ipv6_ifname[IF_NAMESIZE];
    int32_t ipv6_if_index;
    int32_t ipv6_prefix_len;  // see inet6_ifaddr in if_inet6.h kernel header
    int32_t ipv6_scope;
    int32_t ipv6_dad_status;
    struct sockaddr_in6 ipv6_sock;


    /* null all members in struct */
    memset(dyn_if_info, 0, sizeof(dynamic_interface_info));

    if (sock < 0) {
        sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
        if (sock < 0) {
            DEBUGMSGTL(("cpqNic:ethtool", "socket() failed: errno=%d (%s)\n", errno, strerror(errno)));
            /*/ \todo investigate if we will always be able to get
              an AF_INET socket, even if we only have and IPv6 interface. */
            return(-1);
        }
    }

    ifr.ifr_data = (char*)0;
    ifr.ifr_addr.sa_family = AF_INET;
    strcpy(ifr.ifr_name, if_name);

    /*
     * get the current IP address
     */
    if (ioctl(sock, SIOCGIFADDR, &ifr) < 0) {
        if ((errno != ENODEV) && (errno != EADDRNOTAVAIL)) {
            DEBUGMSGTL(("cpqNic:ethtool",
                        "INFO: ioctl(SIOCGIFADDR) for %s: errno=%d, %s\n",
                        if_name, errno, strerror(errno)));
            rc = 1;
        } else {
            DEBUGMSGTL(("cpqNic:ethtool", "INFO: ioctl(SIOCGIFADDR) for %s: errno=%d, %s\n",
                        if_name, errno, strerror(errno)));
        }
    } else {
        /* each digit requires up to 4 ascii chars (eg 255 == "255.")
           plus 1 for null */
        dyn_if_info->ip_addr = (char*) malloc(sizeof(ifr.ifr_addr.sa_data) * 4 + 1);
        if (!(dyn_if_info->ip_addr)) {
            DEBUGMSGTL(("cpqNic:ethtool",
                        "ERROR: malloc failed (errno=%d), %s,%dn",
                        errno, __FILE__, __LINE__));
            abort()  ;
        }

        sprintf(dyn_if_info->ip_addr,
                "%d.%d.%d.%d",
                (ifr.ifr_addr.sa_data[2]&0xFF),
                (ifr.ifr_addr.sa_data[3]&0xFF),
                (ifr.ifr_addr.sa_data[4]&0xFF),
                (ifr.ifr_addr.sa_data[5]&0xFF));


        /*
         * get the current netmask
         */
        if (ioctl(sock, SIOCGIFNETMASK, &ifr) < 0) {
            if ((errno != ENODEV) && (errno != EADDRNOTAVAIL)) {
                DEBUGMSGTL(("cpqNic:ethtool",
                            "INFO: ioctl(SIOCGIFNETMASK) for %s: errno=%d, %s\n",
                            if_name, errno, strerror(errno)));
                rc = 1;
            } else {
                DEBUGMSGTL(("cpqNic:ethtool", "INFO: ioctl(SIOCGIFNETMASK) for %s: errno=%d, %s\n",
                            if_name, errno, strerror(errno)));
            }
        } else {
            /* each digit requires up to 4 ascii chars (eg 255 == "255.")
               plus 1 for null */
            dyn_if_info->netmask = (char*) malloc(sizeof(ifr.ifr_addr.sa_data) * 4 + 1);
            if (!(dyn_if_info->netmask)) {
                DEBUGMSGTL(("cpqNic:ethtool",
                            "ERROR: malloc failed (errno=%d), %s,%dn",
                            errno, __FILE__, __LINE__));
                abort()  ;
            }

            sprintf(dyn_if_info->netmask, "%d.%d.%d.%d",
                    (ifr.ifr_addr.sa_data[2]&0xFF),
                    (ifr.ifr_addr.sa_data[3]&0xFF),
                    (ifr.ifr_addr.sa_data[4]&0xFF),
                    (ifr.ifr_addr.sa_data[5]&0xFF));
        }


        /*
         * get the current broadcast address
         */
        if (ioctl(sock, SIOCGIFBRDADDR, &ifr) < 0) {
            if ((errno != ENODEV) && (errno != EADDRNOTAVAIL)) {
                DEBUGMSGTL(("cpqNic:ethtool",
                            "INFO: ioctl(SIOCGIFBRDADDR) for %s: errno=%d, %s\n",
                            if_name, errno, strerror(errno)));
                rc = 1;
            } else {
                DEBUGMSGTL(("cpqNic:ethtool", "INFO: ioctl(SIOCGIFBRDADDR) for %s: errno=%d, %s\n",
                            if_name, errno, strerror(errno)));
            }
        } else {
            /* each digit requires up to 4 ascii chars (eg 255 == "255.")
               plus 1 for null */
            dyn_if_info->broadcast = (char*) malloc(sizeof(ifr.ifr_addr.sa_data) * 4 + 1);
            if (!(dyn_if_info->broadcast)) {
                DEBUGMSGTL(("cpqNic:ethtool",
                            "ERROR: malloc failed (errno=%d), %s,%dn",
                            errno, __FILE__, __LINE__));
                abort()  ;
            }

            sprintf(dyn_if_info->broadcast,
                    "%d.%d.%d.%d",
                    (ifr.ifr_addr.sa_data[2]&0xFF),
                    (ifr.ifr_addr.sa_data[3]&0xFF),
                    (ifr.ifr_addr.sa_data[4]&0xFF),
                    (ifr.ifr_addr.sa_data[5]&0xFF));
        }

    }

    /*
       get the current MAC address
       */
    if (ioctl(sock, SIOCGIFHWADDR, &ifr) < 0) {
        if (errno != ENODEV) {
            DEBUGMSGTL(("cpqNic:ethtool",
                        "ERROR: ioctl(SIOCGIFHWADDR) for %s: errno=%d, %s\n",
                        if_name, errno, strerror(errno)));
            rc = 1;
        } else {
            DEBUGMSGTL(("cpqNic:ethtool", "INFO: ioctl(SIOCGIFHWADDR) for %s: errno=%d, %s\n",
                        if_name, errno, strerror(errno)));
        }
    } else {
        /* each hex digit requires 3 ascii chars (eg FF == "FF:")
           plus 1 for null */
        dyn_if_info->current_mac =
            (char*) malloc(sizeof(ifr.ifr_hwaddr.sa_data) * 3 + 1);
        if (!(dyn_if_info->current_mac)) {
            DEBUGMSGTL(("cpqNic:ethtool",
                        "ERROR: malloc failed (errno=%d), %s,%dn",
                        errno, __FILE__, __LINE__));
            abort()  ;
        }

        sprintf(dyn_if_info->current_mac,
                "%.2X:%.2X:%.2X:%.2X:%.02X:%.2X",
                (ifr.ifr_hwaddr.sa_data[0]&0xFF),
                (ifr.ifr_hwaddr.sa_data[1]&0xFF),
                (ifr.ifr_hwaddr.sa_data[2]&0xFF),
                (ifr.ifr_hwaddr.sa_data[3]&0xFF),
                (ifr.ifr_hwaddr.sa_data[4]&0xFF),
                (ifr.ifr_hwaddr.sa_data[5]&0xFF));
    } 
    close(sock);


    /*
     * get logical interface status
     */
    dyn_if_info->if_status = get_logical_interface_status(if_name);


    /*
     * get IPv6 information
     */

    /* This is a HACK!
       I can't find a direct one-to-one replacement for the IPv4
       SIOCGIFADDR ioctl call in IPv6. So brute force scan the list...
       */
    /* \todo find an IPv6 version of the SIOCGIFADDR IPv4 ioctl */

    pf = fopen(PROC_NET_IF_INET6, "r");
    if (NULL == pf) {
        /* no IPv6 addresses */
        return(1);
    }

    while (fscanf(pf,
                "%40s %02x %02x %02x %02x %16s\n",
                ipv6_addr, &ipv6_if_index, &ipv6_prefix_len,
                &ipv6_scope, &ipv6_dad_status, ipv6_ifname) != EOF) {

        /* Note that the interface could have multiple IPv6 addresses
           assigned. The most recent assignment will be the first one
           we encounter, so use it since it will over-ride the default.
           This is important in VLANs since the default IPv6 address
           of the associated ethX interface will be assigned to each VLAN. */
        if (strcmp(ipv6_ifname, if_name) == 0) {
            /* we found the interface */
            dyn_if_info->ipv6_addr = (char*) malloc(41);
            if (!(dyn_if_info->ipv6_addr)) {
                DEBUGMSGTL(("cpqNic:ethtool",
                            "ERROR: malloc failed (errno=%d), %s,%dn",
                            errno, __FILE__, __LINE__));
                abort()  ;
            }
            dyn_if_info->ipv6_shorthand_addr =
                (char*) malloc(41);
            if (!(dyn_if_info->ipv6_shorthand_addr)) {
                DEBUGMSGTL(("cpqNic:ethtool",
                            "ERROR: malloc failed (errno=%d), %s,%dn",
                            errno, __FILE__, __LINE__));
                abort()  ;
            }

            sprintf(dyn_if_info->ipv6_addr, "%.4s:%.4s:%.4s:%.4s:%.4s:%.4s:%.4s:%.4s",
                    &ipv6_addr[0],  &ipv6_addr[4],
                    &ipv6_addr[8],  &ipv6_addr[12],
                    &ipv6_addr[16], &ipv6_addr[20],
                    &ipv6_addr[24], &ipv6_addr[28]);

            ipv6_sock.sin6_family = AF_INET6;
            ipv6_sock.sin6_port = 0;
            if (inet_pton(AF_INET6, dyn_if_info->ipv6_addr,
                        ipv6_sock.sin6_addr.s6_addr) <= 0) {
                printf("INFO: could not get ipv6 addr for %s: errno=%d\n",
                        if_name, errno);
                fclose(pf);
                return(1);
            }
            /* let inet_ntop convert the ipv6 addr to shorthand */
            if (inet_ntop(AF_INET6,
                        ipv6_sock.sin6_addr.s6_addr,
                        dyn_if_info->ipv6_shorthand_addr,
                        41) == (char*)0) {
                free(dyn_if_info->ipv6_shorthand_addr);
                dyn_if_info->ipv6_shorthand_addr = (char*)0;
                fclose(pf);
                return(1);
            }

            dyn_if_info->ipv6_prefix_length = ipv6_prefix_len;

            switch (ipv6_scope) {
                case IPV6_ADDR_ANY:
                    dyn_if_info->ipv6_scope = ipv6_global_scope;
                    break;
                case IPV6_ADDR_LOOPBACK:
                    dyn_if_info->ipv6_scope = ipv6_loopback_scope;
                    break;
                case IPV6_ADDR_LINKLOCAL:
                    dyn_if_info->ipv6_scope = ipv6_link_scope;
                    break;
                case IPV6_ADDR_SITELOCAL:
                    dyn_if_info->ipv6_scope = ipv6_site_scope;
                    break;
                case IPV6_ADDR_COMPATv4:
                    dyn_if_info->ipv6_scope = ipv6_v4compat_scope;
                    break;
                default:
                    dyn_if_info->ipv6_scope = ipv6_unspecified_scope;
                    break;
            }

            break;
        }
    }
    fclose(pf);
    return(rc);
}

void free_ethtool_info_members(ethtool_info *et_info)
{
    if (et_info->driver_name != NULL) {
        free(et_info->driver_name);
        et_info->driver_name = (char*) 0;
    }

    if (et_info->driver_version != NULL) {
        free(et_info->driver_version);
        et_info->driver_version = (char*) 0;
    }

    if (et_info->firmware_version != NULL) {
        free(et_info->firmware_version);
        et_info->firmware_version = (char*) 0;
    }

    if (et_info->bus_info != NULL) {
        free(et_info->bus_info);
        et_info->bus_info = (char*) 0;
    }
    return;
}

int32_t get_ethtool_info(char *if_name,
        ethtool_info *et_info)
{
    int32_t rval;
    static int32_t fd = -1;
    struct ifreq ifr;
    struct ethtool_drvinfo    einfo;
    struct ethtool_cmd        ecmd;
    struct ethtool_value      eval;
    struct ethtool_perm_addr *permaddr_data;

    memset(et_info, 0, sizeof(ethtool_info));
    memset(&einfo, 0, sizeof(struct ethtool_drvinfo));

    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, if_name);

    if (fd < 0) {
        fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (fd < 0) {
            DEBUGMSGTL(("cpqNic:ethtool",
                        "ERROR: could not open socket for %s, (errno=%d)\n",
                        if_name, errno));
            return(1);
        }
    }

    /* get driver information */
    einfo.cmd = ETHTOOL_GDRVINFO;
    ifr.ifr_data = (caddr_t)&einfo;
    rval = ioctl(fd, SIOCETHTOOL, &ifr);
    if (0 == rval) {
        if (einfo.driver != NULL)
            et_info->driver_name = strdup(einfo.driver);
        if (!(et_info->driver_name)) {
            DEBUGMSGTL(("cpqNic:ethtool",
                        "ERROR: malloc failed (errno=%d), %s,%dn",
                        errno, __FILE__, __LINE__));
            abort()  ;
        }

        if (einfo.version != NULL)
            et_info->driver_version = strdup(einfo.version);
        if (!(et_info->driver_version)) {
            DEBUGMSGTL(("cpqNic:ethtool",
                        "ERROR: malloc failed (errno=%d), %s,%dn",
                        errno, __FILE__, __LINE__));
            abort()  ;
        }

        if (einfo.fw_version != NULL)
            et_info->firmware_version = strdup(einfo.fw_version);
        if (!(et_info->firmware_version)) {
            DEBUGMSGTL(("cpqNic:ethtool",
                        "ERROR: malloc failed (errno=%d), %s,%dn",
                        errno, __FILE__, __LINE__));
            abort()  ;
        }

        if (einfo.bus_info != NULL)
            et_info->bus_info = strdup(einfo.bus_info);
        if (!(et_info->bus_info)) {
            DEBUGMSGTL(("cpqNic:ethtool",
                        "ERROR: malloc failed (errno=%d), %s,%dn",
                        errno, __FILE__, __LINE__));
            abort()  ;
        }

        DEBUGMSGTL(("cpqNic:ethtool", "ethtool driver info for %s = %s, %s, %s, %s\n",
                    if_name,
                    einfo.driver,
                    einfo.version,
                    einfo.fw_version,
                    einfo.bus_info));

    } else if ((rval < 0) && (errno != EOPNOTSUPP)) {
        DEBUGMSGTL(("cpqNic:ethtool",
                    "ERROR: ETHTOOL_GDRVINFO ioctl to %s failed, (errno=%d, %d)\n",
                    if_name, errno, rval));
    } else if ((rval < 0) && (errno == EOPNOTSUPP)) {
        /* we should not be here, something has changed about this
           interface below our feet. */
        return 1;
    }

    /* get physical link state
       \todo remove this ifdef after vmware gets GLINK */
    memset(&eval, 0, sizeof(struct ethtool_value));
    eval.cmd = ETHTOOL_GLINK;
    ifr.ifr_data = (caddr_t)&eval;
    rval = ioctl(fd, SIOCETHTOOL, &ifr);
    if (0 == rval) {
        et_info->link_status  = eval.data?link_up:link_down;
    } else if ((rval < 0) && (errno != EOPNOTSUPP)) {
        DEBUGMSGTL(("cpqNic:ethtool",
                    "ERROR: ETHTOOL_GLINK ioctl to %s failed, (errno=%d, %d)\n",
                    if_name, errno, rval));
        et_info->link_status  = link_unknown;
    }

    permaddr_data = (struct ethtool_perm_addr *)
        malloc(sizeof(struct ethtool_perm_addr) + MAC_ADDRESS_BYTES);
    if (!(permaddr_data)) {
        DEBUGMSGTL(("cpqNic:ethtool",
                    "ERROR: malloc failed (errno=%d), %s,%dn",
                    errno, __FILE__, __LINE__));
        abort()  ;
    }
    memset(permaddr_data, 0,
            sizeof(struct ethtool_perm_addr) + MAC_ADDRESS_BYTES);

    permaddr_data->cmd = ETHTOOL_GPERMADDR;
    permaddr_data->size = MAC_ADDRESS_BYTES;

    ifr.ifr_data = (caddr_t)permaddr_data;

    rval = ioctl(fd, SIOCETHTOOL, &ifr);
    if (0 == rval) {
        memcpy(et_info->perm_addr, permaddr_data->data,
                sizeof(et_info->perm_addr));
    } else if ((rval < 0) && (errno != EOPNOTSUPP)) {
        DEBUGMSGTL(("cpqNic:ethtool",
                    "ERROR: ETHTOOL_GPERMADDR ioctl to %s failed, (errno=%d, %d)\n",
                    if_name, errno, rval));
    }
    free(permaddr_data);

    /* get physical link speed */
    memset(&ifr, 0, sizeof(ifr));
    memset(&ecmd, 0, sizeof(struct ethtool_cmd));

    strcpy(ifr.ifr_name ,  if_name);
    ecmd.cmd = ETHTOOL_GSET;
    ifr.ifr_data = (caddr_t)&ecmd;

    rval = ioctl(fd, SIOCETHTOOL, &ifr);
    if (0 == rval) {
        et_info->autoneg  = ecmd.autoneg;
        et_info->duplex  = ecmd.duplex;
        if (ecmd.speed != 0xFFFF) {
            DEBUGMSGTL(("cpqNic:ethtool", "ethtool speed info %s = %d\n", if_name, ecmd.speed ));
            et_info->link_speed  = ecmd.speed;
        }
    } else if ((rval < 0) && (errno != EOPNOTSUPP)) {
        DEBUGMSGTL(("cpqNic:ethtool",
                    "ERROR: ETHTOOL_GSET ioctl to %s failed, (errno=%d, %d)\n",
                    if_name, errno, rval));
    }
    return(0);
}

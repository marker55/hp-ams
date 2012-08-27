/* (c) Copyright 2006 Hewlett-Packard Development Company, L.P.
   All rights reserved
 */
/*
 *  \file common_utilities.c
 *
 *  This file contains the code for the common utilities used by various
 *  projects.
 *
 */

#include "common_utilities.h"
#include "nic_db.h"

/*
 *  \brief Fee ethtool struct information allocated by get_ethtool_info()
 *
 *  unit tested by main
 *
 *  \author Tony Cureington
 *  \param[in] pet_info ptr to ethtoo_info structure containing allocated
 *             members to free
 *  \retval void
 *
 *  \see get_ethtool_info()
 *
 */
void free_ethtool_info_members(ethtool_info *pet_info)
{
    FREE_AND_NULL(pet_info->pdriver_name);
    pet_info->pdriver_name = (char*) 0;

    FREE_AND_NULL(pet_info->pdriver_version);
    pet_info->pdriver_version = (char*) 0;

    FREE_AND_NULL(pet_info->pfirmware_version);
    pet_info->pfirmware_version = (char*) 0;

    FREE_AND_NULL(pet_info->pbus_info);
    pet_info->pbus_info = (char*) 0;

    return;
}


/*
 *  \brief Get ethtool information associated with interface
 *
 *  Get the ethtool_drvinfo structure information and the link status
 *  associated with the given interface.
 *
 *  unit tested by main
 *
 *  \author Tony Cureington
 *  \param[in] pif_name ptr to inteface name
 *  \param[out] pet_info ptr to ethtool_info structure to populate
 *  \retval int32_t 0 on success, non-zero on failure
 *
 *  \see free_ethtool_info_members()
 *
 */
int32_t get_ethtool_info(char *pif_name,
        ethtool_info *pet_info)
{
    int32_t rval;
    static int32_t fd = -1;
    struct ifreq ifr;
    struct ethtool_drvinfo    einfo;
    struct ethtool_cmd        ecmd;	
    struct ethtool_value      eval;
    struct ethtool_perm_addr *permaddr_data;

    memset(pet_info, 0, sizeof(ethtool_info));
    memset(&einfo, 0, sizeof(struct ethtool_drvinfo));

    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, pif_name);

    if (fd < 0) {
        fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (fd < 0) {
            fprintf(stderr,
                    "ERROR: could not open socket for %s, (errno=%d)\n",
                    pif_name, errno);
            return(1);
        }
    }

    /* get driver information */
    einfo.cmd = ETHTOOL_GDRVINFO;
    ifr.ifr_data = (caddr_t)&einfo;
    rval = ioctl(fd, SIOCETHTOOL, &ifr);
    if (0 == rval) {
        pet_info->pdriver_name = STRDUP_NN(einfo.driver);
        CHECK_MALLOC(pet_info->pdriver_name);

        pet_info->pdriver_version = STRDUP_NN(einfo.version);
        CHECK_MALLOC(pet_info->pdriver_version);

        pet_info->pfirmware_version = STRDUP_NN(einfo.fw_version);
        CHECK_MALLOC(pet_info->pfirmware_version);

        pet_info->pbus_info = STRDUP_NN(einfo.bus_info);
        CHECK_MALLOC(pet_info->pbus_info);

        DPRINTF("ethtool driver info for %s = %s, %s, %s, %s\n",
                pif_name,
                einfo.driver,
                einfo.version,
                einfo.fw_version,
                einfo.bus_info);

    } else if ((rval < 0) && (errno != EOPNOTSUPP)) {
        fprintf(stderr,
                "ERROR: ETHTOOL_GDRVINFO ioctl to %s failed, (errno=%d, %d)\n",
                pif_name, errno, rval);
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
        pet_info->link_status  = eval.data?link_up:link_down;
    } else if ((rval < 0) && (errno != EOPNOTSUPP)) {
        fprintf(stderr,
                "ERROR: ETHTOOL_GLINK ioctl to %s failed, (errno=%d, %d)\n",
                pif_name, errno, rval);
        pet_info->link_status  = link_unknown;
    }

    permaddr_data = (struct ethtool_perm_addr *)
        malloc(sizeof(struct ethtool_perm_addr) + MAC_ADDRESS_BYTES);
    CHECK_MALLOC(permaddr_data);
    memset(permaddr_data, 0, 
            sizeof(struct ethtool_perm_addr) + MAC_ADDRESS_BYTES);

    permaddr_data->cmd = ETHTOOL_GPERMADDR;
    permaddr_data->size = MAC_ADDRESS_BYTES;

    ifr.ifr_data = (caddr_t)permaddr_data;

    rval = ioctl(fd, SIOCETHTOOL, &ifr);
    if (0 == rval) {
        memcpy(pet_info->perm_addr, permaddr_data->data, 
            sizeof(pet_info->perm_addr));
    } else if ((rval < 0) && (errno != EOPNOTSUPP)) {
        fprintf(stderr,
                "ERROR: ETHTOOL_GPERMADDR ioctl to %s failed, (errno=%d, %d)\n",
                pif_name, errno, rval);
    }
    free(permaddr_data);

    /* get physical link speed */
    memset(&ifr, 0, sizeof(ifr));
    memset(&ecmd, 0, sizeof(struct ethtool_cmd));

    strcpy(ifr.ifr_name ,  pif_name);
    ecmd.cmd = ETHTOOL_GSET;
    ifr.ifr_data = (caddr_t)&ecmd;

    rval = ioctl(fd, SIOCETHTOOL, &ifr);
    if (0 == rval) {
        pet_info->autoneg  = ecmd.autoneg;
        pet_info->duplex  = ecmd.duplex;
        if (ecmd.speed != 0xFFFF) {
            DPRINTF("ethtool speed info %s = %d\n", pif_name, ecmd.speed );
            pet_info->link_speed  = ecmd.speed;
        }
    } else if ((rval < 0) && (errno != EOPNOTSUPP)) {
        fprintf(stderr,
                "ERROR: ETHTOOL_GSET ioctl to %s failed, (errno=%d, %d)\n",
                pif_name, errno, rval);
    }

    return(0);
}

/*
 *  \brief Get the NIC hardware structure
 * 
 *  unit tested by get_db_struct_info
 * 
 *  Get the associated NIC hardware information (inf name, part number, etc)
 *  from the structure generated from the database.
 * 
 *  \author Tony Cureington
 *  \param[in] pci_ven ptr to pci vendor id
 *  \param[in] pci_dev ptr to pci device id
 *  \param[in] pci_sub_ven ptr to pci sub-vendor id
 *  \param[in] pci_sub_dev ptr to pci sub-device id
 *  \param[out] ppdbs pointer-to-ptr to nic_hw_db_t database structure
 *  \retval int32_t zero on match, non-zero if no match
 * 
 *  \warning the ptrs in pdbs point to the global copy of the database struct,
 *           so don't modify the contents. this prevents having to allocate
 *           and free the structure. there is currently no need for an app
 *           to modify the contents of the pdbs, that is why it was done 
 *           this way.
 * 
 *  \note the inf_name struct member must be decoded using the
 *        decode_inf_string function. this is automatically done
 *        if the get_db_struct_info is called to get the struct.
 * 
 *  \see get_pci_info()
 * 
 */
nic_hw_db_t * get_nic_hw_info(ushort pci_ven,
                           ushort pci_dev,
                           ushort pci_sub_ven,
                           ushort pci_sub_dev)
{

    int32_t i;

    for (i=0; i<NIC_DB_ROWS; i++) {
        /* yes, brute force this search - there is no need to get fancy
           since there are less than 100 entries... */
        if ((pci_ven     == nic_hw[i].pci_ven)     &&
            (pci_dev     == nic_hw[i].pci_dev)     &&
            (pci_sub_ven == nic_hw[i].pci_sub_ven) &&
            (pci_sub_dev == nic_hw[i].pci_sub_dev)) {
            return(&nic_hw[i]);
        }
    }
    return(NULL);
}

/*
 *  \brief Determine the slot/bay and port number associated with an interface
 * 
 *  unit tested by main
 * 
 *  Get the slot/port (and bay/port for c-class blades) for the device, given
 *  the PCI bus info sting. The PCI bus info string looks like "0000:02:0e.0"
 * 
 *  \author Tony Cureington
 *  \param[in] pbus_info ptr to bus information
 *  \param[out] pslot the physical slot the NIC is in 
 *  \param[out] pport the physical port of this NIC
 *  \param[out] pbay_slot on c-class type blades, the cable bay of NIC port
 *  \param[out] pbay_port on c-class type blades, the cable port of the NIC
 *  \retval int32_t 0 on success, non-zero on failure
 * 
 *  \see get_ethtool_info()
 * 
 */
int32_t get_slot_info(char *pbus_info,
        int32_t *pslot,
        int32_t *pport,
        int32_t *pbay_slot,
        int32_t *pbay_port,
        int32_t *pvirtual_port,
        int32_t mlxport)
{
    int32_t rc;
    int32_t domain;
    int32_t bus;
    int32_t device;
    int32_t function;
    int32_t colen_count;
    char *pch;

    *pslot = UNKNOWN_SLOT;
    *pport = UNKNOWN_PORT;
    *pbay_slot = UNKNOWN_SLOT;
    *pbay_port = UNKNOWN_PORT;
    *pvirtual_port = UNKNOWN_PORT;

    /* the pbus_info var is in "domain:bus:slot.func". sometimes the
       domain may be absent, in which case we have "bus:slot.func". */
    colen_count=0;
    pch=strchr(pbus_info, ':');
    while ((char*)0 != pch) {
        colen_count++;
        pch=strchr(pch+1, ':');
    }

    if (0 == colen_count) {
        /* driver did not provide the info */
        return(1);
    } else
        if (1 == colen_count) {
            sscanf(pbus_info, "%x:%x.%x",
                    &bus, &device, &function);
        } else
            if (2 == colen_count) {
                sscanf(pbus_info, "%x:%x:%x.%x",
                        &domain, &bus, &device, &function);
            } else {
                /* should not get here unless something is really wacked-out */
                printf("WARNING: unable to parse \"%s\"\n", pbus_info);
                return(1);
            }
        /* port info
           port = get_bus_info(); */
        DPRINTF("bus=%d, device=%d, function=%d, port= %d\n", 
                 bus, device, function, mlxport);

        rc =  get_slot_bay_info(bus, device, function,
                pslot, pport, 
                pbay_slot, pbay_port, pvirtual_port, mlxport);

        DPRINTF("slot=%d, port=%d, bay=%d, bay_port=%d, " 
                "virtual_port=%d, RC=%d\n", 
                *pslot, *pport, *pbay_slot, *pbay_port, 
                *pvirtual_port,  rc);

        return((SUCCESS==rc)?0:1);
}


/*
 *  \brief Get logical interface (up/down/unknown) status using ioctl
 *
 *  Get the logical interface (up/down/unknown) status - the logical interface 
 *  can only be up, down, or unknown. We are NOT talking about the physical
 *  interface status we are taling about the logical interface status.
 *  A link_unknown value could be returned if the logical interface does
 *  not exists, for example.
 *
 *  \author Tony Cureington
 *  \param[in] ptr to interface name
 *  \retval link_status_t (link_up, link_down, link_unknown)
 *
 */
link_status_t get_logical_interface_status(char *pif_name)
{
    struct ifreq ifr;
    static int32_t sock = -1;


    if (sock < 0) {
        sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
        if (sock < 0) {
            DPRINTF("socket() failed: errno=%d (%s)\n",
                    errno, strerror(errno));
            return(link_unknown);
        }
    }

    ifr.ifr_data = (char*)0;

    strcpy(ifr.ifr_name, pif_name);

    /* get interface up/down status */
    if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0) {
        if (errno != ENODEV) {
            fprintf(stderr, 
                    "INFO: ioctl(SIOCGIFFLAGS) for %s: errno=%d, %s\n",
                    pif_name, errno, strerror(errno));
        } else {
            DPRINTF("INFO: ioctl(SIOCGIFFLAGS) for %s: errno=%d, %s\n",
                    pif_name, errno, strerror(errno));
        }
        return(link_unknown);
    }

    return((ifr.ifr_flags & IFF_UP)?link_up:link_down);
}


/*
 *  \brief get the permanent MAC address associated to the interface
 *
 *  \author Tony Cureington
 *  \param[in] ptr to interface name
 *  \param[out] ppermanent_mac the permanent MAC address, allocate memory must
 *              be freed by calling app.
 *  \retval int32_t 0 on success, non-zero on failure
 *
 *  \todo Vx: The ethtool_perm_addr (ethtool_op_get_perm_addr in kernel) should
 *            be called to get the permanent MAC address, but it is not
 *            available in our current distro ethtool interface. It is in at
 *            least kernel version 2.6.18-1.
 *
 */
#ifdef ETHTOOL_GEEPROM
int32_t get_permanent_mac_addr(char *pif_name,
        char **pppermanent_mac)
{
    int i;
    int32_t rval;
    static int32_t fd = -1;
    struct ifreq ifr;
    struct ethtool_eeprom *peeeprom; // ptr to ethtool eeprom struct
    u8 *peeprom_data; // ptr to eeprom data
    int multi_port = 0;
    int max_eelen = 8;  // max bytes read from eeprom
    int eelen = 6;
    char eeeprom[sizeof(struct ethtool_eeprom) + max_eelen];
    char *pch;
    int pci_function = 0;
    int move_mac = 0;
    ethtool_info et_info;
    ushort pci_ven;
    ushort pci_dev;
    ushort pci_sub_ven;
    ushort pci_sub_dev;

    /* get bus info */
    if (get_ethtool_info(pif_name, &et_info)) {
        DPRINTF("can't get ethtool info for = %s\n",
                pif_name);
        goto get_permanent_mac_addr_error_exit;
    }

    /* get pci info */
    if (get_pci_info(et_info.pbus_info, &pci_ven, &pci_dev, 
                &pci_sub_ven, &pci_sub_dev)) {
        DPRINTF("can't get pci device info address for = %s\n",
                pif_name);
        goto get_permanent_mac_addr_error_exit;
    }

    if (get_nic_hw_info(pci_ven, pci_dev, pci_sub_ven, pci_sub_dev) == NULL) {
        DPRINTF("can't get permanent MAC address for = %s\n",
                pif_name);
        goto get_permanent_mac_addr_error_exit;
    }

    peeeprom = (struct ethtool_eeprom *)eeeprom;
    peeprom_data = (u8*)peeeprom->data;

    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, pif_name);

    if (fd < 0) {
        fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (fd < 0) {
            fprintf(stderr,
                    "ERROR: could not open socket for %s, (errno=%d)\n",
                    pif_name, errno);
            goto get_permanent_mac_addr_error_exit;
        }
    }

    /* read mac address from eeprom */
    peeeprom->cmd = ETHTOOL_GEEPROM;
    ifr.ifr_data = (caddr_t)peeeprom;
    peeeprom->len = eelen;

    /* get the pci function (in the form of 0000:02:0e.0) */
    pch = strrchr(et_info.pbus_info, '.');
    if ((char*)0 != pch) {
        pch++;
        pci_function = atoi(pch);
    }

    DPRINTF("pci function = %d\n",
            pci_function);

    if (0x8086 == pci_ven) {
        /* Intel */
        peeeprom->offset = 0x0;

        /* 00db = dual port Intel copper 
           00dc = dual port Intel fiber 
           3109 = quad port Intel copper */
        if ((0x00DB == pci_sub_dev)  || 
                (0x00DC == pci_sub_dev)  ||
                (0x3109 == pci_sub_dev)) {
            multi_port = 1;
        }

        rval = ioctl(fd, SIOCETHTOOL, &ifr);
        if (0 == rval) {
            if (multi_port && (1 == pci_function)) {
                /* add one to mac address */
                for (i=eelen-1; i>=0; i--) {
                    (peeprom_data[i])++;
                    if (0 != peeprom_data[i]) {
                        /* no roll-over */
                        break;
                    }
                    /* value is zero, so add 1 to next byte of MAC address */
                }
            }
        } else if ((rval < 0) && (errno != EOPNOTSUPP)) {
            fprintf(stderr,
                    "ERROR: ETHTOOL_GEEPROM ioctl to %s failed, "
                    "(errno=%d, %d)\n",
                    pif_name, errno, rval);
            goto get_permanent_mac_addr_error_exit;
        }

    } else if (0x14E4 == pci_ven) {
        /* Broadcom */

        /* 00d0 = dual port Broadcom */
        if (0x00D0 == pci_sub_dev) {
            multi_port = 1;
        }


        if ((0x16AA == pci_dev) ||
                (0x164A == pci_dev) ||
                (0x16AC == pci_dev) ||
                (0x164C == pci_dev)) {
            /* bcm5706, bcm5708 */
            peeeprom->offset = 0x136;
        } else {
            /* non 5706/5708 bcm boards must have 4 byte boundary reads */
            peeeprom->len = 8;
            move_mac = 2;
            if (multi_port && (0 == pci_function)) {
                peeeprom->offset = 0xCC;
            } else {
                peeeprom->offset = 0x7C;
            }
        }

        rval = ioctl(fd, SIOCETHTOOL, &ifr);
        if (0 != rval) {
            fprintf(stderr,
                    "ERROR: ETHTOOL_GEEPROM ioctl to %s failed, "
                    "(errno=%d, %d)\n",
                    pif_name, errno, rval);
            goto get_permanent_mac_addr_error_exit;
        }
    } else {
        DPRINTF("unknown pci vendor 0x%X\n",
                pci_ven);
        goto get_permanent_mac_addr_error_exit;
    }

    /* convert hex mac address to string
       each hex digit requires 3 ascii chars (eg FF == "FF:")
       plus 1 for null */
    *pppermanent_mac = (char*) malloc(MAC_ADDRESS_BYTES * 3 + 1);
    CHECK_MALLOC(*pppermanent_mac);

    if (move_mac) {
        memmove(peeprom_data, &(peeprom_data[move_mac]), MAC_ADDRESS_BYTES);
    }

    sprintf(*pppermanent_mac,
            "%.2X:%.2X:%.2X:%.2X:%.02X:%.2X",
            (peeprom_data[0]&0xFF),
            (peeprom_data[1]&0xFF),
            (peeprom_data[2]&0xFF),
            (peeprom_data[3]&0xFF),
            (peeprom_data[4]&0xFF),
            (peeprom_data[5]&0xFF));

    DPRINTF("permanent MAC address from eeprom read = %s\n",
            *pppermanent_mac);

    return(0);

get_permanent_mac_addr_error_exit:
    free_ethtool_info_members(&et_info);
    return(1);
}
#endif /* ETHTOOL_GEEPROM */


/*
 *  \brief free memory allocated by get_dynamic_interface_info call
 *
 *  unit tested by main
 *
 *  \author Tony Cureington
 *  \param[in] pif pointer to dynamic_interf_info_t structure
 *  \retval void
 *
 *  \see get_dynamic_interface_info()
 *
 *  \note any new dynamically structure members added to dynamic_interf_info_t
 *        must be freed here.
 *
 */
void free_dynamic_interface_info(dynamic_interface_info_t *pif)
{
    FREE_AND_NULL(pif->pip_addr);
    FREE_AND_NULL(pif->pnetmask);
    FREE_AND_NULL(pif->pbroadcast);
    FREE_AND_NULL(pif->pcurrent_mac);
    FREE_AND_NULL(pif->pipv6_addr);
    FREE_AND_NULL(pif->pipv6_shorthand_addr);
    return;
}


/*
 *  \brief get dynamic (IP, link status, etc) NIC information
 *
 *  Get the dynamic NIC information:
 *     current IP
 *     current MAC
 *     interface status (up/down)
 *     link status
 *     ETC
 *
 *  unit tested by main
 *
 *  \author Tony Cureington
 *  \param[in] pif_name pointer to interface name (eth0, etc)
 *  \param[out] pif pointer to interface_t structure to get dynamic info for
 *  \retval int32_t 0 on success, 1 if one or more struct members could
 *          not be obtained, a value less than zero on failure (-1)
 *
 *  \see dynamic_interface_info_t structure for items gathered by this function
 *
 *  \see free_dynamic_interface_info()
 *
 *  \note don't use the SIOCGIFCONF ioctl to loop through the interfaces,
 *        as it does not return un-configured interfaces (those without
 *        an IP address)
 *
 *  \note don't use libc networking functions that are not thread 
 *        safe, such as inet_ntoa
 *
 *
 *
 */
int32_t get_dynamic_interface_info(char* pif_name, 
                                   dynamic_interface_info_t *pif)
{
    int32_t rc = 0;
    struct ifreq ifr;
    static int32_t sock = -1;
    FILE *pf;
    char ipv6_addr[IPV6_ADDRESS_ASCII_CHARS]; 
    char ipv6_ifname[IF_NAMESIZE];
    int32_t ipv6_if_index;
    int32_t ipv6_prefix_len;  // see inet6_ifaddr in if_inet6.h kernel header
    int32_t ipv6_scope;
    int32_t ipv6_dad_status;
    struct sockaddr_in6 ipv6_sock;


    /* null all members in struct */
    memset(pif, 0, sizeof(dynamic_interface_info_t));

    if (sock < 0) {
        sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
        if (sock < 0) {
            DPRINTF("socket() failed: errno=%d (%s)\n", errno, strerror(errno));
            /*/ \todo investigate if we will always be able to get
                 an AF_INET socket, even if we only have and IPv6 interface. */
            return(-1);
        }
    }

    ifr.ifr_data = (char*)0;
    ifr.ifr_addr.sa_family = AF_INET;
    strcpy(ifr.ifr_name, pif_name);

    /*
     * get the current IP address
     */
    if (ioctl(sock, SIOCGIFADDR, &ifr) < 0) {
        if ((errno != ENODEV) && (errno != EADDRNOTAVAIL)) {
            fprintf(stderr, 
                    "INFO: ioctl(SIOCGIFADDR) for %s: errno=%d, %s\n",
                    pif_name, errno, strerror(errno));
            rc = 1;
        } else {
            DPRINTF("INFO: ioctl(SIOCGIFADDR) for %s: errno=%d, %s\n",
                    pif_name, errno, strerror(errno));
        }
    } else {
        /* each digit requires up to 4 ascii chars (eg 255 == "255.")
           plus 1 for null */
        pif->pip_addr = (char*) malloc(sizeof(ifr.ifr_addr.sa_data) * 4 + 1);
        CHECK_MALLOC(pif->pip_addr);

        sprintf(pif->pip_addr,
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
                fprintf(stderr, 
                        "INFO: ioctl(SIOCGIFNETMASK) for %s: errno=%d, %s\n",
                        pif_name, errno, strerror(errno));
                rc = 1;
            } else {
                DPRINTF("INFO: ioctl(SIOCGIFNETMASK) for %s: errno=%d, %s\n",
                        pif_name, errno, strerror(errno));
            }
        } else {
            /* each digit requires up to 4 ascii chars (eg 255 == "255.")
            plus 1 for null */
            pif->pnetmask = (char*) malloc(sizeof(ifr.ifr_addr.sa_data) * 4 + 1);
            CHECK_MALLOC(pif->pnetmask);
    
            sprintf(pif->pnetmask, "%d.%d.%d.%d",
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
                fprintf(stderr, 
                        "INFO: ioctl(SIOCGIFBRDADDR) for %s: errno=%d, %s\n",
                        pif_name, errno, strerror(errno));
                rc = 1;
            } else {
                DPRINTF("INFO: ioctl(SIOCGIFBRDADDR) for %s: errno=%d, %s\n",
                        pif_name, errno, strerror(errno));
            }
        } else {
            /* each digit requires up to 4 ascii chars (eg 255 == "255.")
            plus 1 for null */
            pif->pbroadcast = (char*) malloc(sizeof(ifr.ifr_addr.sa_data) * 4 + 1);
            CHECK_MALLOC(pif->pbroadcast);
    
            sprintf(pif->pbroadcast,
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
            fprintf(stderr, 
                    "ERROR: ioctl(SIOCGIFHWADDR) for %s: errno=%d, %s\n",
                    pif_name, errno, strerror(errno));
            rc = 1;
        } else {
            DPRINTF("INFO: ioctl(SIOCGIFHWADDR) for %s: errno=%d, %s\n",
                    pif_name, errno, strerror(errno));
        }
    } else {
        /* each hex digit requires 3 ascii chars (eg FF == "FF:")
           plus 1 for null */
        pif->pcurrent_mac = 
            (char*) malloc(sizeof(ifr.ifr_hwaddr.sa_data) * 3 + 1);
        CHECK_MALLOC(pif->pcurrent_mac);

        sprintf(pif->pcurrent_mac,
                "%.2X:%.2X:%.2X:%.2X:%.02X:%.2X",
                (ifr.ifr_hwaddr.sa_data[0]&0xFF),
                (ifr.ifr_hwaddr.sa_data[1]&0xFF),
                (ifr.ifr_hwaddr.sa_data[2]&0xFF),
                (ifr.ifr_hwaddr.sa_data[3]&0xFF),
                (ifr.ifr_hwaddr.sa_data[4]&0xFF),
                (ifr.ifr_hwaddr.sa_data[5]&0xFF));
    }


    /*
     * get logical interface status
     */
    pif->if_status = get_logical_interface_status(pif_name);


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
        if (strcmp(ipv6_ifname, pif_name) == 0) {
            /* we found the interface */
            pif->pipv6_addr = (char*) malloc(IPV6_ADDRESS_USER_READABLE+1);
            CHECK_MALLOC(pif->pipv6_addr);
            pif->pipv6_shorthand_addr = 
                (char*) malloc(IPV6_ADDRESS_USER_READABLE+1);
            CHECK_MALLOC(pif->pipv6_shorthand_addr);

            sprintf(pif->pipv6_addr, "%.4s:%.4s:%.4s:%.4s:%.4s:%.4s:%.4s:%.4s", 
                    &ipv6_addr[0],  &ipv6_addr[4],  
                    &ipv6_addr[8],  &ipv6_addr[12],
                    &ipv6_addr[16], &ipv6_addr[20], 
                    &ipv6_addr[24], &ipv6_addr[28]);

            ipv6_sock.sin6_family = AF_INET6;
            ipv6_sock.sin6_port = 0;
            if (inet_pton(AF_INET6, pif->pipv6_addr, 
                        ipv6_sock.sin6_addr.s6_addr) <= 0) {
                printf("INFO: could not get ipv6 addr for %s: errno=%d\n",
                        pif_name, errno);
                fclose(pf);
                return(1);
            }
            /* let inet_ntop convert the ipv6 addr to shorthand */
            if (inet_ntop(AF_INET6,
                        ipv6_sock.sin6_addr.s6_addr,
                        pif->pipv6_shorthand_addr,
                        IPV6_ADDRESS_USER_READABLE) == (char*)0) {
                free(pif->pipv6_shorthand_addr);
                pif->pipv6_shorthand_addr = (char*)0;
                fclose(pf);
                return(1);
            }

            pif->pipv6_refix_length = ipv6_prefix_len;

            switch (ipv6_scope) {
                case IPV6_ADDR_ANY:
                    pif->ipv6_scope = ipv6_global_scope;
                    break;
                case IPV6_ADDR_LOOPBACK:
                    pif->ipv6_scope = ipv6_loopback_scope;
                    break;
                case IPV6_ADDR_LINKLOCAL:
                    pif->ipv6_scope = ipv6_link_scope;
                    break;
                case IPV6_ADDR_SITELOCAL:
                    pif->ipv6_scope = ipv6_site_scope;
                    break;
                case IPV6_ADDR_COMPATv4:
                    pif->ipv6_scope = ipv6_v4compat_scope;
                    break;
                default:
                    pif->ipv6_scope = ipv6_unspecified_scope;
                    break;
            }

            break;
        }
    }
    fclose(pf);
    return(rc);
}

/*
 *  \brief Unit test for this file
 *
 *  \author Tony Cureington
 *  \param[in] none
 *  \retval none
 *
 */
#ifdef UNIT_TEST

/* turn on/off unit tests */
#define UT_DECODE_INF_STRING                  0
#define UT_DETECT_LINUX_DISTRO                0
#define UT_TRIM_CHARS                         0
#define UT_GET_SLOT_INFO                      1
#define UT_GET_DYNAMIC_INFERFACE_INFO         0
#define UT_READ_TOKENS                        0
#define UT_GET_INTERFACE_LIST                 0

int main()
{

    printf("\n\nStarting Unit Test!\n\n");

    /*
     * UNIT_TEST get_ethtool_info, free_ethtool_info_members, get_pci_info,
     *           non_hp_nic, get_db_struct_info, free_db_struct_info,
     *           & get_slot_info
     */
#if UT_GET_SLOT_INFO
    {
        int32_t rc;
        char *pif = "eth2";
        ethtool_info einfo;
        ushort pci_ven;
        ushort pci_dev;
        ushort pci_sub_ven;
        ushort pci_sub_dev;
        nic_hw_db_t *pdbs;

        /* NC1020 PCI ID for testing if current server does not contain
           a PCI ID in the database (ie, we are running on laptop) */
        ushort try_pci_ven     = 0x14E4;
        ushort try_pci_dev     = 0x1654;
        ushort try_pci_sub_ven = 0x103C;
        ushort try_pci_sub_dev = 0x3100;

        int32_t slot;
        int32_t port;
        int32_t bay_slot;
        int32_t bay_port;
        int32_t virtual_port;

        get_ethtool_info(pif, &einfo);
        printf("\n\nget_ethtool_info info for %s:\n   %s\n   %s\n   %s\n   %s\n",
                pif,
                einfo.pdriver_name,
                einfo.pdriver_version,
                einfo.pfirmware_version,
                einfo.pbus_info);

        printf("link status: %s\n", link_status_text[einfo.link_status]);

        rc = get_pci_info(einfo.pbus_info,
                &pci_ven,
                &pci_dev,
                &pci_sub_ven,
                &pci_sub_dev);
        if (rc) {
            printf("\n\nERROR: RC = %d\n%s,%d\n\n",
                    rc);
            exit(1);
        }

        printf("\n");
        printf("pci_ven=     0x%.4X\n", pci_ven);
        printf("pci_dev=     0x%.4X\n", pci_dev);
        printf("pci_sub_ven= 0x%.4X\n", pci_sub_ven);
        printf("pci_sub_dev= 0x%.4X\n", pci_sub_dev);

        if (get_nic_hw_info(pci_ven, pci_dev, pci_sub_ven, pci_sub_dev) == NULL) {
            printf("\nThis is NOT an HP NIC according to non_hp_nic\n");
        } else {
            printf("\nThis IS an HP NIC according to non_hp_nic\n");
        }

        if ((pdbs = get_nic_hw_info(pci_ven,
                pci_dev,
                pci_sub_ven,
                pci_sub_dev)) == NULL) {
            printf("\n\nWARNING: hw database did not contain entry for: \n");
            printf("   pci_ven=     0x%.4X\n", pci_ven);
            printf("   pci_dev=     0x%.4X\n", pci_dev);
            printf("   pci_sub_ven= 0x%.4X\n", pci_sub_ven);
            printf("   pci_sub_dev= 0x%.4X\n", pci_sub_dev);

            printf("\nRETRYING with: 0x%.4X 0x%.4X 0x%.4X 0x%.4X\n",
                    try_pci_ven, try_pci_dev, try_pci_sub_ven, try_pci_sub_dev);
            if ((pdbs = get_nic_hw_info(try_pci_ven,
                    try_pci_dev,
                    try_pci_sub_ven,
                    try_pci_sub_dev)) == NULL) {
                printf("\n\nERROR: NOT FOUND \n");
                exit(1);
            }
        }

        printf("\n");
        printf("poem =                 %s\n", pdbs->poem);
        printf("ppca_part_number =     %s\n", pdbs->ppca_part_number);
        printf("pspares_part_number =  %s\n", pdbs->pspares_part_number);
        printf("pboard_rev =           %s\n", pdbs->pboard_rev);
        printf("pport_type =           %s\n", pdbs->pport_type);
        printf("pbus_type =            %s\n", pdbs->pbus_type);
        printf("inf_name_len =         %d\n", pdbs->inf_name_len);
        printf("inf name =             %s\n", pdbs->inf_name);

        rc = get_slot_info(einfo.pbus_info,
                &slot,
                &port,
                &bay_slot,
                &bay_port,
                &virtual_port,
                mlxport);

        if (rc) {
            printf("\n\nWARNING: get_slot_info RC = %d\n%s,%d\n\n",
                    rc);
        } else {
            printf("\n\nslot = %d\nport = %d\nbay_slot = %d\nbay_port = %d\n",
                    slot, port, bay_slot, bay_port);
        }

        free_ethtool_info_members(&einfo);

    }
#endif

    /*
       UNIT_TEST get_dynamic_interface_info & free_dynamic_interface_info
     */
#if UT_GET_DYNAMIC_INFERFACE_INFO
    {
        int32_t i;
        int32_t rc;
        dynamic_interface_info_t dii;
        char *(peth[25]) = {"lo", "eth0", ""};


        for (i=0; strlen(peth[i])>0; i++) {
            printf("\n\n");
            printf("interface name          = %s\n", peth[i]);
            rc = get_dynamic_interface_info(peth[i], &dii);
            if (rc<0) {
                printf("\n\nWARNING: get_dynamic_interface_info RC = %d\n%s,%d\n\n",
                        rc);
                continue;
            }

            printf("ip addr                  = %s\n", dii.pip_addr);
            printf("netmask addr             = %s\n", dii.pnetmask);
            printf("broadcast addr           = %s\n", dii.pbroadcast);
            printf("mac addr                 = %s\n", dii.pcurrent_mac);

            printf("ipv6 scope               = %s\n", 
                    ipv6_link_scope_text[dii.ipv6_scope]);
            printf("ipv6 addr                = %s/%d\n", 
                    dii.pipv6_addr, dii.pipv6_refix_length);
            printf("ipv6 addr (shorthand)    = %s/%d\n", 
                    dii.pipv6_shorthand_addr, dii.pipv6_refix_length);

            printf("logical interface status = %s\n", 
                    link_status_text[dii.if_status]);

            free_dynamic_interface_info(&dii);
        }
    }
#endif

    printf("\n\nDONE!\n\n");
    return(0);
}
#endif // UNIT_TEST

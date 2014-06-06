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
#include <sys/ioctl.h>
#include <dirent.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/utsname.h>
#include <linux/version.h>

#include "cpqFcaHostCntlrTable.h"
#include "cpqHoFwVerTable.h"

#include "fcahc.h"
#include "common/smbios.h"
#include "common/utils.h"
#include "common/scsi_info.h"

#include "fca_linux.h"

extern unsigned char     cpqHoMibHealthStatusArray[];
extern oid       cpqHoFwVerTable_oid[];
extern size_t    cpqHoFwVerTable_oid_len;
extern int register_FW_version(int dir, int fw_idx, int cat, int type,  int update,
                        char *fw_version, char *name, char *location, char *key);

extern int file_select(const struct dirent *);
extern int pcislot_scsi_host(char * buffer);
void SendFcaTrap(int, cpqFcaHostCntlrTable_entry *);
void netsnmp_arch_fcahc_init(void); 

extern  int alphasort();

static struct dirent **FcaHostlist;
static char * FcaHostDir = "/sys/class/fc_host/";
static int NumFcaHost;

static struct dirent **ScsiHostlist;
static char * ScsiHostDir = "/sys/class/scsi_host/";
static int NumScsiHost;

static int fca_select(const struct dirent *entry)
{
    int      i = 0;
    for (i = 0; i < NumFcaHost; i++){
        if (strncmp(entry->d_name,
                    FcaHostlist[i]->d_name,
                    strlen(entry->d_name)) == 0)
            return(1);
    }
    return 0;
}
int enumPCIid(unsigned int BoardID)
{

            switch (BoardID) {
                case 0xF0950E11:
                    return( 6);
                    break;
                case 0xF8000E11:
                    return( 7);
                    break;
                case 0x01000E11:
                    return( 8);
                    break;
                case 0x01020E11:
                    return( 9);
                    break;
                case 0xF98010DF:
                    return(10);
                    break;
                case 0x10000E11:
                    return(11);
                    break;
                case 0xF09810DF:
                    return(12);
                    break;
                case 0x01010E11:
                    return(13);
                    break;
                case 0x12BA103C:
                    return(14);
                    break;
                case 0x01030E11:
                    return(15);
                    break;
                case 0x01040E11:
                    return(16);
                    break;
                case 0x12DD103C:
                    return(19);
                    break;
                case 0x12D7103C:
                    return(20);
                    break;
                case 0xF0D510DF:
                    return(21);
                    break;
                case 0xFD0010DF:
                    return(22);
                    break;
                case 0xF0A510DF:
                    return(23);
                    break;
                case 0x1708103C:
                    return(24);
                    break;
                case 0x1705103C:
                    return(25);
                    break;
                case 0x7040103C:
                    return(26);
                    break;
                case 0x7041103C:
                    return(27);
                    break;
                case 0xF0E510DF:
                    return(28);
                    break;
                case 0xFE0010DF:
                    return(29);
                    break;
                case 0x1702103C:
                    return(30);
                    break;
                case 0x3262103C:
                    return(31);
                    break;
                case 0x3263103C:
                    return(32);
                    break;
                case 0x3261103C:
                    return(33);
                    break;
                case 0x3281103C:
                    return(34);
                    break;
                case 0x3282103C:
                    return(35);
                    break;
                case 0x1719103C:
                    return(36);
                    break;
                case 0x17E4103C:
                    return(37);
                    break;
                case 0x17E5103C:
                    return(38);
                    break;
                case 0x17E7103C:
                    return(39);
                    break;
                case 0x17E8103C:
                    /*HP SN1000Q 16Gb Dual Port FC HBA */
                  return(40); 
                    break;
                case 0x197E103C:
                    /*HP StoreFabric SN1100E 16Gb Single Port */
                    return(41); 
                    break;
                case 0x197F103C:
                    /*HP StoreFabric SN1100E 16Gb Dual Port */
                    return(42); 
                    break;
                case 0x1743103C:
                    /*HP StorageWorks 81B 8Gb Single Port PCI-e FC HBA */
                    return(43); 
                    break;
                case 0x1742103C:
                    /*HP StorageWorks 82B 8Gb Dual Port PCI-e FC HBA */
                    return(44); 
                    break;
                case 0x3344103C:
                    /*HP StorageWorks CN1100E Dual Port Converged Network Adapter */
                    return(45); 
                    break;
                case 0x337B103C:
                    /*HP FlexFabric 10Gb 2-port 554FLB Adapter */
                    return(46); 
                    break;
                case 0x337C103C:
                    /*HP FlexFabric 10Gb 2-port 554M Adapter */
                    return(47); 
                    break;
                case 0x3376103C:
                    /*HP FlexFabric 10Gb 2-port 554FLR-SFP+ Adapter */
                    return(48); 
                    break;
                case 0x338F103C:
                    /*HP Fibre Channel 8Gb LPe1205A Mezz */
                    return(49); 
                    break;
                case 0x3348103C:
                    /*HP StorageWorks CN1000Q Dual Port Converged Network Adapter */
                    return(50); 
                    break;
                case 0x338E103C:
                    /*HP QMH2572 8Gb FC HBA for c-Class BladeSystem */
                    return(51); 
                    break;
                case 0x1957103C:
                    /*HP FlexFabric 10Gb 2-port 526FLR-SFP+ Adapter */
                    return(52); 
                    break;
                case 0x1939103C:
                    /*HP QMH2672 8Gb FC HBA for c-Class BladeSystem */
                    return(53); 
                    break;
                case 0x1932103C:
                    /*HP FlexFabric 10Gb 2-port 534FLB Adapter */
                    return(54); 
                    break;
                case 0x1930103C:
                    /*HP FlexFabric 10Gb 2-port 534FLR-SFP+ Adapter */
                    return(55); 
                    break;
                case 0x1933103C:
                    /*HP FlexFabric 10Gb 2-port 534M Adapter */
                    return(56); 
                    break;

                case 0x1931103C:
                    /*HP StoreFabric CN1100R Dual Port Converged Network Adapter */
                    return(57); 
                    break;

                case 0x1956103C:
                    /*HP Fibre Channel 16Gb LPe1605 Mezz */
                    return(58); 
                    break;
    
                case 0x1916103C:
                    /* Broadcom 630FLB flex fabric id   */
                    return (59);
                    break;

                case 0x1917103C:
                    /* Broadcom 630M flex fabric id     */
                    return (60);
                    break;

                case 0x220A103C:
                    /* Emulex 556FLR-SFP+  */
                    return (61);
                    break;

                case 0x1934103C:
                    /* Emulex 650M */
                    return (62);
                    break;

                case 0x1935103C:
                    /* Emulex 650FLB */
                    return (63);
                    break;

                case 0x21D4103C:
                    /* Emulex CN1200E */
                    return (64);
                    break;

                case 0x22FA103C:
                    /* Broadcom 536FLB */
                    return (65);
                    break;

                default:
                    return(1);
                    break;
            }
}

char * getFcHBASerialNum(char *buffer)
{
    char attribute[1024];
    char *value = NULL;

    if (strlen(buffer) >= 1024)
        return (char*) 0;
    strcpy(attribute, buffer);
    if ((strlen(buffer) + strlen(sysfs_attr[DEVICE_SERIALE])) >= 1024)
        return (char*) 0;
    strcat(attribute, sysfs_attr[DEVICE_SERIALE]);
    DEBUGMSGTL(("fcahc:container:load", "Getting attribute from %s\n",
                    attribute));
    if ((value = get_sysfs_str(attribute)) == NULL) {
        strcpy(attribute, buffer);
        if ((strlen(buffer) + strlen(sysfs_attr[DEVICE_SERIALQ])) >= 1024)
            return (char*) 0;
        strcat(attribute, sysfs_attr[DEVICE_SERIALQ]);
        DEBUGMSGTL(("fcahc:container:load", "Getting attribute from %s\n",
                    attribute));
        if ((value = get_sysfs_str(attribute)) == NULL) {
            strcpy(attribute, buffer);
            if ((strlen(buffer) + strlen(sysfs_attr[DEVICE_SERIALB])) >= 1024)
                return (char*) 0;
            strcat(attribute, sysfs_attr[DEVICE_SERIALB]);
            DEBUGMSGTL(("fcahc:container:load", "Getting attribute from %s\n",
                    attribute));
            value = get_sysfs_str(attribute);
        }
    } 
    return value;
}

char * getFcHBAFWVer(char *buffer)
{
    char attribute[1024];
    char *value = NULL;

    if (strlen(buffer) >= 1024)
        return (char*) 0;
    strcpy(attribute, buffer);
    if ((strlen(buffer) + strlen(sysfs_attr[CLASS_VERSION_FWQ])) >= 1024)
        return (char*) 0;
    strcat(attribute, sysfs_attr[CLASS_VERSION_FWQ]);
    DEBUGMSGTL(("fcahc:container:load", "Getting attribute from %s\n",
                    attribute));
    if ((value = get_sysfs_str(attribute)) == NULL) {
        strcpy(attribute, buffer);
        if ((strlen(sysfs_attr[CLASS_VERSION_FWE]) + strlen(buffer)) >= 1024)
            return (char*) 0;
        strcat(attribute, sysfs_attr[CLASS_VERSION_FWE]);
        DEBUGMSGTL(("fcahc:container:load", "Getting attribute from %s\n",
                    attribute));
        if ((value = get_sysfs_str(attribute)) == NULL) {
            strcpy(attribute, buffer);
            if ((strlen(sysfs_attr[CLASS_VERSION_FWB]) + strlen(buffer)) >= 1024)
                return (char*) 0;
            strcat(attribute, sysfs_attr[CLASS_VERSION_FWB]);
            DEBUGMSGTL(("fcahc:container:load", "Getting attribute from %s\n",
                    attribute));
            value = get_sysfs_str(attribute);
        }
    }
    return value;
}

char * getFcHBARomVer( char *buffer)
{
    char attribute[1024];
    char *value = NULL;

    if (strlen(buffer) >= 1024)
        return (char*) 0;
    strcpy(attribute, buffer);
    if ((strlen(buffer) + strlen(sysfs_attr[CLASS_VERSION_OPTIONROMBE])) >= 1024)
        return (char*) 0;
    strcat(attribute, sysfs_attr[CLASS_VERSION_OPTIONROMBE]);
    DEBUGMSGTL(("fcahc:container:load", "Getting attribute from %s\n",
                    attribute));
    if ((value = get_sysfs_str(attribute)) == NULL) {
        strcpy(attribute, buffer);
        if ((strlen(buffer) + strlen(sysfs_attr[CLASS_VERSION_OPTIONROMQ]))  >= 1024)
            return (char*) 0;
        strcat(attribute, sysfs_attr[CLASS_VERSION_OPTIONROMQ]);
        DEBUGMSGTL(("fcahc:container:load", "Getting attribute from %s\n",
                    attribute));
        value = get_sysfs_str(attribute);
    }
    return value;
}

int netsnmp_arch_fcahc_container_load(netsnmp_container* container)
{

    cpqFcaHostCntlrTable_entry *entry;
    cpqFcaHostCntlrTable_entry *old;

    netsnmp_index tmp;
    oid oid_index[2];

    int FcaIndex, Host;
    char buffer[256];
    char attribute[256];
    char *value;
    long  rc = 0;
    int i;

    DEBUGMSGTL(("fcahc:container:load", "loading\n"));

    DEBUGMSGTL(("fcahc:container:load", "Container=%p\n",container));

    FcaCondition = FC_HBA_CONDITION_OK;

    /* Find all FC HB's */
    if ((NumFcaHost = 
                scandir(FcaHostDir, &FcaHostlist, file_select, alphasort)) <= 0)
        return -1;

    /* Now chack for those HBA's in  the SCSI hosts */
    NumScsiHost = scandir(ScsiHostDir, &ScsiHostlist, fca_select, alphasort);
    /* We're done with FcaHost so lets clean it up */
    for (i = 0; i < NumFcaHost; i++) 
        free(FcaHostlist[i]);
    free(FcaHostlist);

    if (NumScsiHost <= 0)
        /* Should not happen */
        return -1;

    for (i=0; i < NumScsiHost; i++) {
        unsigned int vendor, device, BoardID = 0;

        /*  We will need the host name later on */
        sscanf(ScsiHostlist[i]->d_name, "host%d", &Host); 

        FcaIndex = Host;

        oid_index[0] = FcaIndex;
        tmp.len = 1;
        tmp.oids = &oid_index[0];
        DEBUGMSGTL(("fcahc:container:load", "looking for FCHBA = %d\n", FcaIndex));
        old = CONTAINER_FIND(container, &tmp);

        DEBUGMSGTL(("fca:container:load", "Old entry=%p\n", old));
        if (old != NULL ) {
           DEBUGMSGTL(("cpqnic:container:load", "Re-Use old entry\n"));
           old->oldStatus = old->cpqFcaHostCntlrStatus;
           entry = old;
        } else {
           DEBUGMSGTL(("cpqnic:container:load", "Create new entry\n"));
           /*create one entry for per port on the same Adapter! */
            entry = cpqFcaHostCntlrTable_createEntry(container, (oid)FcaIndex);
            if (NULL == entry)
                continue;   /* error already logged by func */
            DEBUGMSGTL(("fcahc:container:load", "Entry created\n"));

            sprintf(entry->host,"%d:", Host);
            entry->cpqFcaHostCntlrIndex = FcaIndex;
            entry->cpqFcaHostCntlrCondition = FC_HBA_CONDITION_OK;
            entry->cpqFcaHostCntlrOverallCondition = FC_HBA_CONDITION_OK;
            entry->cpqFcaHostCntlrStatus = FC_HBA_STATUS_OK;
            entry->oldStatus = FC_HBA_STATUS_OTHER;
     
            memset(buffer, 0, sizeof(buffer));
            strncpy(buffer, ScsiHostDir, sizeof(buffer) - 1);
            strncat(buffer, ScsiHostlist[i]->d_name, 
                    sizeof(buffer) - strlen(buffer) - 1);

            strcpy(attribute, buffer);
            entry->cpqFcaHostCntlrSlot = pcislot_scsi_host(attribute);

            if (entry->cpqFcaHostCntlrSlot > 0 ) {
                DEBUGMSGTL(("fcahc:container:load", "Got pcislot info for %s = %u\n",
                            buffer,
                            entry->cpqFcaHostCntlrSlot));
    
                if (entry->cpqFcaHostCntlrSlot > 0x1000)
                    entry->cpqFcaHostCntlrHwLocation_len =
                        snprintf(entry->cpqFcaHostCntlrHwLocation,
                                256,
                                "Blade %u, Slot %u",
                                (entry->cpqFcaHostCntlrSlot>>8) & 0xF,
                                (entry->cpqFcaHostCntlrSlot>>16) & 0xFF);
                else
                    entry->cpqFcaHostCntlrHwLocation_len =
                        snprintf(entry->cpqFcaHostCntlrHwLocation,
                                256,
                                "Slot %u",
                                entry->cpqFcaHostCntlrSlot);
            }
    
            strcpy(attribute, buffer);
            strncat(attribute, sysfs_attr[DEVICE_SUBSYSTEM_DEVICE], 
                    sizeof(attribute) - strlen(attribute) - 1);
            DEBUGMSGTL(("fcahc:container:load", "Getting attribute from %s\n",
                            attribute));
            device = get_sysfs_shex(attribute);
            DEBUGMSGTL(("fcahc:container:load", "Value = %x\n", device));

            strcpy(attribute, buffer);
            strncat(attribute, sysfs_attr[DEVICE_SUBSYSTEM_VENDOR],
                    sizeof(attribute) - strlen(attribute) - 1);
            DEBUGMSGTL(("fcahc:container:load", "Getting attribute from %s\n",
                        attribute));
            vendor = get_sysfs_shex(attribute);
            DEBUGMSGTL(("fcahc:container:load", "Value = %x\n", vendor));

            BoardID = device << 16;
            BoardID += vendor;

            entry->cpqFcaHostCntlrModel = enumPCIid(BoardID);
    
            if ((value = getFcHBASerialNum(buffer)) != NULL) {
                DEBUGMSGTL(("fcahc:container:load", "Value = %s\n", value));
                strncpy(entry->cpqFcaHostCntlrSerialNumber, value,
                        sizeof(entry->cpqFcaHostCntlrSerialNumber) - 1);    
                entry->cpqFcaHostCntlrSerialNumber_len = 
                        strlen(entry->cpqFcaHostCntlrSerialNumber);
                free(value);
            }
    
            if ((value = getFcHBAFWVer(buffer)) != NULL) {
                DEBUGMSGTL(("fcahc:container:load", "Value = %s\n", value));
                strncpy(entry->cpqFcaHostCntlrFirmwareVersion, value,
                        sizeof(entry->cpqFcaHostCntlrFirmwareVersion) - 1);    
                entry->cpqFcaHostCntlrFirmwareVersion_len = 
                        strlen(entry->cpqFcaHostCntlrFirmwareVersion);
                free(value);
            } 
    
            if (entry->cpqFcaHostCntlrFirmwareVersion_len > 0) 
                other_fw_idx = register_FW_version(1, /* direction */
                                                other_fw_idx,
                                                2, /*CATEGORY*/
                                                4, /* type */
                                                3, /*update method */
                                                entry->cpqFcaHostCntlrFirmwareVersion,
                                                "FC HBA",
                                                entry->cpqFcaHostCntlrHwLocation,
                                                "");
    
            if ((value = getFcHBARomVer(buffer)) != NULL) {
                DEBUGMSGTL(("fcahc:container:load", "Value = %s\n", value));
                strncpy(entry->cpqFcaHostCntlrOptionRomVersion, value,
                        sizeof(entry->cpqFcaHostCntlrOptionRomVersion) - 1);    
                entry->cpqFcaHostCntlrOptionRomVersion_len = 
                                 strlen(entry->cpqFcaHostCntlrOptionRomVersion);
                free(value);
            }
            if (entry->cpqFcaHostCntlrOptionRomVersion_len > 0) 
                other_fw_idx = register_FW_version(1, /* direction */
                                                other_fw_idx,
                                                2, /*CATEGORY*/
                                                4, /* type */
                                                3, /*update method */
                                                entry->cpqFcaHostCntlrOptionRomVersion,
                                                "FC HBA (BIOS)",
                                                entry->cpqFcaHostCntlrHwLocation,
                                                "");
    
            memset(buffer, 0, sizeof(buffer));
            strncpy(buffer, FcaHostDir, sizeof(buffer) - 1);
            strncat(buffer, ScsiHostlist[i]->d_name,
                    sizeof(buffer) - strlen(buffer) - 1);
    
            strcpy(attribute, buffer);
            strncat(attribute, sysfs_attr[FC_WWN],
                    sizeof(attribute) - strlen(attribute) - 1);
            DEBUGMSGTL(("fcahc:container:load", "Getting attribute from %s\n",
                            attribute));
            if ((value = get_sysfs_str(attribute)) != NULL) {
                DEBUGMSGTL(("fcahc:container:load", "Value = %s\n", value));
                memcpy(entry->cpqFcaHostCntlrWorldWideName, &value[2], 
                        sizeof(entry->cpqFcaHostCntlrWorldWideName));
                entry->cpqFcaHostCntlrWorldWideName_len = 
                        sizeof(entry->cpqFcaHostCntlrWorldWideName);
                free(value);
            }
    
            strcpy(attribute, buffer);
            strncat(attribute, sysfs_attr[FC_WWNPORT],
                    sizeof(attribute) - strlen(attribute) - 1);
            DEBUGMSGTL(("fcahc:container:load", "Getting attribute from %s\n",
                            attribute));
            if ((value = get_sysfs_str(attribute)) != NULL) {
                DEBUGMSGTL(("fcahc:container:load", "Value = %s\n", value));
                memcpy(entry->cpqFcaHostCntlrWorldWidePortName, &value[2], 
                        sizeof(entry->cpqFcaHostCntlrWorldWidePortName));
                entry->cpqFcaHostCntlrWorldWidePortName_len =
                        sizeof(entry->cpqFcaHostCntlrWorldWidePortName);
                free(value);
            }
    
            rc = CONTAINER_INSERT(container, entry);
            DEBUGMSGTL(("fcahc:container:load", "container inserted\n"));
        }
    
#if RHEL_MAJOR == 5
        /* Older kernels show link state in /sys/class/scsi_host/host?/state
           so assume things are running if we are talking to the HBA */
        entry->cpqFcaHostCntlrStatus = FC_HBA_STATUS_OK;
#else
        memset(buffer, 0, sizeof(buffer));
        strncpy(buffer, ScsiHostDir, sizeof(buffer) - 1);
        strncat(buffer, ScsiHostlist[i]->d_name,
                sizeof(buffer) - strlen(buffer) - 1);

        strcpy(attribute, buffer);
        strncat(attribute, sysfs_attr[CLASS_STATE],
                sizeof(attribute) - strlen(attribute) - 1);
        DEBUGMSGTL(("fcahc:container:load", "Getting attribute from %s\n",
                        attribute));
        if ((value = get_sysfs_str(attribute)) != NULL) {
            DEBUGMSGTL(("fcahc:container:load", "Value = %s\n", value));
                if (!strcmp(value, "running") || !strcmp(value, "Link down"))
                    entry->cpqFcaHostCntlrStatus = FC_HBA_STATUS_OK;
                else
                    entry->cpqFcaHostCntlrStatus = FC_HBA_STATUS_FAILED;
	        free(value);
        }
#endif
        memset(buffer, 0, sizeof(buffer));
        strncpy(buffer, FcaHostDir, sizeof(buffer) - 1);
        strncat(buffer, ScsiHostlist[i]->d_name,
                sizeof(buffer) - strlen(buffer) - 1);
    
        strcpy(attribute, buffer);
        strncat(attribute, sysfs_attr[FC_PORTSTATE],
                sizeof(attribute) - strlen(attribute) - 1);
        DEBUGMSGTL(("fcahc:container:load", "Getting attribute from %s\n",
                        attribute));
        if ((value = get_sysfs_str(attribute)) != NULL) {
            DEBUGMSGTL(("fcahc:container:load", "Value = %s\n", value));
            if (!strcmp(value, "Linkdown")) {
                if (entry->cpqFcaHostCntlrStatus == FC_HBA_STATUS_OK)
                    entry->cpqFcaHostCntlrStatus = FC_HBA_STATUS_NOTCONNECTED;
            } else if (!strcmp(value, "Bypassed")) {
                if (entry->cpqFcaHostCntlrStatus == FC_HBA_STATUS_OK)
                    entry->cpqFcaHostCntlrStatus = FC_HBA_STATUS_LOOPDEGRADED;
            } else if (!strcmp(value, "Error"))
                if (entry->cpqFcaHostCntlrStatus == FC_HBA_STATUS_OK)
                    entry->cpqFcaHostCntlrStatus = FC_HBA_STATUS_FAILED;
            free(value);
        }

        switch (entry->cpqFcaHostCntlrStatus) {
        case FC_HBA_STATUS_FAILED:
        case FC_HBA_STATUS_SHUTDOWN:
        case FC_HBA_STATUS_LOOPFAILED:
            entry->cpqFcaHostCntlrCondition = FC_HBA_CONDITION_FAILED;
            break;
        case FC_HBA_STATUS_LOOPDEGRADED:
            entry->cpqFcaHostCntlrCondition = FC_HBA_CONDITION_DEGRADED;
            break;
        case FC_HBA_STATUS_NOTCONNECTED:
        case FC_HBA_STATUS_OK:
            entry->cpqFcaHostCntlrCondition = FC_HBA_CONDITION_OK;
            break;
        default:
            entry->cpqFcaHostCntlrCondition = FC_HBA_CONDITION_OTHER;
        }

        if ((entry->cpqFcaHostCntlrStatus != entry->oldStatus) &&
            (entry->oldStatus != FC_HBA_STATUS_OTHER))  {
             DEBUGMSGTL(("fcahc:container:load", 
                         "SENDING FCA STATUS CHANGE TRAP status = %ld old = %ld\n", 
                         entry->cpqFcaHostCntlrStatus, entry->oldStatus));
             SendFcaTrap( FCA_TRAP_HOST_CNTLR_STATUS_CHANGE, entry );
             entry->oldStatus = entry->cpqFcaHostCntlrStatus; //save Old status
        }

        if (FcaCondition < entry->cpqFcaHostCntlrCondition)
            FcaCondition = entry->cpqFcaHostCntlrCondition;
        DEBUGMSGTL(("fcahc:", "FcaCondition = %d\n", FcaCondition));

        free(ScsiHostlist[i]);
    }
    free(ScsiHostlist);

    return(rc);
}


void SendFcaTrap(int trapID,     
        cpqFcaHostCntlrTable_entry *fca)
{
    static oid compaq[] = { 1, 3, 6, 1, 4, 1, 232 };
    static oid compaq_len = 7;
    static oid sysName[] = { 1, 3, 6, 1, 2, 1, 1, 5, 0 };
    static oid cpqHoTrapFlags[] = { 1, 3, 6, 1, 4, 1, 232, 11, 2, 11, 1, 0 };
    static oid cpqFcaHostCntlrHwLocation[] = 
           { 1, 3, 6, 1, 4, 1, 232, 16, 2, 7, 1, 1, 11 };
    static oid cpqFcaHostCntlrIndex[] =
           { 1, 3, 6, 1, 4, 1, 232, 16, 2, 7, 1, 1, 1 };
    static oid cpqFcaHostCntlrStatus[] =
           { 1, 3, 6, 1, 4, 1, 232, 16, 2, 7, 1, 1, 4 };
    static oid cpqFcaHostCntlrModel[] =
           { 1, 3, 6, 1, 4, 1, 232, 16, 2, 7, 1, 1, 3 };
    static oid cpqFcaHostCntlrSerialNumber[] =
           { 1, 3, 6, 1, 4, 1, 232, 16, 2, 7, 1, 1, 10 };
    static oid cpqFcaHostCntlrWorldWideName[] =
           { 1, 3, 6, 1, 4, 1, 232, 16, 2, 7, 1, 1, 6 };
    static oid cpqFcaHostCntlrWorldWidePortName[] =
           { 1, 3, 6, 1, 4, 1, 232, 16, 2, 7, 1, 1, 12 };


    netsnmp_variable_list *var_list = NULL;
    struct utsname sys_name;
    unsigned int cpqHoTrapFlag;
    cpqHoTrapFlag = fca->cpqFcaHostCntlrCondition << 2;

    uname(&sys_name);   /* get sysName */

    snmp_varlist_add_variable(&var_list, sysName,
            sizeof(sysName) / sizeof(oid),
            ASN_OCTET_STR,
            (u_char *) sys_name.nodename,
            strlen(sys_name.nodename));

    snmp_varlist_add_variable(&var_list, cpqHoTrapFlags,
            sizeof(cpqHoTrapFlags) / sizeof(oid),
            ASN_INTEGER, &cpqHoTrapFlag,
            sizeof(ASN_INTEGER));

    snmp_varlist_add_variable(&var_list, cpqFcaHostCntlrHwLocation,
            sizeof(cpqFcaHostCntlrHwLocation) / sizeof(oid),
            ASN_OCTET_STR,
            (u_char *) fca->cpqFcaHostCntlrHwLocation,
            fca->cpqFcaHostCntlrHwLocation_len);

    snmp_varlist_add_variable(&var_list, cpqFcaHostCntlrIndex,
            sizeof(cpqFcaHostCntlrIndex) / sizeof(oid),
            ASN_INTEGER, (u_char *)&fca->cpqFcaHostCntlrIndex,
            sizeof(ASN_INTEGER));

    snmp_varlist_add_variable(&var_list, cpqFcaHostCntlrStatus,
            sizeof(cpqFcaHostCntlrStatus) / sizeof(oid),
            ASN_INTEGER,(u_char *)&fca->cpqFcaHostCntlrStatus,
            sizeof(ASN_INTEGER));

    snmp_varlist_add_variable(&var_list, cpqFcaHostCntlrModel,
            sizeof(cpqFcaHostCntlrModel) / sizeof(oid),
            ASN_INTEGER,(u_char *)&fca->cpqFcaHostCntlrModel,
            sizeof(ASN_INTEGER));

   snmp_varlist_add_variable(&var_list, cpqFcaHostCntlrSerialNumber,
            sizeof(cpqFcaHostCntlrSerialNumber) / sizeof(oid),
            ASN_OCTET_STR,
            (u_char *) fca->cpqFcaHostCntlrSerialNumber,
            fca->cpqFcaHostCntlrSerialNumber_len);

    snmp_varlist_add_variable(&var_list, cpqFcaHostCntlrWorldWideName,
            sizeof(cpqFcaHostCntlrWorldWideName) / sizeof(oid),
            ASN_OCTET_STR,
            (u_char *) fca->cpqFcaHostCntlrWorldWideName,
            fca->cpqFcaHostCntlrWorldWideName_len);

    snmp_varlist_add_variable(&var_list, cpqFcaHostCntlrWorldWidePortName,
            sizeof(cpqFcaHostCntlrWorldWidePortName) / sizeof(oid),
            ASN_OCTET_STR,
            (u_char *) fca->cpqFcaHostCntlrWorldWidePortName,
            fca->cpqFcaHostCntlrWorldWidePortName_len);

    switch(trapID)
    {
        case FCA_TRAP_HOST_CNTLR_STATUS_CHANGE:

            netsnmp_send_traps(SNMP_TRAP_ENTERPRISESPECIFIC,
                    FCA_TRAP_HOST_CNTLR_STATUS_CHANGE,
                    compaq,
                    compaq_len,
                    var_list,
                    NULL,
                    0);

            DEBUGMSGTL(("fcahc:", "Free varbind list...\n"));
            snmp_free_varbind(var_list);
            DEBUGMSGTL(("fcahc:", "Done freeing varbind list...\n"));

            break;

        default:
            return;
    }
}


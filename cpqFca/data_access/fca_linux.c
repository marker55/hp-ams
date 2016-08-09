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

#include "fcahc.h"
#include "common/smbios.h"
#include "common/utils.h"
#include "common/scsi_info.h"

#include "fca_linux.h"
#include "fca_db.h"

extern unsigned char     cpqHoMibHealthStatusArray[];
extern int trap_fire;

extern int file_select(const struct dirent *);
extern int pcislot_scsi_host(char * buffer);
void SendFcaTrap(int, cpqFcaHostCntlrTable_entry *);
void netsnmp_arch_fcahc_init(void); 
int  getFcaHostCntlrModel( int BoardID, char *pHPModelName, int *pFCoE );

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
                    strlen(FcaHostlist[i]->d_name)) == 0)
            return(1);
    }
    return 0;
}

/***************************************************************************/
/*  Function: SetHbaModelName                                              */
/*                                                                         */
/*  Description:                                                           */
/*    This function determines the HBA model based on board id and HBA API */
/*    model information.                                                   */
/*    Also returns info if device is fcoe or not   */
/*                                                                         */
/***************************************************************************/
int getFcaHostCntlrModel( int BoardID, char *pHPModelName, int *pFCoE )
{
   int fcaModel = 0, i, fcoe = 0;

   DEBUGMSGTL(("fcahc:container:load","getFcaHostCntlrModel:  BoardID is 0x%X\n", BoardID));

   /* Loop through the list of HBA models to find this one. */
   for (i = 0; i < MAX_FCA_HBA_LIST; i++) {
      if (BoardID == gFcaHbaList[i].ulBoardId) {
            fcaModel = gFcaHbaList[i].bRegModel;
            if( strstr(gFcaHbaList[i].szHbaHPModel, "Converged")!=NULL )
                fcoe = 1;
            else {
                if( strstr(gFcaHbaList[i].szHbaHPModel, "Flex")!=NULL )
                    fcoe = 1;
            }
            if( pHPModelName )
                strcpy( pHPModelName, gFcaHbaList[i].szHbaHPModel );
            if( pFCoE )
                *pFCoE = fcoe;
            DEBUGMSGTL(("fcahc:container:load","   Found matching FCHBA.  Returning FCA Model [ %d]  and fcoe flag [%d]\n", fcaModel, fcoe));
            return ( fcaModel );
      } /* if */
   } /* for */

   fcaModel = FCA_HOST_MODEL_FC_GENERIC;
   if( pHPModelName )
       sprintf( pHPModelName, "Generic FCHBA Model" );
   DEBUGMSGTL(("fcahc:container:load","   Matching FCHBA Not Found.  Returning FCA Model   %d\n", fcaModel));

   return (fcaModel);

} /*  */

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

long cpqfca_update_condition(long status) 
{
    switch (status) {
        case FC_HBA_STATUS_FAILED:
        case FC_HBA_STATUS_SHUTDOWN:
        case FC_HBA_STATUS_LOOPFAILED:
            return FC_HBA_CONDITION_FAILED;
            break;
        case FC_HBA_STATUS_LOOPDEGRADED:
            return FC_HBA_CONDITION_DEGRADED;
            break;
        case FC_HBA_STATUS_NOTCONNECTED:
        case FC_HBA_STATUS_OK:
            return FC_HBA_CONDITION_OK;
            break;
        default:
            return FC_HBA_CONDITION_OTHER;
    }
}

long cpqfca_update_status( char * value, long status)
{
    if (!strcmp(value, "Online")) 
        return FC_HBA_STATUS_OK;
    if (!strcmp(value, "Linkdown")) 
        if (status == FC_HBA_STATUS_OK)
            return FC_HBA_STATUS_NOTCONNECTED;
    if (!strcmp(value, "Bypassed")) 
        if (status == FC_HBA_STATUS_OK)
            return FC_HBA_STATUS_LOOPDEGRADED;
    if (!strcmp(value, "Error"))
        if (status == FC_HBA_STATUS_OK)
            return FC_HBA_STATUS_FAILED;
    return FC_HBA_STATUS_OTHER;
}

void cpqfca_update_hba_status(char *devpath, char *devname, void *data)
{
    cpqFcaHostCntlrTable_entry *entry;
    cpqFcaHostCntlrTable_entry *old;

    char * rport;
    char buffer[1024];
    size_t buf_len = 1024;
    int NumHost;
    static struct dirent **Hostlist;
    char *port_state;
    long status;

    netsnmp_index tmp;
    oid oid_index[2];

    int FcaIndex, Host;

    DEBUGMSGTL(("fcahc:container:load", "devpath = %s\n", devpath));
    if (devpath == NULL)
        return;
    if ((rport = strstr(devpath, "rport-")) != NULL) {
        memset(buffer, 0, buf_len);
        strcpy(buffer, "/sys");
        strncat(buffer, devpath,  (size_t) (rport - devpath));
        strcat(buffer, "fc_host/");
        if ((NumHost =
            scandir(buffer, &Hostlist, file_select, alphasort)) == 1) {
            sscanf(Hostlist[0]->d_name, "host%d", &Host); 

            FcaIndex = Host;

            oid_index[0] = FcaIndex;
            tmp.len = 1;
            tmp.oids = &oid_index[0];
            DEBUGMSGTL(("fcahc:container:load", "looking for FCHBA = %d\n", FcaIndex));

            old = CONTAINER_FIND((netsnmp_container *)data, &tmp);

            DEBUGMSGTL(("fcahc:container:load", "Old entry=%p\n", old));
            strcat(buffer, Hostlist[0]->d_name);
            strcat(buffer, "/port_state");
            DEBUGMSGTL(("fcahc:container:load", "port_state = %s\n", buffer));
            if (old == NULL ) return;
            old->oldStatus = old->cpqFcaHostCntlrStatus;
            entry = old;
            if ((port_state = get_sysfs_str(buffer)) != NULL) {
                status =
                    cpqfca_update_status(port_state, entry->cpqFcaHostCntlrStatus);
                if ((status != FC_HBA_STATUS_OTHER) && (status != entry->cpqFcaHostCntlrStatus))
                    entry->cpqFcaHostCntlrStatus = status;

                free(port_state);

            }
            entry->cpqFcaHostCntlrCondition = 
                cpqfca_update_condition(entry->cpqFcaHostCntlrStatus);

            if ((entry->cpqFcaHostCntlrStatus != entry->oldStatus) &&
                (entry->oldStatus != FC_HBA_STATUS_OTHER))  {
                DEBUGMSGTL(("fcahc:container:load", 
                    "SENDING FCA STATUS CHANGE TRAP status = %ld old = %ld\n", 
                    entry->cpqFcaHostCntlrStatus, entry->oldStatus));
                SendFcaTrap( FCA_TRAP_HOST_CNTLR_STATUS_CHANGE, entry );
                entry->oldStatus = entry->cpqFcaHostCntlrStatus; //save Old status
            }
            if (trap_fire)
                SendFcaTrap( FCA_TRAP_HOST_CNTLR_STATUS_CHANGE, entry );

            if (FcaCondition < entry->cpqFcaHostCntlrCondition)
                FcaCondition = entry->cpqFcaHostCntlrCondition;
            DEBUGMSGTL(("fcahc:", "FcaCondition = %d\n", FcaCondition));

            free(Hostlist[0]);
            free(Hostlist);
        }
    }
    DEBUGMSGTL(("fcahc:container:load", "cpqfca_update_hba_status() exiting\n"));
}

int netsnmp_arch_fcahc_container_load(netsnmp_container* container)
{

    cpqFcaHostCntlrTable_entry *entry;
    cpqFcaHostCntlrTable_entry *old;

    netsnmp_index tmp;
    oid oid_index[2];

    int FcaIndex, Host;
    char hpModelName[128];
    int is_fcoe;
    char buffer[256];
    char attribute[256];
    char *value;
    long  rc = 0;
    int i;
    char    linkBuf[1024];
    ssize_t link_sz;

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

            entry->cpqFcaHostCntlrHwLocation_len = 0;
            entry->cpqFcaHostCntlrHwLocation[0] = '\0';
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
            }
    
            strcpy(attribute, buffer);
            strncat(attribute, sysfs_attr[DEVICE_SUBSYSTEM_DEVICE], 
                    sizeof(attribute) - strlen(attribute) - 1);
            DEBUGMSGTL(("fcahc:container:load", "Getting attribute from %s\n",
                            attribute));
            device = get_sysfs_shex(attribute);
            DEBUGMSGTL(("fcahc:container:load", "Value = %x\n", device));
            if ((unsigned short) device != 0xffff ) {
            strcpy(attribute, buffer);
            strncat(attribute, sysfs_attr[DEVICE_SUBSYSTEM_VENDOR],
                    sizeof(attribute) - strlen(attribute) - 1);
            DEBUGMSGTL(("fcahc:container:load", "Getting attribute from %s\n",
                        attribute));
            vendor = get_sysfs_shex(attribute);
            DEBUGMSGTL(("fcahc:container:load", "Value = %x\n", vendor));
            } else {
                unsigned char * start;
                unsigned char * end;
                unsigned char pcidevice[256];
                int j;
                
                /* this is possible a converged NIC look elsewere */

                DEBUGMSGTL(("fcahc:container:load", "buffer is  %s\n", buffer));
                if ((link_sz = readlink(buffer, linkBuf, 1024)) < 0) {
                    return NULL;
                }
                linkBuf[link_sz] = 0;
                start = strrchr(linkBuf, '/');
                while (start != NULL) {
                    *start = 0;
                    if (!strncmp(start + 1, "0000:", 5)) {
                        strcpy(attribute, "/sys/bus/pci/devices/");
                        strcat(attribute, start + 1);
                        start = NULL;
                        continue;
                    } else {
                        start = strrchr(linkBuf, '/');
                    }
                }

                DEBUGMSGTL(("fcahc:container:load", "attribute is %s\n",
                        attribute));
                strcpy(pcidevice, attribute);
                strcat(attribute, "/subsystem_device");
                DEBUGMSGTL(("fcahc:container:load", "Getting attribute from %s\n",
                        attribute));
                device = get_sysfs_shex(attribute);
                DEBUGMSGTL(("fcahc:container:load", "Value = %x\n", device));
                strcat(pcidevice, "/subsystem_vendor");
                DEBUGMSGTL(("fcahc:container:load", "Getting attribute from %s\n",
                        pcidevice));
                vendor = get_sysfs_shex(pcidevice);
                DEBUGMSGTL(("fcahc:container:load", "Value = %x\n", vendor));
            }

            BoardID = device << 16;
            BoardID += vendor;

            entry->cpqFcaHostCntlrModel = 
                getFcaHostCntlrModel(BoardID, hpModelName, &is_fcoe );
    
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
    
            if ((value = getFcHBARomVer(buffer)) != NULL) {
                DEBUGMSGTL(("fcahc:container:load", "Value = %s\n", value));
                strncpy(entry->cpqFcaHostCntlrOptionRomVersion, value,
                        sizeof(entry->cpqFcaHostCntlrOptionRomVersion) - 1);    
                entry->cpqFcaHostCntlrOptionRomVersion_len = 
                                 strlen(entry->cpqFcaHostCntlrOptionRomVersion);
                free(value);
            }
    
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
            entry->cpqFcaHostCntlrStatus = 
                cpqfca_update_status(value, entry->cpqFcaHostCntlrStatus);
            free(value);
        }
        entry->cpqFcaHostCntlrCondition = 
                    cpqfca_update_condition(entry->cpqFcaHostCntlrStatus);

        if ((entry->cpqFcaHostCntlrStatus != entry->oldStatus) &&
            (entry->oldStatus != FC_HBA_STATUS_OTHER))  {
             DEBUGMSGTL(("fcahc:container:load", 
                        "SENDING FCA STATUS CHANGE TRAP status = %ld old = %ld\n", 
                        entry->cpqFcaHostCntlrStatus, entry->oldStatus));
             SendFcaTrap( FCA_TRAP_HOST_CNTLR_STATUS_CHANGE, entry );
             entry->oldStatus = entry->cpqFcaHostCntlrStatus; //save Old status
        }
        if (trap_fire)
            SendFcaTrap( FCA_TRAP_HOST_CNTLR_STATUS_CHANGE, entry );

        if (FcaCondition < entry->cpqFcaHostCntlrCondition)
            FcaCondition = entry->cpqFcaHostCntlrCondition;
        DEBUGMSGTL(("fcahc:", "FcaCondition = %d\n", FcaCondition));

        free(ScsiHostlist[i]);
    }
    free(ScsiHostlist);

    DEBUGMSGTL(("fcahc:container:load", "exiting\n"));
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
    if (trap_fire)
        cpqHoTrapFlag = trap_fire << 2;

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

            send_enterprise_trap_vars(SNMP_TRAP_ENTERPRISESPECIFIC,
                    FCA_TRAP_HOST_CNTLR_STATUS_CHANGE,
                    compaq,
                    compaq_len,
                    var_list);

            DEBUGMSGTL(("fcahc:", "Free varbind list...\n"));
            snmp_free_varbind(var_list);
            DEBUGMSGTL(("fcahc:", "Done freeing varbind list...\n"));

            break;

        default:
            return;
    }
}


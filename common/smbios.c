
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
/* #include "convert.h" */


#include "smbios.h"

static int fSMBiosInited = 0;
static SMBIOSEntryPoint EPS;
static u_char * pSMBTables = NULL;

/*
 *  * Probe for EFI interface
 *   */
#define EFI_NOT_FOUND   (-1)
#define EFI_NO_SECTION   (-2)
static int address_from_efi(char *section, size_t *address)
{
    FILE *systab;
    char linebuf[64];
    int ret;

    *address = 0; /* Prevent compiler warning */

    /*
     * Linux up to 2.6.6: /proc/efi/systab
     * Linux 2.6.7 and up: /sys/firmware/efi/systab
     */
    if ((systab = fopen("/sys/firmware/efi/systab", "r")) == NULL
             && (systab = fopen("/proc/efi/systab", "r")) == NULL) {
        /* No EFI interface, fallback to memory scan */
        return EFI_NOT_FOUND;
    }
    ret = EFI_NO_SECTION;
    while ((fgets(linebuf, sizeof(linebuf) - 1, systab)) != NULL) {
        char *addrp = strchr(linebuf, '=');
        if (addrp != NULL) {
            *(addrp++) = '\0';
            if (strcmp(linebuf, section) == 0) {
                *address = strtoul(addrp, NULL, 0);
                ret = 0;
                break;
            }
        }
    }
    fclose(systab);

    return ret;
}

/*
 *  Open a file.
 *
 *	input:
 *		file : file to open
 *
 *	output
 *		fd   : file descriptor that is returned from open call
 *
 *  returns 0 if passed, -1 if a problem occured
 */
int open_file(char *file, int oflag, int *fd) {
    int tryOpen = 0;
    while (((*fd = open(file, oflag)) == -1) && (++tryOpen <= 5)) {
        sleep(5);
    } if (*fd == -1) {
        fprintf(stderr, "ERROR: Failed to open %s\n", file);
        return (-1);
    }

    return 0;
}

/*
 *  Read from a file.
 *
 *	input:
 *		fd     : file descriptor that is returned from open call
 *		offset : place to start mapping in file
 *		size   : amount of data to map
 *
 *	output
 *		buffer : buffer to store data
 *
 *  returns 0 if passed, -1 if a problem occured
 */
int read_buf(int fd, long offset, long size, u_char *buf) {
    int psize;
    unsigned char *mapptr;
    long mapoff, mapsize, poff;

    /* align size, offset to page boundaries */
    psize = getpagesize();
    mapoff = offset & (~(psize - 1));
    poff = offset & (psize - 1);
    mapsize = (size + poff + psize - 1) & (~(psize - 1));

    mapptr =
        (u_char *)mmap(0, mapsize, PROT_READ, MAP_SHARED, fd,
                mapoff);
    if (mapptr == (void *)-1) {
        fprintf(stderr, "ERROR: Failed to map 0x%lx 0x%lx\n", offset, size);
        return -1;
    }
    memcpy(buf, mapptr + poff, size);
    munmap(mapptr, mapsize);
    return 0;
}

/*
 *  Get a buffer from system memory.
 *	input:
 *		offset : offset into file
 *		size   : length of data to read
 *	output:
 *		buffer : buffer to store data in.  Returned to call.
 *  returns 0 if passed, -1 if a problem occured
 */
int ReadPhysMem(u_int offset, u_int size, u_char * buffer) {
    int fd;
    int status = 0;

    if (open_file("/dev/mem", O_RDONLY, &fd) == 0) {
        if (read_buf(fd, offset, size, buffer) == 0)
            status = 1;
        close(fd);
    }
    return status;
}

/*
 *  Function:
 *     InitSMBIOS
 *
 *  Description:
 *     Initializes the SMBIOS tables by reading physical memory.  All records
 *     are read in at once, and parsed later.
 *
 *  Return:
 *     1 if inited correctly; 0 otherwise
 */
int InitSMBIOS() {
    u_char * pBuf = NULL;
    size_t fp, count;
    u_char* p;
    int efi;

    SMBIOSEntryPoint *pEPS = NULL;

    if ((pBuf = malloc(SMBIOS_EPS_SCAN_SIZE)) == NULL)
        return fSMBiosInited;

        /* First try EFI (ia64, Intel-based Mac) */
    if ((efi = address_from_efi("SMBIOS", &fp)) >= 0) {
        count = 0x20;
    } else {
        if (efi == EFI_NO_SECTION) {
            free(pBuf);
            return 0;
        }

        count = SMBIOS_EPS_SCAN_SIZE;
        fp = SMBIOS_EPS_ADDR_START;
    }

    
    if (ReadPhysMem(fp, count, pBuf)) {

        /* scan 0xF0000 through 0xFFFFF for an EPS (Entry Point Structure) */
        for (p = pBuf; p < pBuf + SMBIOS_EPS_SCAN_SIZE;
                p += SMBIOS_EPS_BOUNDRY) {
            /* check for a possible EPS.
             * we look for a valid signature and checksum */
            pEPS = (SMBIOSEntryPoint *) p;
            if ((strncmp((const char *)pEPS->sAnchorString,
                            SMBIOS_ANCHOR_STRING,
                            sizeof(pEPS->sAnchorString))) ||
                    (SmbChecksum((u_char*) pEPS, pEPS->byEPSLength)) ||
                    (((pEPS->byMajorVer << 8) + pEPS->byMinorVer) < 0x0201))
                continue;

            /* we have found a valid EPS (entry point structure) */
            memcpy(&EPS, pEPS, sizeof(EPS));
            if ((pSMBTables = malloc(EPS.wSTLength)) != NULL) {
                if (ReadPhysMem(EPS.dwSTAddr, EPS.wSTLength, pSMBTables))
                    fSMBiosInited = 1;
            }
            break;
        }
    }
    free(pBuf);
    return fSMBiosInited;
}

/*
 *  Function:
 *     DeinitSMBIOS
 *
 *  Description:
 *     Free SMBIOS tables.
 *
 *  Return:
 *     None
 */
void DeinitSMBIOS() {
    fSMBiosInited = 0;
    if (pSMBTables)
        free(pSMBTables);
    pSMBTables = NULL;
}

/*
 *  Function:
 *     ReinitSMBIOS
 *
 *  Description:
 *     Re-initialize SMBIOS tables.
 *
 *  Return:
 *     1 if inited; 0 otherwise
 */
int ReinitSMBIOS() {
    if (fSMBiosInited)
        DeinitSMBIOS();

    return InitSMBIOS();
}

/*
 *  Function:
 *     IsSMBIOSAvailable
 *
 *  Description:
 *     Check if SMBIOS tables are initialized.
 *
 *  Return:
 *     1 if inited; 0 otherwise
 */
int IsSMBIOSAvailable() {
    return fSMBiosInited;
}

/*
 *  Function:
 *     SmbChecksum
 *
 *  Description:
 *     Compute checksum of specified memory.
 *
 *  Return:
 *     Byte checksum value (0 assumed as valid for SMBIOS)
 */
u_char SmbChecksum(u_char* pAddr, u_short wCount) {
    u_char byChecksum = 0;
    u_short wIndex;

    for (wIndex = 0; wIndex < wCount; wIndex++) {
        byChecksum = byChecksum + *pAddr;
        pAddr++;
    }

    return byChecksum;
}

int SmbGetRecordByType(u_char byType, u_short wCopy, void * * ppRecord) {
    u_char* pSaveState = NULL;
    PSMBIOS_HEADER pHeader;

    /* copy is zero based */

    while (SmbGetRecord(&pSaveState)) {
        /* is it the right type? */
        pHeader = (PSMBIOS_HEADER) pSaveState;
        if (pHeader->byType == byType) {
            /* it's the right type.  is it the right copy? */
            if (wCopy == 0) {
                *ppRecord = (void *) pSaveState;
                return 1;
            } else
                wCopy--;
        }
    }

    return 0;
}

/*
 *  Function:
 *     SmbGetRecordByHandle
 *
 *  Description:
 *     Get an SMBIOS record by its unique handle.
 *
 *  Return:
 *     1 if record found; FASLE otherwise
 */
int SmbGetRecordByHandle(u_short wHandle, void * * ppRecord) {
    u_char* pSaveState = NULL;
    PSMBIOS_HEADER pHeader;

    while (SmbGetRecord(&pSaveState)) {
        /* is it the right handle? */
        pHeader = (PSMBIOS_HEADER) pSaveState;
        if (pHeader->wHandle == wHandle) {
            *ppRecord = pSaveState;
            return 1;
        }
    }

    return 0;
}

/*
 *  Function:
 *     SmbGetRecordByNumber
 *
 *  Description:
 *     Get an SMBIOS record by number.
 *
 *  Return:
 *     1 if record found; FASLE otherwise
 */
int SmbGetRecordByNumber(u_short wNumber, void * * ppRecord) {
    u_char* pSaveState = NULL;
    u_short wIndex = 0;

    while (SmbGetRecord(&pSaveState)) {
        /* is it the right record? */
        if (wIndex == wNumber) {
            *ppRecord = (void *) pSaveState;
            return 1;
        }

        wIndex++;
    }

    return 0;
}

/*
 *  Function:
 *     SmbGetRecord
 *
 *  Description:
 *     Get next SMBIOS record.  If ppRecord contains a null, the start
 *     at the beginning.
 *
 *  Return:
 *     1 if record found; FASLE otherwise
 */
int SmbGetRecord(u_char* * ppRecord) {
    u_char* pBuffer;
    PSMBIOS_HEADER pHeader;

    if (*ppRecord == 0) {
        /* find first */
        *ppRecord = pSMBTables;
        pBuffer = pSMBTables;
    } else {
        /* find next */
        pHeader = (PSMBIOS_HEADER) * ppRecord;
        pBuffer = *ppRecord;

        /* skip over the structure itself */
        *ppRecord += pHeader->byLength;
        pBuffer += pHeader->byLength;

        /* skip over any strings following the structure */
        while (*pBuffer || *(pBuffer + 1)) {
            pBuffer++;
        }

        /* skip over the double NUL characters */
        pBuffer += 2;
    }

    if (pBuffer < (pSMBTables + EPS.wSTLength)) {
        *ppRecord = pBuffer;
        return 1;
    } else
        return 0;
}

/*
 *  Function:
 *     SmbGetStringByNumber
 *
 *  Description:
 *     Get an SMBIOS string based on the string index.  All strings are at the
 *     end of the record as a series of null terminated strings, terminated
 *     by a double null.
 *
 *  Return:
 *     Pointer to string
 */
u_char* SmbGetStringByNumber(void * pRecord, u_short wString) {
    u_short i;
    u_char* pBuffer = (u_char*) pRecord;
    PSMBIOS_HEADER pHeader = (PSMBIOS_HEADER) pRecord;

    /* QXCR1000956799: cmastdeqd segfault observed on SL340z */
    /* string number 0 means no string */
    if (wString == 0)
        return (u_char *)"";

    /* make wString zero-based */
    wString--;

    /* skip over the structure itself */
    pBuffer += pHeader->byLength;

    for (i = 0; i < wString; i++)
        pBuffer += (strlen((const char *)pBuffer) + 1);

    return pBuffer;
}

/*
 *  Function:
 *     SmbGetSysGen
 *
 *  Description:
 *     Get the ProLiant G/Gen number of the Server
 *
 *  Return:
 *     int (generation #) 0 if not ProLiant
 */
int SmbGetSysGen() {
    char *ProductName;
    char *GenStr = "0";
    int len;
    PSMBIOS_SYSTEM_INFORMATION sysInfo;
    PCQSMBIOS_SERV_SYS_ID ServSysId;

    SmbGetRecordByType(SMBIOS_SYSTEM_INFO, 0, (void *)&sysInfo);

    ProductName = (char *)SmbGetStringByNumber(sysInfo, sysInfo->byProductName);
    if (!strncmp("ProLiant", ProductName, 8)){
        len = strlen(ProductName);
        while (len ) {
            if (isdigit(ProductName[len-1]))
                if ((len - 1) &&  ! (
                    (ProductName[len - 2] == 'v') || (ProductName[len - 2] == 'v')
                    )) {
                    if ((len - 1) && isdigit(ProductName[len - 2]))
                        GenStr = &ProductName[len - 2];
                    else
                        GenStr = &ProductName[len - 1];
                    return atoi(GenStr);
                }
            len--;
        }
        return 0;
    } else {
        if (!strncasecmp(SmbGetStringByNumber(sysInfo, sysInfo->byManufacturer), 
                         "H3C", 3))
            if (SmbGetRecordByType(SMBIOS_HPOEM_SYSID, 0, (void *)&ServSysId))
                if (!strncasecmp(SmbGetStringByNumber(ServSysId, 
                                                      ServSysId->serverSystemIdStr), 
                                 "$0E11", 5))
                    return (9);

    }
    return 0;
}

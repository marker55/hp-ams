#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <netdb.h>
#include <sys/time.h>
#include "utils.h"
#include "smbios.h"

long uptime()
{
    FILE           *in = fopen("/proc/uptime", "r");
    long            uptim = 0, a, b;
    if (in) {
        if (2 == fscanf(in, "%ld.%ld", &a, &b))
            uptim = a * 100 + b;
        fclose(in);
    }
    return uptim;
}

void init_etime(struct timeval the_time[]) {
    struct timeval *prev_time = &the_time[1];

    gettimeofday(prev_time, NULL);
}

int get_etime(struct timeval the_time[]) {
    struct timeval *curr_time = &the_time[0];
    struct timeval *prev_time = &the_time[1];
    struct timeval e_time;

    //fprintf(stderr, "the_time[0] = %p, the_time[1] = %p\n",&the_time[0],  &the_time[1]);
    gettimeofday(curr_time, NULL);
    
    //fprintf(stderr, "curr_time->tv_sec = %ld, curr_time->tv_usec= %ld, prev_time->tv_sec = %ld, prev_time->tv_usec = %ld\n",
    //                curr_time->tv_sec, curr_time->tv_usec, prev_time->tv_sec, prev_time->tv_usec);
    timersub(curr_time, prev_time, &e_time);
    //fprintf(stderr, "e_time->tv_sec = %ld, e_time->tv_usec= %ld\n",
    //                e_time.tv_sec, e_time.tv_usec);
    prev_time->tv_sec = curr_time->tv_sec;
    prev_time->tv_usec = curr_time->tv_usec;
    return (e_time.tv_sec*1000 + e_time.tv_usec/1000);
}

int get_uetime(struct timeval the_time[]) {
    struct timeval *curr_time = &the_time[0];
    struct timeval *prev_time = &the_time[1];
    struct timeval e_time;

    //fprintf(stderr, "the_time[0] = %p, the_time[1] = %p\n",&the_time[0],  &the_time[1]);
    gettimeofday(curr_time, NULL);
    
    //fprintf(stderr, "curr_time->tv_sec = %ld, curr_time->tv_usec= %ld, prev_time->tv_sec = %ld, prev_time->tv_usec = %ld\n",
    //                curr_time->tv_sec, curr_time->tv_usec, prev_time->tv_sec, prev_time->tv_usec);
    timersub(curr_time, prev_time, &e_time);
    //fprintf(stderr, "e_time->tv_sec = %ld, e_time->tv_usec= %ld\n",
    //                e_time.tv_sec, e_time.tv_usec);
    prev_time->tv_sec = curr_time->tv_sec;
    prev_time->tv_usec = curr_time->tv_usec;
    return (e_time.tv_sec*1000000 + e_time.tv_usec);
}

int get_nic_port(char * name)
{
    int pc = 0;
    char buffer[256];
    struct dirent **filelist;
    int i;
    int port = -1;
    char pcilink[1025];
    ssize_t link_sz;

    memset(buffer, 0, 256);
    snprintf(buffer, 255, "/sys/class/net/%s/device/net", name);
    if ((pc = scandir(buffer, &filelist, file_select, alphasort)) > 0) {
        for (i = 0; i < pc; i++) {
            if (!strcmp(name, filelist[i]->d_name)) 
                port = i + 1;
            free(filelist[i]);
        } 
        free(filelist);
    }
    if (port == 1) {
        snprintf(buffer, 255, "/sys/class/net/%s/device", name);
        if ((link_sz = readlink(buffer, pcilink, 1024)) > 0) {
            pcilink[link_sz] = '\0';
            port = atoi(&pcilink[link_sz - 1]) + 1;
        } 
    }
    return port;
}

int getChassisPort_str(char * pci) 
{
    unsigned short dom = 0;
    unsigned char  bus = 0, dev = 0, func = 0;
    if (pci == NULL) return 0;
    sscanf(pci, "%4hx:%2hhx:%2hhx.%1hhx", &dom, &bus, &dev, &func);
    return getChassisPort(bus, dev, func);
}

int getChassisPort(int bus, int dev, int func) 
{
    int i = 0;
    PSMBIOS_ONBOARD_DEVICES_EX_INFORMATION portInfo;

    while (SmbGetRecordByType(SMBIOS_SMBIOS_ONBOARD_DEVICES_EX, 
                              i , (void *)&portInfo) != 0) {
        if ((bus == portInfo->byBusNumber) &&
            (dev == portInfo->byDeviceNumber) &&
            (func == portInfo->byFunctionNumber)) 
           return (((int) portInfo->byDevTypeInstance)&0x07);
        i++;
    }
    return 0;
}

PSMBIOS_SYSTEM_SLOTS  getPCIslot_rec(int  bus)
{
    int i = 0;
    PSMBIOS_SYSTEM_SLOTS slotInfo;

    while (SmbGetRecordByType(SMBIOS_SYS_SLOTS, i , (void *)&slotInfo) != 0) {
        if (bus == slotInfo->byBusNumber) 
            return (slotInfo);
        i++;
    }
    return NULL;
}

int getPCIslot_bus(int  bus)
{
    int i = 0;
    PSMBIOS_SYSTEM_SLOTS slotInfo;

    while (SmbGetRecordByType(SMBIOS_SYS_SLOTS, i , (void *)&slotInfo) != 0) {
        if (bus == slotInfo->byBusNumber) 
            return ((int)slotInfo->wSlotID);
        i++;
    }
    return 0;
}

int getPCIslot_str(char * pci)
{
    unsigned short dom = 0;
    unsigned char  bus = 0, dev = 0, func = 0;
    int slot;
    char pcilink[1025];
    char syspci[256];
    int i;
    ssize_t link_sz;
    ssize_t remaining = 255;

    if (pci == NULL) 
        return 0;

    strncpy(syspci, "/sys/bus/pci/devices/", remaining);
    remaining -=  strlen(syspci);

    strncat(syspci, pci, remaining);
    if ((strchr(pci, '.') == NULL))
        strcat(syspci, ".0");
    link_sz = readlink(syspci, pcilink, 1024);
    if (link_sz > 0) 
        pcilink[link_sz] = '\0';
    else
        return 0;
    for (i = 0; i < link_sz; i++) {
        if (pcilink[i] == '/') {
            i++;
            if (sscanf(&pcilink[i], "%4hx:%2hhx:%2hhx.%1hhx", &dom, &bus, &dev, &func) == 4) 
                if ((slot = getPCIslot_bus(bus)) > 0) 
                    return slot;
        }
    }
    return 0;
}

/* buffer point to /sys/class/scsi_host/host? */
int pcislot_scsi_host(char * buffer)
{
    char lbuffer[256], *pbuffer;
    char *value = "";
    int len;

    if ((len = readlink(buffer, lbuffer, 253)) <= 0) {
        strcat(buffer, "/device");
        len = readlink(buffer, lbuffer, 253);
    }
    if (len > 0) {
        lbuffer[len]='\0'; /* Null terminate the string */
        pbuffer = strrchr(lbuffer, '/');
        while (pbuffer !=  (char *) 0) {
            *(pbuffer++) = '\0';
            if ((*pbuffer >= 0x30) && (*pbuffer <= 0x39))
                break;
            pbuffer = strrchr(lbuffer, '/');
        }
        if (pbuffer != NULL) {
            value = pbuffer;
            if ((pbuffer = strrchr(pbuffer,'.')) != (char *) 0)
                *(pbuffer) = '\0';
        }
    }
    return getPCIslot_str(value);
}

int get_pcislot(char *bus_info)
{
    struct dirent **filelist;
    int slotcount;
    char fname[256] = "/sys/bus/pci/slots/";
    int sysdirlen;
    char buf[80];
    int bc;
    int slotfd = -1;
    int i;
    int ret = 0;


    /* the bus_info var is in "domain:bus:slot.func".
     * sometimes the domain may be absent, in which case
     * we have "bus:slot.func". */
    if ((slotcount = scandir(fname, &filelist, file_select, alphasort)) > 0) {
        sysdirlen = strlen(fname);
        for (i = 0; i < slotcount; i++) {
            strcpy(&fname[sysdirlen], filelist[i]->d_name);
            strcat(fname, "/address");
            if ((slotfd = open(fname, O_RDONLY)) != -1 ) {
                bc = read(slotfd, buf, 80);
                if (bc  > 0) {
                    buf[bc] = 0;
                    if (!strncmp(buf, bus_info, bc - 1))
                        ret = i + 1;
                }
                close(slotfd);
            }
            free (filelist[i]);
        }
        free(filelist);
    }
    return ret;
}

int file_select(const struct dirent *entry)
{
   if ((strcmp(entry->d_name, ".") == 0) || (strcmp(entry->d_name, "..") == 0))
     return (0);
   else
     return (1);
}

int pci_select(const struct dirent *entry)
{
    unsigned short dom = 0;
    unsigned char  bus = 0, dev = 0, func = 0;
    if (sscanf(entry->d_name, "%4hx:%2hhx:%2hhx.%1hhx", &dom, &bus, &dev, &func) == 4)
        return (1);
    return(0);
}

int proc_select(const struct dirent *entry)
{
   if (entry->d_name[0] < '1')
        return(0);
   if (entry->d_name[0] > '9')
        return(0);
   return (1);
}

/*****************************************************************************
 *   Description: Read contents of file into dynamically allocated buffer
 *    Parameters: File name to read from
 *       Returns: Pointer to Buffer or NULL (1st word, 16bits contain count)
 ****************************************************************************/
read_buf_t *ReadFile(char *FileName)
{
    int readsize = 2048;
    int bufsize;

    int readcount, readnext;

    int fileFD;
    read_buf_t *readbuf = NULL;

    if (FileName  != NULL) {
        if ((fileFD = open(FileName, O_RDONLY))  == -1) {
            return (NULL);
        }
    } else
        return (NULL);

    bufsize = readsize;
    readbuf = malloc(bufsize + sizeof(read_buf_t));
    memset(readbuf, 0, bufsize);
    readnext = 0;
    while ((readcount = read(fileFD, &readbuf->buf[readnext], readsize)) > 0) {
        readnext += readcount;
        if ((readnext + readsize) > bufsize) {
            bufsize += readsize;
            readbuf = realloc(readbuf, bufsize + sizeof(read_buf_t));
        }
    }
    
    close(fileFD);
    readbuf->count = readnext;
    return (readbuf);
}

read_line_t *ReadFileLines(char *FileName)
{
    unsigned int  lc = 0;
    unsigned int  lines_len = sizeof(read_line_t);
    char *addrp = NULL;
    read_buf_t *buf;
    read_line_t *lines;

    if ((buf = ReadFile(FileName)) == (read_buf_t *) 0 ) {
        return NULL;
    }
    /* Change linefeeds to EOS */
    addrp = buf->buf;
    while ((addrp = strchr(addrp, '\n'))) {
        *(addrp++) = '\0';
        lc++;
    }
    /* allocate buffer our lines, for count+ pointer+ pointer[lc] */
    lines_len += (lc * sizeof(char *));
    lines = malloc(lines_len);
    memset(lines, 0, lines_len);
    lines->count = lc;
    lines->read_buf = buf;
    lines->line[0] = buf->buf;
    addrp = buf->buf;
    for  (lc = 1; lc < lines->count; lc++) {
        addrp = index(addrp, '\0');
        lines->line[lc] = ++addrp;
    }
    return (lines);
}

#ifdef UNUSED
read_line_t *ReadFilteredFileLines(char *FileName,char *prefix[])
{
    char *str = NULL;
    int  lc,idx;

    read_buf_t *buf;
    read_line_t *lines;
    int lines_len = sizeof(read_line_t);
    char *splitbuf[128];

    if ((buf = ReadFile(FileName)) == (read_buf_t *) 0 ) {
        return 0;
    }

    for (lc=0, str = (char *)buf->buf;;lc++, str = NULL) {
        splitbuf[lc] = strtok(str,"\n");
        if (splitbuf[lc] == NULL)
            break;
    }

    lines_len += (lc * sizeof(char *));
    lines = malloc(lines_len);
    memset(lines, 0, lines_len);
    lines->read_buf = buf;
    lines->count = 0;
    for (lc = 0; splitbuf[lc] != (char *) 0; lc++) {
        idx = 0;
        while (prefix[idx] != (char *) 0) {
            if (strncmp(prefix[idx],splitbuf[lc],strlen(prefix[idx])) == 0) {
                lines->buf[lines->count] = malloc(strlen(splitbuf[lc]) + 1);
                strncpy(lines->buf[lc], splitbuf[lc], strlen(splitbuf[lc]) + 1);
                lines->count++;
            }
            idx++;   
        }
    }
    return (lines);
}

file_tok_t *ReadFilteredFileTokens(char *FileName, char *prefix[],char *s)
{
    line_tok_t *entry;
    file_tok_t *entries;
    read_line_t *lines;
    int idx, tc;
    char *str;
    char *tokens[1000];
    int entry_len = sizeof(line_tok_t);
    int entries_len = sizeof(file_tok_t);
    int lines_len = sizeof(read_line_t);

    lines = ReadFilteredFileLines(FileName, prefix);
    if (lines == (read_line_t *) 0) 
        return 0;

    entries_len += (lines->count * sizeof(char *));
    entries = malloc(entries_len);
    entries->count = lines->count;
    for (idx = 0;  idx < lines->count; idx ++) {
        for (tc = 0, str = lines->buf[idx];;tc++, str = NULL) {
            tokens[tc] = strtok(str,s);
            if (tokens[tc] == NULL)
                break;
        }
        entry_len += (tc * sizeof(line_tok_t));
        entry = malloc(entry_len);
        entry->count = tc;
        for (tc = 0; tc < entry->count; tc++) {
            entry->buf[tc] = malloc(strlen(tokens[tc]) + 1);
            strncpy(entry->buf[tc], tokens[tc], strlen(tokens[tc]) + 1);
        }
        entries->tok[idx] = entry;
    }
    return (entries);
}

#endif
char * get_sysfs_str(char * sysfs_attr)
{
    int fd;
    char *string;
    ssize_t count;
    char *indx;

    if ((fd = open(sysfs_attr,O_RDONLY)) < 0) 
        return NULL;
    if ((string = malloc(4096)) == NULL) {
        close(fd);
        return NULL;
    }
    if ((count = read(fd, string, 4096)) <= 0) {
        close(fd);
        free(string);
        return NULL;
    } else 
        string = realloc(string, count + 1);

    if ((string == NULL) || (*string == 0)){
        close(fd);
	    return NULL;
    }
    indx = index(string,'\n');
    if (indx < (string + count))
        *indx = 0;

    close(fd);
    return string;
}

int _get_sysfs_numeric(char * sysfs_attr, char *fmt, void *value)
{
    char *string;

    if ((string = get_sysfs_str(sysfs_attr)) == NULL)
        return -1;
    errno = 0;
    sscanf(string, fmt, value);
    free(string);
    return 0;
}
 
unsigned long long get_sysfs_llhex(char * sysfs_attr)
{
    unsigned long  long number;
    
    if (_get_sysfs_numeric(sysfs_attr, "%llx", &number) == 0)
    return number;
    else 
        return ((unsigned long long) -1);
}

unsigned long get_sysfs_lhex(char * sysfs_attr)
{
    unsigned long  number;
    
    if (_get_sysfs_numeric(sysfs_attr, "%lx", &number) == 0)
    return number;
    else 
        return ((unsigned long) -1);
}

unsigned int get_sysfs_ihex(char * sysfs_attr)
{
    unsigned int  number;
    
    if (_get_sysfs_numeric(sysfs_attr, "%x", &number) == 0)
    return number;
    else 
        return ((unsigned int) -1);
}

unsigned short get_sysfs_shex(char * sysfs_attr)
{
    unsigned short  number;
    
    if (_get_sysfs_numeric(sysfs_attr, "%hx", &number) == 0)
    return number;
    else 
        return ((unsigned short) -1);
}

unsigned char get_sysfs_chex(char * sysfs_attr)
{
    unsigned char  number;
    
    if (_get_sysfs_numeric(sysfs_attr, "%hhx", &number) == 0)
    return number;
    else 
        return ((unsigned char) -1);
}

unsigned int get_sysfs_uint(char * sysfs_attr)
{
    unsigned int number;

    if (_get_sysfs_numeric(sysfs_attr, "%u", &number) == 0)
    return number;
    else 
        return ((unsigned int) -1);
}

unsigned long long get_sysfs_ullong(char * sysfs_attr)
{
    unsigned long long number;

    if (_get_sysfs_numeric(sysfs_attr, "%llu", &number) == 0)
    return number;
    else 
        return ((unsigned long long) -1);
}

int get_sysfs_int(char * sysfs_attr)
{
    int number = -1;

    if (_get_sysfs_numeric(sysfs_attr, "%d", &number) == 0)
    return number;
    else 
        return ((int) -1);
}

long long get_sysfs_llong(char * sysfs_attr)
{
    long long number;

    if (_get_sysfs_numeric(sysfs_attr, "%ld", &number) == 0)
    return number;
    else 
        return ((long long) -1);
}

#ifdef UTEST
        int main(int argc, char **argv)
        {
            int count, idx;
read_line_t *s;

            s = ReadFileLines("/proc/interrupts");
            fprintf(stderr,"read_size = %d\n",s->read_buf->count);
            fprintf(stderr,"count = %d\n",s->count);
            for (count=0; count < s->count; count++) 
                fprintf(stderr,"%s\n",s->line[count]);
                
/*
            if ((distro = getDistroInfo()) != NULL ) {
                fprintf (stderr,"%s\n",distro->LongDistro);
                fprintf (stderr,"%s\n",distro->Vendor);
                fprintf (stderr,"%s\n",distro->Code);
                fprintf (stderr,"%s\n",distro->VersionString);
                fprintf (stderr,"%s\n",distro->Description);
                fprintf (stderr,"%s\n",distro->Role);
                fprintf (stderr,"%d.%d\n",distro->Version,distro->SubVersion);
            } else 
                fprintf(stderr," getDistroInfo() Error\n");
            fprintf(stderr,"hasTelnet() = %d\n",hasTelnet("/proc/net/tcp"));
            fprintf(stderr,"hasTelnet6() = %d\n",hasTelnet("/proc/net/tcp6"));
    
                if (fsentries == (read_line_t *) 0)  
                  fprintf(stderr," number of lines %d \n",0);
                  else
                  fprintf(stderr," number of lines %d \n",fsentries->count);
                  for (count = 0 ;count < fsentries-> count; count++) {
                  fsentry = fsentries->tok[count];
                  for (idx = 0; idx < fsentry->count; idx++) 
                  fprintf(stderr,"%s ",fsentry->buf[idx]);
                  fprintf(stderr,"\n");
                  } 
                  */

        }
#endif

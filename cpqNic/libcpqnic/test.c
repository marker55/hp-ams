#include <dirent.h>
#include <string.h>
#include <stdio.h>
#define SYS_CLASS_NET "/sys/class/net"
char *nic_names[] = {"eth",
                     (char *) 0};


iGetNicProcFileCount(void)
{
    int      iCount;
    struct dirent **filelist;
    int i=0;

    int nic_select ( const struct dirent *d );

    iCount = scandir("/sys/class/net/", &filelist, nic_select, alphasort);
    while (i < iCount) {
      printf("%s\n", filelist[i]->d_name);
        i++;
    }


    return(iCount);
}

int nic_select(const struct dirent *entry)
{
    int      i = 0;
    while (nic_names[i] != (char *) 0) {
        if (strncmp(entry->d_name,nic_names[i],strlen(nic_names[i])) == 0)
            return(1);
        i++;
    }
     return 0;

}
main(int argv, char** argc) 
{
printf("%d\n",iGetNicProcFileCount());
}


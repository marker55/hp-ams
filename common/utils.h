#include <dirent.h>

#define MAC_ADDRESS_BYTES      6

int file_select(const struct dirent *);
int proc_select(const struct dirent *);

typedef struct _line_tok_t {
    unsigned int count;
    char *buf[];
} line_tok_t;

typedef struct _file_tok_t {
    unsigned int count;
    line_tok_t *tok[];
} file_tok_t;

typedef struct _read_buf_t {
    unsigned int count;
    char buf[];
} read_buf_t;

typedef struct _read_line_t {
    unsigned int count;
    read_buf_t *read_buf;
    char * line[];
} read_line_t;


read_buf_t *ReadFile(char *Filename);
read_line_t *ReadFileLines(char *FileName);

int getChassisPort_str(char *);
int getChassisPort(int, int, int);
int getPCIslot_str(char *);
int getPCIslot_bus(int);

char * get_sysfs_str(char *);
unsigned long long get_sysfs_llhex(char *);
unsigned long long get_sysfs_ullong(char *);
unsigned long  get_sysfs_lhex(char *);
unsigned int   get_sysfs_ihex(char *);
unsigned short get_sysfs_shex(char *);
unsigned char  get_sysfs_chex(char *);
int get_sysfs_hex(char *);
int get_sysfs_hex(char *);
int get_sysfs_int(char *);
long long get_sysfs_llong(char *);


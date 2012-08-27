#ifndef REC_SERVICE_H
#define REC_SERVICE_H
typedef struct _svc_entry {
    int pid;
    int ppid;
    int tgid;
    char *name;
    char *filename;
} svc_entry;

#endif


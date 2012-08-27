#ifndef RECORDER_SERVICE_H
#define RECORDER_SERVICE_H
typedef struct _svc_entry {
    int pid;
    int ppid;
    int tgid;
    char *name;
    char *filename;
} svc_entry;

#endif


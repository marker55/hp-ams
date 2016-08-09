/**
 *  \file crash.c
 *  \brief Provide KDump crash processing capabilities to AMS/REC
 *  \version $Id$
 *  \created 2011/07/05 16:05:31
 *
 *  This module is a little different than most of the REC code here,
 *  in that it runs at hpHelper startup time, checks for any new crash
 *  records (by default in /var/crash), and then it's done.  Crashes
 *  that have been processed are marked by touching a file in its (the
 *  crash's) directory named SUCCESSFULLY_PROCESSED_TAGFILE.
 *
 *  The code starts at LOG_PROCESS_CRASHES at the bottom.
 *
 *  Copyright (C) 2011, Hewlett-Packard Development Company, L.P.
 *
 *  Request: crash bt-r
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <assert.h>
#include <signal.h>
#include <stdarg.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/utsname.h>
#include "ams_rec.h"
#include "recorder.h"
#include "data.h"
#include "../cpqHost/libcpqhost/cpqhost.h"

/* routines needed for debug output */
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/output_api.h>

#ifndef PATH_MAX
#define PATH_MAX 256
#endif /* PATH_MAX */

#define SUCCESSFULLY_PROCESSED_TAGFILE ".hp-ams"
#define DEBUG_TRIGGER_FILE "/tmp/.hp-ams"
#define DEBUG_LOG_FILE "/tmp/hp-ams.log"

#define LINVER_UNK         0x0
#define LINVER_RHEL_UNK    0x1
#define LINVER_RHEL_5      0x2
#define LINVER_RHEL_6      0x3
#define LINVER_SLES_UNK    0x4
#define LINVER_SLES_10     0x5
#define LINVER_SLES_11     0x6f

typedef struct os_dep_config_t {
    int linux_version;
    char* crash_bin;
    char* makedumpfile_bin;
    char* gzip_bin;
    char* tar_bin;
    char* kdump_conf;
    char* crash_dir;
    char* debug_sym_dir_pattern;
    char* vmlinux_pattern;
    char* crash_core;
    char* output_template;
} os_dep_config;

static const char *rh_string = "Red Hat";
static const char *suse_string = "SuSE";
os_dep_config dep;

/* global values */
extern int s_ams_rec_class;
extern int s_ams_rec_handle;
FILE* debug_output = NULL;
int debug = 0;

/* some external prototypes we need */
int rec_init();
int doCodeDescriptor(unsigned int, unsigned int, unsigned char, unsigned char,
                     unsigned char, unsigned char, unsigned char, char *);

static field fld[] = {
    {REC_BINARY_DATA, REC_TYP_TEXT, "KDump Crash Contents"},
};

/*
 * Define simple linked list structure to obviate any external
 * dependencies as much as possible.
 */
typedef struct llist_t {
    char entry[PATH_MAX];
    struct llist_t *next;
} llist;

/**
 *  Rudimentary "strip" function that removes all new line characters
 *  from a string.
 *
 *  \param buf String to process
 *  \param sz Size of string
 *
 */
static void strip(char *buf, int sz) {
    int i = 0;

    if (NULL != buf) {
        for (i = 0; i < sz; i++) {
            if (buf[i] == '\n') {
                buf[i] = 0;
            }
        }
    }
}

static void debug_log(const char *fmt, ...) {
    va_list argp;
    pid_t pid = getpid();


    /*
    time_t t = time(NULL);
    char t_str[ = ctime(&t);
    */

    if (debug) {
        if (NULL == debug_output) {
            debug_output = fopen(DEBUG_LOG_FILE, "a");
            assert(NULL != debug_output);
        }
        //fprintf(debug_output, "[%s]: ", ctime(&t));
        fprintf(debug_output, "hpHelper[%d] ", pid);
        va_start(argp, fmt);
        vfprintf(debug_output, fmt, argp); 
        fprintf(debug_output, "\n");
        va_end(argp);
        fflush(debug_output);
    }
}

static void close_debug_log(void) {
    if (NULL != debug_output) {
        fclose(debug_output);
    }
}

static void get_os_deps(os_dep_config* dep) {
    int linux_version = LINVER_UNK;
    distro_id_t* distro = NULL;

    assert(NULL != dep);

    distro = getDistroInfo();
    if (NULL != distro) {
        if (memcmp(rh_string, distro->Vendor, strlen(rh_string)) == 0) {
            if (6 == distro->Version) {
                linux_version = LINVER_RHEL_6;
            } else if (5 == distro->Version) {
                linux_version = LINVER_RHEL_5;
            } else {
                linux_version = LINVER_RHEL_UNK;
            }
        } if (memcmp(suse_string, distro->Vendor, strlen(suse_string)) == 0) {
            if (11 == distro->Version) {
                linux_version = LINVER_SLES_11;
            } else if (10 == distro->Version) {
                linux_version = LINVER_SLES_10;
            } else {
                linux_version = LINVER_SLES_UNK;
            }
        }
    }
    dep->linux_version = linux_version;

    // TODO: Need to actually fix up the dependencies.
    switch(dep->linux_version) {
    case LINVER_RHEL_UNK:
    case LINVER_RHEL_5:
    case LINVER_RHEL_6:
    case LINVER_SLES_UNK:
    case LINVER_SLES_10:
    case LINVER_SLES_11:
    case LINVER_UNK:        
    default:
        dep->crash_dir = "/var/crash";
        dep->makedumpfile_bin = "/usr/sbin/makedumpfile";
        dep->crash_bin = "/usr/bin/crash";
        dep->gzip_bin = "/bin/gzip";
        dep->tar_bin = "/bin/tar";
        dep->kdump_conf = "/etc/kdump.conf";
        dep->debug_sym_dir_pattern = "/usr/lib/debug/lib/modules/%s";
        dep->vmlinux_pattern = "/usr/lib/debug/lib/modules/%s/vmlinux";
        dep->crash_core = "/vmcore";
        dep->output_template = "/tmp/bbkdwXXXXXX";
    }
}

static int get_release_name(char* buf, int bufsz) {
    int rc = 0;
    if (NULL != buf) {
        struct utsname ubuf;
        memset(&ubuf, 0, sizeof(struct utsname));
        rc = uname(&ubuf);
        if (0 == rc) {
            memset(buf, 0, bufsz);
            strncpy(buf, ubuf.release, bufsz-1);
        } else {
            rc = -1;
        }
    } else {
        rc = -1;
    }

    return rc;
}

/**
 *  Simple check to see if a file or directory exists or not using
 *  the stat() system call.  If filename is NULL, no attempt is
 *  made, and a negative result is returned immediately.
 *
 *  \param filename file or directory name to check
 *  \return 1 if file/directory exists, 0 if it does not
 *
 */
static int does_exist(char *filename) {
    int rc = 0;
    struct stat buf;

    if (NULL != filename) {
        memset(&buf, 0, sizeof(struct stat));
        rc = stat(filename, &buf);
        if (0 == rc) {
            rc = 1;
        } else {
            rc = 0;
        }
    }

    return rc;
}

/**
 *  Generate a temporary name for the output functions
 *
 *  \param buf Character buffer to store name into
 *  \param bufsz Size of character buffer
 *  \returns 0 if successfully, -1 on error
 *
 */
static int gettmpname(char *buf, int bufsz) {
    int fd = 0;
    int rc = 0;

    memset(buf, 0, bufsz);
    strncpy(buf, dep.output_template, bufsz-1);
    
    fd = mkstemp(buf);
    
    if (fd == -1) {
        rc = fd;
    } else {
        close(fd);
        unlink(buf);
    }
    return rc;
}

/**
 *  Tail-recurse through a list freeing the elements individually.
 *
 *  \param l List to free
 *
 */
static void free_list(llist *l) {
    if (NULL == l) {
        return;
    } else {
        free_list(l->next);
        free(l);
        return;
    }
}

/**
 *  Insert an item into a list, returning the new list as a result
 *
 *  \param buf Item to insert into list l
 *  \param l List to insert Item buf into
 *  \returns New list with the new item inserted at the HEAD of the list
 *
 */
static llist* insert_item(char *buf, llist *l) {
    llist* new_item = malloc(sizeof(llist));
    assert(NULL != new_item);
    strncpy(new_item->entry, buf, 
            sizeof(new_item->entry) - strlen(new_item->entry) - 1);
    new_item->next = l;
    return new_item;
}

/**
 *  Determine if a given element exists within a list
 *
 *  \param buf Element to search for
 *  \param l List to search for element
 *  \returns 1 on match, 0 otherwise
 *
 */
static int does_list_contain(char *buf, llist *l) {
    llist *t = l;
    if (NULL != t) {
        while (t != NULL) {
            if (strcmp(buf, t->entry) == 0) {
                return 1;
            }
            t = t->next;
        }
    }
    return 0;
}

/**
 *  Combined the contents to two lists, eliminating duplicates
 *  as necessary.
 *
 *  NOTE: This uses the does_list_contain helper function which will
 *  recursivly check the current list for matching contents several
 *  times over.  Efficient this is not.  However, for these purposes
 *  this is AOK.  This daemon only runs on boot up, and the number of
 *  crashes seen will typically be very small.
 *
 *  \param a First list to combine
 *  \param b Second list to combine
 *  \returns Combined list of elements from lists a and b
 *
 */
static llist* combine_lists_no_dup (llist* a, llist* b) {
    llist* combined = NULL;
    llist* tmp      = NULL;

    if (NULL == a && NULL == b) {
        return NULL;
    }

    tmp = a;
    while (NULL != tmp) {
        combined = insert_item(tmp->entry, combined);
        tmp = tmp->next;
    }

    tmp = b;
    while (NULL != tmp) {
        if (!does_list_contain(tmp->entry, combined)) {
            combined = insert_item(tmp->entry, combined);
        }
        tmp = tmp->next;
    }
    return combined;
}

static int is_binary_available(char *filename) {
    int ret = 0, rc = 0;
    struct stat buf;
    
    if (NULL != filename) {
        memset(&buf, 0, sizeof(struct stat));
        rc = stat(filename, &buf);
        if (0 == rc) {
            if ((S_ISREG(buf.st_mode)) &&
                (S_IXUSR & buf.st_mode) &&
                (S_IRUSR & buf.st_mode)) {
                ret = 1;
            }
        }
    }

    return ret;
}
    
static int is_crash_binary_available(void) {
    return is_binary_available(dep.crash_bin);
}

static int is_makedumpfile_binary_available(void) {
    return is_binary_available(dep.makedumpfile_bin);
}

static int is_gzip_binary_available(void) {
    return is_binary_available(dep.gzip_bin);
}

/* forgive the bad english */
static int is_debug_symbols_available(void) {
    char buf[BUFSIZ];
    char release[PATH_MAX];
    int ret = 0;
    int rc = 0;
    
    memset(buf, 0, BUFSIZ);
    memset(release, 0, PATH_MAX);
    rc = get_release_name(release, PATH_MAX);

    if (0 == rc) {
        struct stat sbuf;
        memset(&sbuf, 0, sizeof(struct stat));
        snprintf(buf, BUFSIZ-1, dep.debug_sym_dir_pattern, release);
        rc = stat(buf, &sbuf);

        if (0 == rc) {
            if (S_ISDIR(sbuf.st_mode)) {
                ret = 1;
            }
        }
    }
    return ret;
}

static int run_proc_return_result(char *cmd, int* result)
{
    int rc  = 0;
    char buf[BUFSIZ];
    if (NULL != cmd) {
        FILE *pp = popen(cmd, "r");
        if (NULL != pp) {
            while(fgets(buf, BUFSIZ-1, pp) != NULL) {
                continue;
            }
            rc = pclose(pp);
        } else {
            rc = -1;
        }
    } else {
        rc = -1;
    }

    if (NULL != result) {
        *result = rc;
    }
    return rc;
}

static int log_buffer_contents_rec(char *msg) {
    int i = 0;
    int rc = 0;
    int num_descript = 0;
    size_t toc_sz = 0;
    size_t blob_sz = 0;
    char* blob;
    RECORDER_API4_RECORD *toc;

    if (NULL != msg) {

        if (!rec_init()) {
            return -1;
        }

        if ((rc = doCodeDescriptor(s_ams_rec_handle,
                                   s_ams_rec_class,
                                   REC_CODE_AMS_OS_CRASH,
                                   0,
                                   REC_SEV_CRIT,
                                   REC_BINARY_DATA,
                                   REC_VIS_CUSTOMER,
                                   REC_CODE_AMS_OS_CRASH_STR)) != RECORDER_OK) {
            DEBUGMSGTL(("rec:rec_kdump","KDump register descriptor failed %d\n", rc));
            return -1;
        }

        /* TODO: Since I'm collapsing all of the kdump data into a
         * single field, do I have to do this? 
         */
        num_descript = sizeof(fld)/sizeof(field);
        DEBUGMSGTL(("rec:log","Kdump num_descript = %d\n", num_descript));
        if ((toc_sz = sizeof(RECORDER_API4_RECORD) * num_descript) 
            > RECORDER_MAX_BLOB_SIZE) {
            DEBUGMSGTL(("rec:log", "KDump descriptor too large %ld\n", toc_sz));
            return -1;
        }

        DEBUGMSGTL(("rec:log", "Kdump toc_sz = %ld\n", toc_sz));
        if ((toc = malloc(toc_sz)) == NULL) {
            DEBUGMSGTL(("rec:log","KDump unable to malloc %ld bytes\n", toc_sz));
            return -1;
        }
        memset(toc, 0, toc_sz);

        /* now setup field descriptor */
        for (i = 0; i < num_descript; i++) {
            toc[i].flags = REC_FLAGS_API4 | REC_FLAGS_DESC_FIELD;
            toc[i].classs = s_ams_rec_class;
            toc[i].code = REC_CODE_AMS_OS_CRASH;
            toc[i].field = i;
            toc[i].severity_type = REC_DESC_SEVERITY_CRITICAL | fld[i].tp;
            toc[i].format = fld[i].sz;
            toc[i].visibility = REC_DESC_VISIBILITY_CUSTOMER;
            strncpy(toc[i].desc, fld[i].nm, strlen(fld[i].nm));
            DEBUGMSGTL(("rec:log", "toc[i] = %p\n", &toc[i]));
            dump_chunk("rec:rec_log", "descriptor", (const u_char*)&toc[i], 96);
        }
        dump_chunk("rec:rec_log", "toc", (const u_char*)toc, toc_sz);
        
        if ((rc = rec_api4_field(s_ams_rec_handle, toc_sz, toc))
            != RECORDER_OK) {
            DEBUGMSGTL(("rec:log", "KDump register descriptor failed %d\n", rc));
            free(toc);
            return -1;
        }
        free(toc);

        blob_sz = RECORDER_MAX_BLOB_SIZE;
        if ((blob = malloc(blob_sz)) == NULL) {
            DEBUGMSGTL(("rec:log", "KDump unable to malloc() %ld blob\n", blob_sz));
            return -1;
        }

        memset(blob, 0, blob_sz);
        strncpy(blob, msg, blob_sz);
        blob_sz = strlen(blob);
        if ((rc = rec_api6(s_ams_rec_handle, (const char*)blob, blob_sz))
            != RECORDER_OK) {
            DEBUGMSGTL(("rec:log", "KDump log to recorder data failed (%d)\n", rc));
            free(blob);
            return -1;
        }
        DEBUGMSGTL(("rec:log", "Logged record for code %d\n", REC_CODE_AMS_OS_CRASH));
        free(blob);
        return 0;
    }

    /* msg was NULL */
    return -1;
}

static int log_buffer_contents(char *msg) {
    int rc  = -1;
    int len = 0;
    char buf[RECORDER_MAX_BLOB_SIZE];
    char* cur = msg;

    memset(buf, 0, RECORDER_MAX_BLOB_SIZE);
    
    if (NULL != msg) {
        len = strlen(msg);

        /* copy the msg contents into a fixed-size buffer, and log */
        strncpy(buf, msg, sizeof(buf) - 1);
        rc = log_buffer_contents_rec(buf);
        
        /* if msg was larger than the maximum blob size, forward the
         * cur pointer by the maximum blob size, and call this
         * function again to log the remainder of the message. 
         */
        if (len > RECORDER_MAX_BLOB_SIZE) {
            cur = cur+RECORDER_MAX_BLOB_SIZE;
            rc = log_buffer_contents(cur);
        }
    }
    return rc;
}

static int log_file_contents(char *filename) {
    int rc = 0;
    char* buf = NULL;
    FILE* fp = NULL;
    int file_len = 0;

    fp = fopen(filename, "rb");
    if (fp) {
        fseek(fp, 0, SEEK_END);
        file_len = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        if (file_len >0) {
            buf = (char*)malloc(file_len + 1);
    
            if (buf) {
                size_t sz;
    
                memset(buf, 0, file_len + 1);

                sz = fread(buf, file_len, 1, fp);
                rc = log_buffer_contents(buf);
                free(buf);
            } else {
                rc = -1;
            }
        } else
            rc = -1;
        fclose(fp);
    } else {
        rc = -1;
    }
    return rc;
}

/**
 *  Extract a path value from a given kdump configuration file.
 *
 *  \param filename Filename to search (expected to be a kdump.conf file)
 *  \param buf Buffer to store the return value in
 *  \param bufsz Size of buffer used to return value
 *  \returns 0 if path value successfully identified, 1 otherwise.
 *
 */
static int get_path_value_from_file(char* filename, char* buf, int bufsz) {
    int rc = 1;
    FILE* fp = NULL;
    if (NULL != buf) {
        memset(buf, 0, bufsz);
        fp = fopen(filename, "r");
        if (NULL != fp) {
            int lcount = 1;
            char* line = malloc(BUFSIZ);  /* using a ptr so we can do
                                           * some ptr magic to get the
                                           * rest of a string after
                                           * matching part of it
                                           */
            memset(line, 0, BUFSIZ);

            while(fgets(line, BUFSIZ-1, fp) != NULL) {
                char match[] = "path";
                int match_sz = sizeof(match);

                strip(line, BUFSIZ);
                if (memcmp(line, match, match_sz-1) == 0) {
                    char *nptr = line+match_sz;

                    debug_log("Found path configuration %s in %s on line %d.",
                              nptr, filename, lcount);
                    strncpy(buf, nptr, bufsz-1);
                    rc = 0;
                }
                lcount++;
                memset(line, 0, BUFSIZ);
            }
            free(line);
            fclose(fp);
        }
    }

    return rc;
}

/**
 *  Determine the name of the crash directory.  If the user has
 *  specified value in /etc/kdump.conf, use it.  Otherwise, use
 *  the default value as defined in DEFAULT_CRASH_DIR.
 *
 *  \param buf Character buffer to store the crash directory string in
 *  \param bufsz Size of character buffer
 *  \returns 0 on success, -1 on error (buffer was invalid)
 *
 */
static int get_crash_dir(char *buf, int bufsz) {
    char buffer[BUFSIZ];
    memset(buffer, 0, BUFSIZ);

    if (NULL != buf) {
        memset(buf, 0, bufsz);
        if (does_exist(dep.kdump_conf)) {
            /* kdump.conf exists, look for path value */
            int rc = get_path_value_from_file(dep.kdump_conf,
                                              buffer, BUFSIZ);

            if (rc) {
                /* failed to obtain a path from the kdump config, use
                 * default
                 */
                debug_log("Did not find path value in %s, using default value: %s", dep.kdump_conf, dep.crash_dir);
                strncpy(buf, dep.crash_dir, bufsz-1);
            } else {
                /* obtained a path from the kdump config, copy it into
                 * the result buffer
                 */
                debug_log("Found path value in %s, using: %s", dep.kdump_conf, buffer);
                strncpy(buf, buffer, bufsz-1);
            }

        } else {
            /* no kdump.conf, use default */
            debug_log("No %s file found, using default: %s", dep.kdump_conf, dep.crash_dir);
            strncpy(buf, dep.crash_dir, bufsz-1);
        }
    } else {
        /* buf is NULL, so do nothing, just return error value */
        return -1;
    }

    return 0;
}

/**
 *  Determine if the given path and directory are a crash directory,
 *  determined by its existance as a filesystem directory, and then
 *  verifying that it contains a vmcore file.
 *
 *  \param path Path to the top level crash directory
 *  \param dir Directory within the crash directory
 *  \returns 1 If target is a valid crash directory, 0 otherwise.
 *
 */
static int is_crash_dir(char *path, char *dir) {
    char buf[BUFSIZ];
    struct stat sb;
    int x = 0;
    int rc = 0;
    memset(buf, 0, BUFSIZ);
    memset(&sb, 0, sizeof(struct stat));

    strncpy(buf, path, BUFSIZ-1);
    x = strlen(buf) + 1;
    strncat(buf, "/", BUFSIZ-x);
    strncat(buf, dir, BUFSIZ-(x+1)); 
    x = strlen(buf) + 1;

    if (0 == stat(buf, &sb)) {
        if (S_ISDIR(sb.st_mode)) {
            strncat(buf, dep.crash_core, BUFSIZ-x);
            memset(&sb, 0, sizeof(struct stat));
            if (0 == stat(buf, &sb)) {
                rc = 1;
            }
        }
    }
    return rc;
}

/**
 *  Create a linked list of crash entries as found in CRASH_DIR.  If
 *  the previous_flag parameter is set, the list will include crash
 *  entries that include the SUCCESSFULLY_PROCESSED_TAGFILE file.  If
 *  the parameter is set to zero, then the list will include crash
 *  entries that do NOT include the tag file.
 *
 *  \param previous_flag If set, require the 
 *  \returns Linked list of entries found, or NULL if none.
 */
static llist* find_crash_entries(int previous_flag) {
    llist*          l = NULL;
    DIR           *dp = NULL;
    struct dirent *ep = NULL;
    struct stat    sb;
    char crash_dir[PATH_MAX];

    memset(&sb, 0, sizeof(struct stat));

    get_crash_dir(crash_dir, PATH_MAX);

    if (does_exist(crash_dir)) {
        debug_log("Crash directory exists (%s), examining for candidates to process.", crash_dir);
        dp = opendir(crash_dir);
        if (NULL != dp) {
            while ((ep = readdir(dp))) {
                if ((strcmp(".", ep->d_name)  == 0) ||
                    (strcmp("..", ep->d_name) == 0)) {
                    continue;
                }

                /* verify that the subdirectory contains a vmcore file */
                if (is_crash_dir(crash_dir, ep->d_name)) {
                    char touch_file[PATH_MAX];
                    int stat_result = 0;
                    memset(touch_file, 0, PATH_MAX);
                    snprintf(touch_file, PATH_MAX-1, "%s/%s/%s",
                             crash_dir, ep->d_name, 
                             SUCCESSFULLY_PROCESSED_TAGFILE);

                    stat_result = stat(touch_file, &sb);
                    if (previous_flag && !stat_result) {
                        // looking for previous, and found one
                        l = insert_item(ep->d_name, l);
                        debug_log("Adding previously processed: %s/%s.", crash_dir, ep->d_name);
                    } else {
                        if (!previous_flag && stat_result) {
                            // looking for new, and did not find touch file
                            l = insert_item(ep->d_name, l);

                            debug_log("Adding candidate: %s/%s.", crash_dir, ep->d_name);
                        }
                    }
                } else {
                    /* skip if there is no vmcore file */
                    debug_log ("Skipping: %s (no vmcore found).", ep->d_name);
                }
            }
            closedir(dp);
        } else {
            perror ("Couldn't open crash directory.");
        }
    } else {
        debug_log ("Crash directory does not exist (%s), skipping.", crash_dir);
    }
    return l;
}

static llist* find_current_crash_entries(void) {
    return find_crash_entries(0);
}

static llist* find_previous_crash_entries(void) {
    return find_crash_entries(1);
}

/**
 *  Merge to lists of entries together, without duplicates, and
 *  write them back out to the KDUMP_FOUND_LIST file.
 *
 *  \param cur List of current crash entries
 *  \param cur List of previously recorded crash entries
 *  \returns 0 on success, 1 on failure
 *
 */
static int update_previous_crash_entries(llist* cur, llist* prev) {
    llist* update = combine_lists_no_dup(cur, prev);
    llist* t = update;
    FILE *fp = NULL;
    struct stat sb;

    memset(&sb, 0, sizeof(struct stat));
    while(NULL != t) {
        char filename[PATH_MAX];
        char crash_dir[PATH_MAX];
        memset(filename, 0, PATH_MAX);
        memset(crash_dir, 0, PATH_MAX);

        get_crash_dir(crash_dir, PATH_MAX);

        snprintf(filename, PATH_MAX-1, "%s/%s/%s",
                 crash_dir, t->entry, SUCCESSFULLY_PROCESSED_TAGFILE);
        if (stat(filename, &sb)) {
            // file doesn't exist
            if ((fp = fopen(filename, "w")) != NULL ) {
                fprintf(fp, "This file notes that this crash has already been processed\n");
                fprintf(fp, "and recorded by hpHelper.\n");
                fclose(fp);
                debug_log ("Created %s", filename);
            }
        }

        t = t->next;
    }

    free_list(update);
    return 0;
}

/**
 *  Write a simple list of crash commands to a file to be used as 
 *  input for the crash kdump vmcore processor.
 *
 *  \param filename File to write command script into
 *  \returns 0 on success, -1 on failure.
 *
 */
static int write_crash_input_script(char *filename) {
    int rc = -1;
    FILE* fp = NULL;
    if (NULL != filename) {
        fp = fopen(filename, "w");
        if (NULL != fp) {
            fprintf(fp, "ps\n");
            fprintf(fp, "panic\n");
            fprintf(fp, "proc 0..7\n");
            fprintf(fp, "sym -l\n");
            fclose(fp);
            rc = 0;
        }
    }
    return rc;
}

static void append_gz(char *buf, int bufsz) {
    char tmp[bufsz];
    memset(tmp, 0, bufsz);
    strncpy(tmp, buf, bufsz);
    memset(buf, 0, bufsz);
    snprintf(buf, bufsz-1, "%s.gz", tmp);
}

/* TODO: Move this to get_os_deps() */
#if 0
static int build_crash_command(char* buf, 
                               int bufsz, 
                               char* entry, 
                               char *input_script,
                               char *output) {
    int rc = 1;
    char vmcore_location[PATH_MAX];
    char vmlinux_location[PATH_MAX];
    char release[PATH_MAX];
    char crash_dir[PATH_MAX];

    if (NULL != buf && NULL != entry && NULL != input_script && NULL != output) {
        memset(buf, 0, bufsz);
        memset(vmcore_location, 0, PATH_MAX);
        memset(vmlinux_location, 0, PATH_MAX);
        memset(release, 0, PATH_MAX);
        memset(crash_dir, 0, PATH_MAX);

        get_release_name(release, PATH_MAX);
        get_crash_dir(crash_dir, PATH_MAX);

        // TODO: Replace this.. dsb 2011.07.09
        switch(dep.linux_version) {
        case LINVER_RHEL_UNK:
        case LINVER_RHEL_6:
        case LINVER_RHEL_5:
        case LINVER_SLES_UNK:
        case LINVER_SLES_10:
        case LINVER_SLES_11:
            snprintf(vmcore_location, PATH_MAX-1, "%s/%s/vmcore", crash_dir, entry);
            snprintf(vmlinux_location, PATH_MAX-1, dep.debug_sym_dir_pattern, release);

            snprintf(buf, bufsz-1, "%s %s %s < %s > %s",
                     dep.crash_bin, vmlinux_location, vmcore_location,
                     input_script, output);

            rc = 0;
            break;
        default:
            rc = -1;
            break;
        }
    } else {
        rc = -1;
    }

    return rc;
}
#endif 

/**
 *  Perform crash information extraction from a single given crash entry.
 *  This function performs the lion's share of the work in processing
 *  each crash.
 *
 *  \param entry Subdirectory (of CRASH_DIR) to process
 *  \returns 0 on success, 1 on failure
 *
 *  TODO: Refactor this function to simplify it, break out the
 *  individual elements into their own function.  dsb 2011.09.07
 */
static int process_crash_item(char *entry) {
    int rc = 0;
    char cmdbuf[BUFSIZ];
    char tmp_name[PATH_MAX];
    char dmesg_output[PATH_MAX];
    char crash_input_script[PATH_MAX];
    char crash_output[PATH_MAX];
    char crash_dir[PATH_MAX];

    memset(cmdbuf, 0, BUFSIZ);
    memset(tmp_name, 0, PATH_MAX);
    memset(dmesg_output, 0, PATH_MAX);
    memset(crash_input_script, 0, PATH_MAX);
    memset(crash_output, 0, PATH_MAX);
    memset(crash_dir, 0, PATH_MAX);
    
    get_crash_dir(crash_dir, PATH_MAX);
    gettmpname(tmp_name, PATH_MAX);
    snprintf(dmesg_output, PATH_MAX-1, "%s-dmesg", tmp_name);
    snprintf(crash_input_script, PATH_MAX-1, "%s-crash-input", tmp_name);
    snprintf(crash_output, PATH_MAX-1, "%s-crash-output", tmp_name);

    /* extract dmesg contents, log to REC */
    if (is_makedumpfile_binary_available()) {
        memset(cmdbuf, 0, BUFSIZ);
        snprintf(cmdbuf, BUFSIZ-1, "%s --dump-dmesg %s/%s/vmcore %s",
                 dep.makedumpfile_bin, crash_dir, entry, dmesg_output);
        debug_log("RUN: %s", cmdbuf);
        rc += run_proc_return_result(cmdbuf, NULL);

        if (is_gzip_binary_available()) {
            memset(cmdbuf, 0, BUFSIZ);
            snprintf(cmdbuf, BUFSIZ-1, "%s --best %s", dep.gzip_bin, dmesg_output);
            debug_log("RUN: gzip --best %s", dmesg_output);
            append_gz(dmesg_output, PATH_MAX);
            rc += run_proc_return_result(cmdbuf, NULL);
        }

        debug_log("LOG: %s", dmesg_output);
        log_file_contents(dmesg_output);

        if (is_crash_binary_available()) {
            debug_log("crash is available");
            if (is_debug_symbols_available()) {
                char vmcore_loc[PATH_MAX];
                char vmlinux_loc[PATH_MAX];
                char release[PATH_MAX];

                debug_log ("Debug symbols are available");
                memset(vmcore_loc, 0, PATH_MAX);
                memset(vmlinux_loc, 0, PATH_MAX);
                memset(release, 0, PATH_MAX);

                get_release_name(release, PATH_MAX);
                
                snprintf(vmcore_loc, PATH_MAX-1, "%s/%s/vmcore", crash_dir, entry);
                snprintf(vmlinux_loc, PATH_MAX-1, dep.vmlinux_pattern, release);

                rc = write_crash_input_script(crash_input_script);
                if (!rc) {

                    memset(cmdbuf, 0, BUFSIZ);
                    snprintf(cmdbuf, BUFSIZ-1, "%s %s %s < %s > %s",
                             dep.crash_bin, vmlinux_loc, vmcore_loc, 
                             crash_input_script, crash_output);
                    
                    debug_log("RUN: %s", cmdbuf);
                    rc += run_proc_return_result(cmdbuf, NULL);

                    if (is_gzip_binary_available()) {
                        memset(cmdbuf, 0, BUFSIZ);
                        snprintf(cmdbuf, BUFSIZ-1, "%s --best %s", dep.gzip_bin, crash_output);
                        debug_log("RUN: gzip --best %s", crash_output);
                        append_gz(crash_output, PATH_MAX);

                        rc += run_proc_return_result(cmdbuf, NULL);
                    }


                    debug_log ("LOG: %s", crash_output);
                    rc = log_file_contents(crash_output);
                } else {
                    debug_log ("Failed to write crash input script.");
                    log_buffer_contents("Failed to write crash input script.");
                    rc = 1;
                }
            } else {
                debug_log ("Debug package matching the running kernel is not installed.");
                log_buffer_contents("Install the matching kernel debug package to enable the extraction of crash information.");
                rc = 1;
            }
        } else {
            debug_log ("The crash binary is not installed.");
            log_buffer_contents("Install the crash package to enable the extraction of kdump information.");
            rc = 1;
        }
    } else {
        debug_log ("The makedumpfile binary is not installed.");
        log_buffer_contents("makedumpfile is not available on this system.  Cannot extract crash information.");
        rc = 1;
    }

    /* clean up */
    if (!debug) {
        unlink(dmesg_output);
        unlink(crash_input_script);
        unlink(crash_output);
    }
    return rc;
}

/**
 *  Determine which crashes are new, and which are not.  For the new
 *  items, call process_crash_item.
 *
 *  \param cur List of currently found crashes
 *  \param prev List of previously found crashes
 *  \returns 0 on success, 1 or more on failure (errors from process_crash_item aggregate
 *
 */
static int process_new_crashes(llist* cur, llist* prev) {
    int rc = 0;
    llist* tmp = cur;

    while (NULL != tmp) {
        if (!does_list_contain(tmp->entry, prev)) {
            /* this is a new crash */
            rc += process_crash_item(tmp->entry);
        } else {
            debug_log ("%s has already been processed before", tmp->entry);
        }
        tmp = tmp->next;
    }
    return rc;

}

/**
 *  Perform the crash check.  This is the primary function of the
 *  kdump_watcher program.  It builds lists of current crash
 *  directories, and previously seen crash directories.  If a new one
 *  is found, we perform a few bits of data extraction and log the
 *  results to Recorder.
 *
 *  \returns 0 on success, non-zero otherwise
 *
 */
void LOG_PROCESS_CRASHES(void) {
    int rc = 0;
    llist* current_crash_entries = NULL;
    llist* previous_crash_entries = NULL;
    struct stat sb;

    /* enable debugging based on a trigger file, rather than command
     * line option */
    memset(&sb, 0, sizeof(struct stat));
    if (does_exist(DEBUG_TRIGGER_FILE)) {
        debug = 1;
        debug_log ("Found %s, enabling debugging", DEBUG_TRIGGER_FILE);
    }
    
    memset(&dep, 0, sizeof(os_dep_config));
    get_os_deps(&dep);
    debug_log ("Loaded OS dependencies.");
    
    /* List entries in crash directory */
    current_crash_entries = find_current_crash_entries();

    if (NULL != current_crash_entries) {

        /* List entries in previous crash list */
        previous_crash_entries = find_previous_crash_entries();

        /* Process any NEW crashes that we have not seen before */
        rc = process_new_crashes(current_crash_entries, previous_crash_entries);

        if (!rc) {
            /* update previous_crash_entries file */
            rc = update_previous_crash_entries(current_crash_entries, previous_crash_entries);
        } else {
            debug_log("Failed to process one or more crashes.  Not updating crash list.");
        }

    } else {
        debug_log("No crashes found.");
    }

    close_debug_log();
    free_list(current_crash_entries);
    free_list(previous_crash_entries);
    return;
}

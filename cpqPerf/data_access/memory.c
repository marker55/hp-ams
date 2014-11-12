#include <stdio.h>
#include <string.h>

#include "parseproc.h"
#include "parsestat.h"
#include "procfs.h"
#include "queue.h"
#include "obj.h"
#include "objqueue.h"

typedef struct data_s {
    int total;
    int free;
    int high_total;
    int high_free;
    int low_total;
    int low_free;
    int swap_total;
    int swap_free;
    int cached;
    int swap_cached;
    int active;
    int inactive_dirty;
    int inactive_clean;
} data_t;

static objnode_info_t infodb[] = {
    {"cpqLinOsMemTotal", sizeof(int), SIGNED_TYPE,
     READ_ONLY, ENABLED, DATA_OFFSET(data_t, total), NULL},
    {"cpqLinOsMemFree", sizeof(int), SIGNED_TYPE,
     READ_ONLY, ENABLED, DATA_OFFSET(data_t, free), NULL},
    {"cpqLinOsMemHighTotal", sizeof(int), SIGNED_TYPE,
     READ_ONLY, ENABLED, DATA_OFFSET(data_t, high_total), NULL},
    {"cpqLinOsMemHighFree", sizeof(int), SIGNED_TYPE,
     READ_ONLY, ENABLED, DATA_OFFSET(data_t, high_free), NULL},
    {"cpqLinOsMemLowTotal", sizeof(int), SIGNED_TYPE,
     READ_ONLY, ENABLED, DATA_OFFSET(data_t, low_total), NULL},
    {"cpqLinOsMemLowFree", sizeof(int), SIGNED_TYPE,
     READ_ONLY, ENABLED, DATA_OFFSET(data_t, low_free), NULL},
    {"cpqLinOsMemSwapTotal", sizeof(int), SIGNED_TYPE,
     READ_ONLY, ENABLED, DATA_OFFSET(data_t, swap_total), NULL},
    {"cpqLinOsMemSwapFree", sizeof(int), SIGNED_TYPE,
     READ_ONLY, ENABLED, DATA_OFFSET(data_t, swap_free), NULL},
    {"cpqLinOsMemCached", sizeof(int), SIGNED_TYPE,
     READ_ONLY, ENABLED, DATA_OFFSET(data_t, cached), NULL},
    {"cpqLinOsMemSwapCached", sizeof(int), SIGNED_TYPE,
     READ_ONLY, ENABLED, DATA_OFFSET(data_t, swap_cached), NULL},
    {"cpqLinOsMemActive", sizeof(int), SIGNED_TYPE,
     READ_ONLY, ENABLED, DATA_OFFSET(data_t, active), NULL},
    {"cpqLinOsMemInactiveDirty", sizeof(int), SIGNED_TYPE,
     READ_ONLY, ENABLED, DATA_OFFSET(data_t, inactive_dirty), NULL},
    {"cpqLinOsMemInactiveClean", sizeof(int), SIGNED_TYPE,
     READ_ONLY, ENABLED, DATA_OFFSET(data_t, inactive_clean), NULL}
};

static size_t which_meminfo(objgroup_t * objgroup, index_t ** index)
{
    *index = index_ctor(1, 0);  // 1 x 0-dimensional index
    return (*index == NULL) ? 0 : 1;
}

static int do_meminfo(objnode_t * objnode)
{
    obj_t *obj = objnode->obj;
    data_t *dat = (data_t *) obj->data;

    objnode->flags |= OBJNODE_DIRTY;

    if (curr_stat->mem.nmemb <= 13) {
        memcpy(dat, curr_stat->mem.memstat, 13 * sizeof(int));
    }

    return 0;

}

//initialization
int init_meminfo(q_node ** objnode_Q)
{
    return objgroup_ctor(objnode_Q, "memory", infodb, OBJNODE_INFO_SIZE(infodb),
                         do_meminfo, which_meminfo);
}

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>

#include <net-snmp/agent/net-snmp-agent-includes.h>
#include "cpqIdeDisk.h"
#include "idedisk.h"


/**---------------------------------------------------------------------*/
/*
 * local static vars
 */
static int _idedisk_init = 0;
       int _idedisk_max  = 0;
static netsnmp_cache     *idedisk_cache     = NULL;
static netsnmp_container *idedisk_container = NULL;

void netsnmp_idedisk_container_free(netsnmp_container *);
netsnmp_container* netsnmp_idedisk_container_load(netsnmp_container*);
void netsnmp_idedisk_container_free_items(netsnmp_container *);

netsnmp_container * netsnmp_idedisk_container(void);
netsnmp_cache     * netsnmp_idedisk_cache    (void);

/*
 * local static prototypes
 */
static void _idedisk_entry_release(netsnmp_idedisk_entry * entry,
                                            void *unused);

/**---------------------------------------------------------------------*/
/*
 * external per-architecture functions prototypes
 *
 * These shouldn't be called by the general public, so they aren't in
 * the header file.
 */
extern int netsnmp_arch_idedisk_container_load(netsnmp_container* container);

/**
 * initialization
 */
void
init_idedisk(void)
{
    DEBUGMSGTL(("idedisk:access", "init\n"));

    netsnmp_assert(0 == _idedisk_init); /* who is calling twice? */

    if (1 == _idedisk_init)
        return;

    _idedisk_init = 1;

    (void)netsnmp_idedisk_container();
    (void) netsnmp_idedisk_cache();
}

void
shutdown_idedisk(void)
{
    DEBUGMSGTL(("idedisk:access", "shutdown\n"));

}

/**---------------------------------------------------------------------*/
/*
 * cache functions
 */

static int
_cache_load( netsnmp_cache *cache,  void *magic )
{
    netsnmp_idedisk_container_load( idedisk_container);
    return 0;
}

static void
_cache_free( netsnmp_cache *cache,  void *magic )
{
    //netsnmp_idedisk_container_free_items( idedisk_container );
    return;
}

/**
 * create idedisk cache
 */
netsnmp_cache *
netsnmp_idedisk_cache(void)
{
    DEBUGMSGTL(("idedisk:cache", "init\n"));
    if ( !idedisk_cache ) {
        idedisk_cache = netsnmp_cache_create(3600,   /* timeout in seconds */
                           _cache_load,  _cache_free,
                           cpqIdeDiskTable_oid, cpqIdeDiskTable_oid_len);
        if (idedisk_cache)
            idedisk_cache->flags =  NETSNMP_CACHE_PRELOAD |
                                    NETSNMP_CACHE_DONT_FREE_EXPIRED |
                                    NETSNMP_CACHE_DONT_AUTO_RELEASE |
                                    NETSNMP_CACHE_AUTO_RELOAD;
    }
    return idedisk_cache;
}


/**---------------------------------------------------------------------*/
/*
 * container functions
 */
/**
 * create idedisk container
 */
netsnmp_container *
netsnmp_idedisk_container(void)
{
    DEBUGMSGTL(("idedisk:container", "init\n"));

    /*
     * create the container.
     */
  if (!idedisk_container) {
    idedisk_container = netsnmp_container_find("idedisk:binary_array");
    if (NULL == idedisk_container)
        return NULL;

    idedisk_container->container_name = strdup("idedisk container");
  }

    return idedisk_container;
}

/**
 * load idedisk information in specified container
 *
 * @param container empty container to be filled.
 *                  pass NULL to have the function create one.
 *
 * @retval NULL  error
 * @retval !NULL pointer to container
 */
netsnmp_container*
netsnmp_idedisk_container_load(netsnmp_container* user_container)
{
    netsnmp_container* container = user_container;
    int rc;

    DEBUGMSGTL(("idedisk:container:load", "load\n"));
    //netsnmp_assert(1 == _idedisk_init);

    if (NULL == container)
        container = netsnmp_idedisk_container();
    if (NULL == container) {
        snmp_log(LOG_ERR, "no container specified/found for idedisk\n");
        return NULL;
    }

    rc =  netsnmp_arch_idedisk_container_load(container);
    if (0 != rc) {
        if (NULL == user_container) {
            netsnmp_idedisk_container_free(container);
            container = NULL;
        }
    }

    return container;
}

void
netsnmp_idedisk_container_free(netsnmp_container *container)
{
    DEBUGMSGTL(("idedisk:container", "free\n"));

    if (NULL == container) {
        snmp_log(LOG_ERR, "invalid container for netsnmp_idedisk_container_free\n");
        return;
    }

    netsnmp_idedisk_container_free_items(container);

    CONTAINER_FREE(container);
}

void
netsnmp_idedisk_container_free_items(netsnmp_container *container)
{
    DEBUGMSGTL(("idedisk:container", "free_items\n"));

    if (NULL == container) {
        snmp_log(LOG_ERR, "invalid container for netsnmp_idedisk_container_free_items\n");
        return;
    }

    /*
     * free all items.
     */
    CONTAINER_CLEAR(container,
                    (netsnmp_container_obj_func*)_idedisk_entry_release,
                    NULL);
}

/**---------------------------------------------------------------------*/
/*
 * idedisk_entry functions
 */
/**
 */
netsnmp_idedisk_entry *
netsnmp_idedisk_entry_get_by_index(netsnmp_container *container, oid index)
{
    netsnmp_index   tmp;

    DEBUGMSGTL(("idedisk:entry", "by_index\n"));
    netsnmp_assert(1 == _idedisk_init);

    if (NULL == container) {
        snmp_log(LOG_ERR,
                 "invalid container for netsnmp_idedisk_entry_get_by_index\n");
        return NULL;
    }

    tmp.len = 1;
    tmp.oids = &index;

    return (netsnmp_idedisk_entry *) CONTAINER_FIND(container, &tmp);
}

/**
 */
netsnmp_idedisk_entry *
netsnmp_idedisk_entry_create(oid index, oid index2)
{
    netsnmp_idedisk_entry *entry =
        SNMP_MALLOC_TYPEDEF(netsnmp_idedisk_entry);

    if(NULL == entry)
        return NULL;

    memset(entry,0,sizeof(netsnmp_idedisk_entry));
    entry->Controller  = index;
    entry->Index  = index2;

    entry->oid_index.len = 2;
    entry->oid_index.oids = (oid *) & entry->Controller;

    return entry;
}

/**
 */
NETSNMP_INLINE void
netsnmp_idedisk_entry_free(netsnmp_idedisk_entry * entry)
{
    if (NULL == entry)
        return;

    /*
     * SNMP_FREE not needed, for any of these, 
     * since the whole entry is about to be freed
     */
    free(entry);
}

/**---------------------------------------------------------------------*/
/*
 * Utility routines
 */

/**
 */
static void
_idedisk_entry_release(netsnmp_idedisk_entry * entry, void *context)
{
    netsnmp_idedisk_entry_free(entry);
}


#ifdef TEST
int main(int argc, char *argv[])
{
    const char *tokens = getenv("SNMP_DEBUG");

    netsnmp_container_init_list();

    /** idedisk,verbose:idedisk,9:idedisk,8:idedisk,5:idedisk */
    if (tokens)
        debug_register_tokens(tokens);
    else
        debug_register_tokens("idedisk");
    snmp_set_do_debugging(1);

    init_idedisk();
    netsnmp_idedisk_container_load(NULL, 0);
    shutdown_idedisk();

    return 0;
}
#endif

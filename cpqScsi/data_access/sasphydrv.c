#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>

#include <net-snmp/agent/net-snmp-agent-includes.h>
#include "cpqSasPhyDrv.h"
#include "sasphydrv.h"


/**---------------------------------------------------------------------*/
/*
 * local static vars
 */
static int _sasphydrv_init = 0;
       int _sasphydrv_max  = 0;
static netsnmp_cache     *sasphydrv_cache     = NULL;
static netsnmp_container *sasphydrv_container = NULL;

void netsnmp_sasphydrv_container_free(netsnmp_container *);
netsnmp_container* netsnmp_sasphydrv_container_load(netsnmp_container*);
void netsnmp_sasphydrv_container_free_items(netsnmp_container *);

netsnmp_container * netsnmp_sasphydrv_container(void);
netsnmp_cache     * netsnmp_sasphydrv_cache    (void);

/*
 * local static prototypes
 */
static void _sasphydrv_entry_release(netsnmp_sasphydrv_entry * entry,
                                            void *unused);

/**---------------------------------------------------------------------*/
/*
 * external per-architecture functions prototypes
 *
 * These shouldn't be called by the general public, so they aren't in
 * the header file.
 */
extern int netsnmp_arch_sasphydrv_container_load(netsnmp_container* container);

/**
 * initialization
 */
void
init_sasphydrv(void)
{
    DEBUGMSGTL(("sasphydrv:access", "init\n"));

    netsnmp_assert(0 == _sasphydrv_init); /* who is calling twice? */

    if (1 == _sasphydrv_init)
        return;

    _sasphydrv_init = 1;

    (void)netsnmp_sasphydrv_container();
    (void) netsnmp_sasphydrv_cache();
}

void
shutdown_sasphydrv(void)
{
    DEBUGMSGTL(("sasphydrv:access", "shutdown\n"));

}

/**---------------------------------------------------------------------*/
/*
 * cache functions
 */

static int
_cache_load( netsnmp_cache *cache,  void *magic )
{
    netsnmp_sasphydrv_container_load( sasphydrv_container);
    return 0;
}

static void
_cache_free( netsnmp_cache *cache,  void *magic )
{
    //netsnmp_sasphydrv_container_free_items( sasphydrv_container );
    return;
}

/**
 * create sasphydrv cache
 */
netsnmp_cache *
netsnmp_sasphydrv_cache(void)
{
    DEBUGMSGTL(("sasphydrv:cache", "init\n"));
    if ( !sasphydrv_cache ) {
        sasphydrv_cache = netsnmp_cache_create(3600,   /* timeout in seconds */
                           _cache_load,  _cache_free,
                           cpqSasPhyDrvTable_oid, cpqSasPhyDrvTable_oid_len);
        if (sasphydrv_cache)
            sasphydrv_cache->flags = NETSNMP_CACHE_PRELOAD |
                                     NETSNMP_CACHE_DONT_FREE_EXPIRED | 
                                     NETSNMP_CACHE_DONT_AUTO_RELEASE |
                                     NETSNMP_CACHE_AUTO_RELOAD;
    }
    return sasphydrv_cache;
}


/**---------------------------------------------------------------------*/
/*
 * container functions
 */
/**
 * create sasphydrv container
 */
netsnmp_container *
netsnmp_sasphydrv_container(void)
{

    DEBUGMSGTL(("sasphydrv:container", "init\n"));

    /*
     * create the container.
     */
    if (!sasphydrv_container) {
        sasphydrv_container = netsnmp_container_find("sasphydrv:binary_array");
        if (NULL == sasphydrv_container)
            return NULL;

        sasphydrv_container->container_name = strdup("sasphydrv container");
    }

    return sasphydrv_container;
}

/**
 * load sasphydrv information in specified container
 *
 * @param container empty container to be filled.
 *                  pass NULL to have the function create one.
 *
 * @retval NULL  error
 * @retval !NULL pointer to container
 */
netsnmp_container*
netsnmp_sasphydrv_container_load(netsnmp_container* user_container)
{
    netsnmp_container* container = user_container;
    int rc;

    DEBUGMSGTL(("sasphydrv:container:load", "load\n"));
    //netsnmp_assert(1 == _sasphydrv_init);

    if (NULL == container)
        container = netsnmp_sasphydrv_container();
    if (NULL == container) {
        snmp_log(LOG_ERR, "no container specified/found for sasphydrv\n");
        return NULL;
    }

    rc =  netsnmp_arch_sasphydrv_container_load(container);
    if (0 != rc) {
        if (NULL == user_container) {
            netsnmp_sasphydrv_container_free(container);
            container = NULL;
        }
    }

    return container;
}

void
netsnmp_sasphydrv_container_free(netsnmp_container *container)
{
    DEBUGMSGTL(("sasphydrv:container", "free\n"));

    if (NULL == container) {
        snmp_log(LOG_ERR, "invalid container for netsnmp_sasphydrv_container_free\n");
        return;
    }

    netsnmp_sasphydrv_container_free_items(container);

    CONTAINER_FREE(container);
}

void
netsnmp_sasphydrv_container_free_items(netsnmp_container *container)
{
    DEBUGMSGTL(("sasphydrv:container", "free_items\n"));

    if (NULL == container) {
        snmp_log(LOG_ERR, "invalid container for netsnmp_sasphydrv_container_free_items\n");
        return;
    }

    /*
     * free all items.
     */
    CONTAINER_CLEAR(container,
                    (netsnmp_container_obj_func*)_sasphydrv_entry_release,
                    NULL);
}

/**---------------------------------------------------------------------*/
/*
 * sasphydrv_entry functions
 */
/**
 */
netsnmp_sasphydrv_entry *
netsnmp_sasphydrv_entry_get_by_index(netsnmp_container *container, oid index)
{
    netsnmp_index   tmp;

    DEBUGMSGTL(("sasphydrv:entry", "by_index\n"));
    netsnmp_assert(1 == _sasphydrv_init);

    if (NULL == container) {
        snmp_log(LOG_ERR,
                "invalid container for netsnmp_sasphydrv_entry_get_by_index\n");
        return NULL;
    }

    tmp.len = 1;
    tmp.oids = &index;

    return (netsnmp_sasphydrv_entry *) CONTAINER_FIND(container, &tmp);
}

/**
 */
netsnmp_sasphydrv_entry *
netsnmp_sasphydrv_entry_create(oid index, oid index2)
{
    netsnmp_sasphydrv_entry *entry =
        SNMP_MALLOC_TYPEDEF(netsnmp_sasphydrv_entry);

    if(NULL == entry)
        return NULL;

    memset(entry,0,sizeof(netsnmp_sasphydrv_entry));
    entry->Hba  = index;
    entry->Index  = index2;

    entry->oid_index.len = 2;
    entry->oid_index.oids = (oid *) & entry->Hba;

    return entry;
}

/**
 */
NETSNMP_INLINE void
netsnmp_sasphydrv_entry_free(netsnmp_sasphydrv_entry * entry)
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
_sasphydrv_entry_release(netsnmp_sasphydrv_entry * entry, void *context)
{
    netsnmp_sasphydrv_entry_free(entry);
}


#ifdef TEST
int main(int argc, char *argv[])
{
    const char *tokens = getenv("SNMP_DEBUG");

    netsnmp_container_init_list();

    /** sasphydrv,verbose:sasphydrv,9:sasphydrv,8:sasphydrv,5:sasphydrv */
    if (tokens)
        debug_register_tokens(tokens);
    else
        debug_register_tokens("sasphydrv");
    snmp_set_do_debugging(1);

    init_sasphydrv();
    netsnmp_sasphydrv_container_load(NULL, 0);
    shutdown_sasphydrv();

    return 0;
}
#endif

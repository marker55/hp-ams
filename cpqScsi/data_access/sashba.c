#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>

#include <net-snmp/agent/net-snmp-agent-includes.h>
#include "cpqSasHba.h"
#include "sashba.h"


/**---------------------------------------------------------------------*/
/*
 * local static vars
 */
static int _sashba_init = 0;
       int _sashba_max  = 0;
static netsnmp_cache     *sashba_cache     = NULL;
static netsnmp_container *sashba_container = NULL;

void netsnmp_sashba_container_free(netsnmp_container *);
netsnmp_container* netsnmp_sashba_container_load(netsnmp_container*);
void netsnmp_sashba_container_free_items(netsnmp_container *);

netsnmp_container * netsnmp_sashba_container(void);
netsnmp_cache     * netsnmp_sashba_cache    (void);

/*
 * local static prototypes
 */
static void _sashba_entry_release(netsnmp_sashba_entry * entry,
                                            void *unused);

/**---------------------------------------------------------------------*/
/*
 * external per-architecture functions prototypes
 *
 * These shouldn't be called by the general public, so they aren't in
 * the header file.
 */
extern int netsnmp_arch_sashba_container_load(netsnmp_container* container);

/**
 * initialization
 */
void
init_sashba(void)
{
    DEBUGMSGTL(("sashba:access", "init\n"));

    netsnmp_assert(0 == _sashba_init); /* who is calling twice? */

    if (1 == _sashba_init)
        return;

    _sashba_init = 1;

    (void)netsnmp_sashba_container();
    (void) netsnmp_sashba_cache();
}

void
shutdown_sashba(void)
{
    DEBUGMSGTL(("sashba:access", "shutdown\n"));

}

/**---------------------------------------------------------------------*/
/*
 * cache functions
 */

static int
_cache_load( netsnmp_cache *cache,  void *magic )
{
    netsnmp_sashba_container_load( sashba_container);
    return 0;
}

static void
_cache_free( netsnmp_cache *cache,  void *magic )
{
    netsnmp_sashba_container_free_items( sashba_container );
    return;
}

/**
 * create sashba cache
 */
netsnmp_cache *
netsnmp_sashba_cache(void)
{
    DEBUGMSGTL(("sashba:cache", "init\n"));
    if ( !sashba_cache ) {
        sashba_cache = netsnmp_cache_create(7200,   /* timeout in seconds */
                           _cache_load,  _cache_free,
                           cpqSasHbaTable_oid, cpqSasHbaTable_oid_len);
        if (sashba_cache) {
            sashba_cache->flags = NETSNMP_CACHE_PRELOAD |
                                  NETSNMP_CACHE_DONT_FREE_EXPIRED |
                                  NETSNMP_CACHE_DONT_AUTO_RELEASE |
                                  NETSNMP_CACHE_DONT_INVALIDATE_ON_SET;
            if (sashba_container) 
                sashba_cache->magic = sashba_container;
        }
    }
    return sashba_cache;
}


/**---------------------------------------------------------------------*/
/*
 * container functions
 */
/**
 * create sashba container
 */
netsnmp_container *
netsnmp_sashba_container(void)
{
    DEBUGMSGTL(("sashba:container", "init\n"));

    /*
     * create the container.
     */
  if (!sashba_container) {
    sashba_container = netsnmp_container_find("sashba:binary_array");
    if (NULL == sashba_container)
        return NULL;

    sashba_container->container_name = strdup("sashba container");
  }

    return sashba_container;
}

/**
 * load sashba information in specified container
 *
 * @param container empty container to be filled.
 *                  pass NULL to have the function create one.
 *
 * @retval NULL  error
 * @retval !NULL pointer to container
 */
netsnmp_container*
netsnmp_sashba_container_load(netsnmp_container* user_container)
{
    netsnmp_container* container = user_container;
    int rc;

    DEBUGMSGTL(("sashba:container:load", "load\n"));
    //netsnmp_assert(1 == _sashba_init);

    if (NULL == container)
        container = netsnmp_sashba_container();
    if (NULL == container) {
        snmp_log(LOG_ERR, "no container specified/found for sashba\n");
        return NULL;
    }

    rc =  netsnmp_arch_sashba_container_load(container);
    DEBUGMSGTL(("sashba:container:load", "rc = %d\n",rc));
    if (0 != rc) {
        if (NULL == user_container) {
            netsnmp_sashba_container_free(container);
            container = NULL;
        }
    }

    return container;
}

void
netsnmp_sashba_container_free(netsnmp_container *container)
{
    DEBUGMSGTL(("sashba:container", "free\n"));

    if (NULL == container) {
        snmp_log(LOG_ERR, "invalid container for netsnmp_sashba_container_free\n");
        return;
    }

    netsnmp_sashba_container_free_items(container);

    CONTAINER_FREE(container);
}

void
netsnmp_sashba_container_free_items(netsnmp_container *container)
{
    DEBUGMSGTL(("sashba:container", "free_items\n"));

    if (NULL == container) {
        snmp_log(LOG_ERR, "invalid container for netsnmp_sashba_container_free_items\n");
        return;
    }

    /*
     * free all items.
     */
    CONTAINER_CLEAR(container,
                    (netsnmp_container_obj_func*)_sashba_entry_release,
                    NULL);
}

/**---------------------------------------------------------------------*/
/*
 * sashba_entry functions
 */
/**
 */
netsnmp_sashba_entry *
netsnmp_sashba_entry_get_by_index(netsnmp_container *container, oid index)
{
    netsnmp_index   tmp;

    DEBUGMSGTL(("sashba:entry", "by_index\n"));
    netsnmp_assert(1 == _sashba_init);

    if (NULL == container) {
        snmp_log(LOG_ERR,
                 "invalid container for netsnmp_sashba_entry_get_by_index\n");
        return NULL;
    }

    tmp.len = 1;
    tmp.oids = &index;

    return (netsnmp_sashba_entry *) CONTAINER_FIND(container, &tmp);
}

/**
 */
netsnmp_sashba_entry *
netsnmp_sashba_entry_create(oid index)
{
    netsnmp_sashba_entry *entry =
        SNMP_MALLOC_TYPEDEF(netsnmp_sashba_entry);

    if(NULL == entry)
        return NULL;

    memset(entry,0,sizeof(netsnmp_sashba_entry));
    entry->Hba  = index;

    entry->oid_index.len = 1;
    entry->oid_index.oids = (oid *) & entry->Hba;

    return entry;
}

/**
 */
NETSNMP_INLINE void
netsnmp_sashba_entry_free(netsnmp_sashba_entry * entry)
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
_sashba_entry_release(netsnmp_sashba_entry * entry, void *context)
{
    netsnmp_sashba_entry_free(entry);
}


#ifdef TEST
int main(int argc, char *argv[])
{
    const char *tokens = getenv("SNMP_DEBUG");

    netsnmp_container_init_list();

    /** sashba,verbose:sashba,9:sashba,8:sashba,5:sashba */
    if (tokens)
        debug_register_tokens(tokens);
    else
        debug_register_tokens("sashba");
    snmp_set_do_debugging(1);

    init_sashba();
    netsnmp_sashba_container_load(NULL, 0);
    shutdown_sashba();

    return 0;
}
#endif

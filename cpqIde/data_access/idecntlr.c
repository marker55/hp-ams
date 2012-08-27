#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>

#include <net-snmp/agent/net-snmp-agent-includes.h>
#include "cpqIdeCntlr.h"
#include "idecntlr.h"


/**---------------------------------------------------------------------*/
/*
 * local static vars
 */
static int _idecntlr_init = 0;
       int _idecntlr_max  = 0;
static netsnmp_cache     *idecntlr_cache     = NULL;
static netsnmp_container *idecntlr_container = NULL;

void netsnmp_idecntlr_container_free(netsnmp_container *);
netsnmp_container* netsnmp_idecntlr_container_load(netsnmp_container*);
void netsnmp_idecntlr_container_free_items(netsnmp_container *);

netsnmp_container * netsnmp_idecntlr_container(void);
netsnmp_cache     * netsnmp_idecntlr_cache    (void);

/*
 * local static prototypes
 */
static void _idecntlr_entry_release(netsnmp_idecntlr_entry * entry,
                                            void *unused);

/**---------------------------------------------------------------------*/
/*
 * external per-architecture functions prototypes
 *
 * These shouldn't be called by the general public, so they aren't in
 * the header file.
 */
extern int netsnmp_arch_idecntlr_container_load(netsnmp_container* container);

/**
 * initialization
 */
void
init_idecntlr(void)
{
    DEBUGMSGTL(("idecntlr:access", "init\n"));

    netsnmp_assert(0 == _idecntlr_init); /* who is calling twice? */

    if (1 == _idecntlr_init)
        return;

    _idecntlr_init = 1;

    (void)netsnmp_idecntlr_container();
    (void) netsnmp_idecntlr_cache();
}

void
shutdown_idecntlr(void)
{
    DEBUGMSGTL(("idecntlr:access", "shutdown\n"));

}

/**---------------------------------------------------------------------*/
/*
 * cache functions
 */

static int
_cache_load( netsnmp_cache *cache,  void *magic )
{
    netsnmp_idecntlr_container_load( idecntlr_container);
    return 0;
}

static void
_cache_free( netsnmp_cache *cache,  void *magic )
{
    netsnmp_idecntlr_container_free_items( idecntlr_container );
    return;
}

/**
 * create idecntlr cache
 */
netsnmp_cache *
netsnmp_idecntlr_cache(void)
{
    DEBUGMSGTL(("idecntlr:cache", "init\n"));
    if ( !idecntlr_cache ) {
        idecntlr_cache = netsnmp_cache_create(240,   /* timeout in seconds */
                           _cache_load,  _cache_free,
                           cpqIdeCntlrTable_oid, cpqIdeCntlrTable_oid_len);
        if (idecntlr_cache) {
            idecntlr_cache->flags = NETSNMP_CACHE_DONT_INVALIDATE_ON_SET;
            if (idecntlr_container) 
                idecntlr_cache->magic = idecntlr_container;
        }
    }
    return idecntlr_cache;
}


/**---------------------------------------------------------------------*/
/*
 * container functions
 */
/**
 * create idecntlr container
 */
netsnmp_container *
netsnmp_idecntlr_container(void)
{
    DEBUGMSGTL(("idecntlr:container", "init\n"));

    /*
     * create the container.
     */
  if (!idecntlr_container) {
    idecntlr_container = netsnmp_container_find("idecntlr:binary_array");
    if (NULL == idecntlr_container)
        return NULL;

    idecntlr_container->container_name = strdup("idecntlr container");
  }

    return idecntlr_container;
}

/**
 * load idecntlr information in specified container
 *
 * @param container empty container to be filled.
 *                  pass NULL to have the function create one.
 *
 * @retval NULL  error
 * @retval !NULL pointer to container
 */
netsnmp_container*
netsnmp_idecntlr_container_load(netsnmp_container* user_container)
{
    netsnmp_container* container = user_container;
    int rc;

    DEBUGMSGTL(("idecntlr:container:load", "load\n"));
    //netsnmp_assert(1 == _idecntlr_init);

    if (NULL == container)
        container = netsnmp_idecntlr_container();
    if (NULL == container) {
        snmp_log(LOG_ERR, "no container specified/found for idecntlr\n");
        return NULL;
    }

    rc =  netsnmp_arch_idecntlr_container_load(container);
    DEBUGMSGTL(("idecntlr:container:load", "rc = %d\n",rc));
    if (0 != rc) {
        if (NULL == user_container) {
            netsnmp_idecntlr_container_free(container);
            container = NULL;
        }
    }

    return container;
}

void
netsnmp_idecntlr_container_free(netsnmp_container *container)
{
    DEBUGMSGTL(("idecntlr:container", "free\n"));

    if (NULL == container) {
        snmp_log(LOG_ERR, "invalid container for netsnmp_idecntlr_container_free\n");
        return;
    }

    netsnmp_idecntlr_container_free_items(container);

    CONTAINER_FREE(container);
}

void
netsnmp_idecntlr_container_free_items(netsnmp_container *container)
{
    DEBUGMSGTL(("idecntlr:container", "free_items\n"));

    if (NULL == container) {
        snmp_log(LOG_ERR, "invalid container for netsnmp_idecntlr_container_free_items\n");
        return;
    }

    /*
     * free all items.
     */
    CONTAINER_CLEAR(container,
                    (netsnmp_container_obj_func*)_idecntlr_entry_release,
                    NULL);
}

/**---------------------------------------------------------------------*/
/*
 * idecntlr_entry functions
 */
/**
 */
netsnmp_idecntlr_entry *
netsnmp_idecntlr_entry_get_by_index(netsnmp_container *container, oid index)
{
    netsnmp_index   tmp;

    DEBUGMSGTL(("idecntlr:entry", "by_index\n"));
    netsnmp_assert(1 == _idecntlr_init);

    if (NULL == container) {
        snmp_log(LOG_ERR,
                 "invalid container for netsnmp_idecntlr_entry_get_by_index\n");
        return NULL;
    }

    tmp.len = 1;
    tmp.oids = &index;

    return (netsnmp_idecntlr_entry *) CONTAINER_FIND(container, &tmp);
}

/**
 */
netsnmp_idecntlr_entry *
netsnmp_idecntlr_entry_create(oid index)
{
    netsnmp_idecntlr_entry *entry =
        SNMP_MALLOC_TYPEDEF(netsnmp_idecntlr_entry);

    if(NULL == entry)
        return NULL;

    memset(entry,0,sizeof(netsnmp_idecntlr_entry));
    entry->Index  = index;

    entry->oid_index.len = 1;
    entry->oid_index.oids = (oid *) & entry->Index;

    return entry;
}

/**
 */
NETSNMP_INLINE void
netsnmp_idecntlr_entry_free(netsnmp_idecntlr_entry * entry)
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
_idecntlr_entry_release(netsnmp_idecntlr_entry * entry, void *context)
{
    netsnmp_idecntlr_entry_free(entry);
}


#ifdef TEST
int main(int argc, char *argv[])
{
    const char *tokens = getenv("SNMP_DEBUG");

    netsnmp_container_init_list();

    /** idecntlr,verbose:idecntlr,9:idecntlr,8:idecntlr,5:idecntlr */
    if (tokens)
        debug_register_tokens(tokens);
    else
        debug_register_tokens("idecntlr");
    snmp_set_do_debugging(1);

    init_idecntlr();
    netsnmp_idecntlr_container_load(NULL, 0);
    shutdown_idecntlr();

    return 0;
}
#endif

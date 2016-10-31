/*
 * Note: this file originally auto-generated by mib2c using
 *  : mib2c.container.conf 17798 2009-10-27 06:44:54Z magfr $
 */

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <net-snmp/agent/table_container.h>
#include "cpqNicIfLogMapTable.h"

extern void     netsnmp_arch_iflog_container_load(netsnmp_container*);
extern void     cpqNicIfLogMap_reload_entry(cpqNicIfLogMapTable_entry *);
static void     _cache_free(netsnmp_cache * cache, void *magic);
static int      _cache_load(netsnmp_cache * cache, void *vmagic);

const oid       cpqNicIfLogMapTable_oid[] =
        { 1, 3, 6, 1, 4, 1, 232, 18, 2, 2, 1 };
const size_t    cpqNicIfLogMapTable_oid_len =
        OID_LENGTH(cpqNicIfLogMapTable_oid);
extern long     get_cpqNicIfLogMapOverallCondition(void);

#define CPQNICIFLOGMAPOVERALLCONDITION            2

void
initialize_cpqNicIfLogMapOverallCondition(void)
{
    const oid       cpqNicIfLogMapOverallCondition_oid[] =
        { 1, 3, 6, 1, 4, 1, 232, 18, 2, 2 };
    const size_t    cpqNicIfLogMapOverallCondition_oid_len =
        OID_LENGTH(cpqNicIfLogMapOverallCondition_oid);

    DEBUGMSGTL(("cpqNic", "Initializing cpqNicIfLogMapOverallCondition\n"));

    /*
     * register ourselves with the agent to handle our mib tree
     */
    netsnmp_register_scalar_group(
        netsnmp_create_handler_registration("cpqNicIfLogMapOverallCondition",
                             cpqNicIfLogMapOverallCondition_handler,
                             cpqNicIfLogMapOverallCondition_oid, cpqNicIfLogMapOverallCondition_oid_len,
                             HANDLER_CAN_RONLY),
                             CPQNICIFLOGMAPOVERALLCONDITION, CPQNICIFLOGMAPOVERALLCONDITION);
}

int
cpqNicIfLogMapOverallCondition_handler(netsnmp_mib_handler          *handler,
               netsnmp_handler_registration *reginfo,
               netsnmp_agent_request_info   *reqinfo,
               netsnmp_request_info         *requests)
{
    long val = 0;

    switch (reqinfo->mode) {
    case MODE_GET:
        switch (requests->requestvb->name[ reginfo->rootoid_len - 2 ]) {
            case CPQNICIFLOGMAPOVERALLCONDITION:
                val = get_cpqNicIfLogMapOverallCondition();
                break;
            default:
                /*
                 * An unsupported/unreadable column (if applicable)
                 */
                snmp_log(LOG_ERR, "unknown object (%lu) in cpqNicIfLogMapOverallCondition_handler\n",
                     requests->requestvb->name[ reginfo->rootoid_len - 2 ]);
                netsnmp_set_request_error( reqinfo, requests,
                                            SNMP_NOSUCHOBJECT );
                return SNMP_ERR_NOERROR;
        }
        snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER,
                                 (u_char *)&val, sizeof(val));
        break;

    default:
        snmp_log(LOG_ERR, "unknown mode (%d) in cpqNicIfLogMapOverallCondition_handler\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;

}

/** Initializes the cpqNicIfLogMapTable module */
void
init_cpqNicIfLogMapTable(void)
{
    /*
     * here we initialize all the tables we're planning on supporting 
     */
    initialize_table_cpqNicIfLogMapTable();
}

/** Initialize the cpqNicIfLogMapTable table by defining its contents and how it's structured */
void
initialize_table_cpqNicIfLogMapTable(void)
{
    const oid       cpqNicIfLogMapTable_oid[] =
        { 1, 3, 6, 1, 4, 1, 232, 18, 2, 2, 1 };
    const size_t    cpqNicIfLogMapTable_oid_len =
        OID_LENGTH(cpqNicIfLogMapTable_oid);
    netsnmp_handler_registration *reg = NULL;
    netsnmp_mib_handler *handler = NULL;
    netsnmp_container *container = NULL;
    netsnmp_table_registration_info *table_info = NULL;
    netsnmp_cache  *cache = NULL;

    int reg_tbl_ret = SNMPERR_SUCCESS;

    DEBUGMSGTL(("cpqNicIfLogMapTable:init",
                "initializing table cpqNicIfLogMapTable\n"));

    reg =
        netsnmp_create_handler_registration("cpqNicIfLogMapTable",
                                            cpqNicIfLogMapTable_handler,
                                            cpqNicIfLogMapTable_oid,
                                            cpqNicIfLogMapTable_oid_len,
                                            HANDLER_CAN_RONLY);
    if (NULL == reg) {
        snmp_log(LOG_ERR,
                 "error creating handler registration for cpqNicIfLogMapTable\n");
        goto bail;
    }

    container =
        netsnmp_container_find("cpqNicIfLogMapTable:table_container");
    if (NULL == container) {
        snmp_log(LOG_ERR,
                 "error creating container for cpqNicIfLogMapTable\n");
        goto bail;
    }
    container->container_name = strdup("cpqNicIfLogMapTable container");


    table_info = SNMP_MALLOC_TYPEDEF(netsnmp_table_registration_info);
    if (NULL == table_info) {
        snmp_log(LOG_ERR,
                 "error allocating table registration for cpqNicIfLogMapTable\n");
        goto bail;
    }
    netsnmp_table_helper_add_indexes(table_info, ASN_INTEGER,   /* index: cpqNicIfLogMapIndex */
                                     0);
    table_info->min_column = COLUMN_CPQNICIFLOGMAPINDEX;
    table_info->max_column = COLUMN_CPQNICIFLOGMAPPCILOCATION;

    /*************************************************
     *
     * inject container_table helper
     */
    handler = netsnmp_container_table_handler_get(table_info, container,
                                                  TABLE_CONTAINER_KEY_NETSNMP_INDEX);
    if (NULL == handler) {
        snmp_log(LOG_ERR,
                 "error allocating table registration for cpqNicIfLogMapTable\n");
        goto bail;
    }
    if (SNMPERR_SUCCESS != netsnmp_inject_handler(reg, handler)) {
        snmp_log(LOG_ERR,
                 "error injecting container_table handler for cpqNicIfLogMapTable\n");
        goto bail;
    }
    handler = NULL;             /* reg has it, will reuse below */

    /*************************************************
     *
     * inject cache helper
     */
    cache = netsnmp_cache_create(30,    /* timeout in seconds */
                                 _cache_load, _cache_free,
                                 cpqNicIfLogMapTable_oid,
                                 cpqNicIfLogMapTable_oid_len);

    if (NULL == cache) {
        snmp_log(LOG_ERR,
                 "error creating cache for cpqNicIfLogMapTable\n");
        goto bail;
    }
    cache->flags = NETSNMP_CACHE_PRELOAD |
                   NETSNMP_CACHE_DONT_FREE_EXPIRED |
                   NETSNMP_CACHE_DONT_AUTO_RELEASE |
                   NETSNMP_CACHE_DONT_FREE_BEFORE_LOAD |
                   NETSNMP_CACHE_DONT_INVALIDATE_ON_SET;

    cache->magic = container;

    handler = netsnmp_cache_handler_get(cache);
    if (NULL == handler) {
        snmp_log(LOG_ERR,
                 "error creating cache handler for cpqNicIfLogMapTable\n");
        goto bail;
    }

    if (SNMPERR_SUCCESS != netsnmp_inject_handler(reg, handler)) {
        snmp_log(LOG_ERR,
                 "error injecting cache handler for cpqNicIfLogMapTable\n");
        goto bail;
    }
    handler = NULL;             /* reg has it */

    /*
     * register the table
     */
    reg_tbl_ret = netsnmp_register_table(reg, table_info);
    if (reg_tbl_ret != SNMPERR_SUCCESS) {
        snmp_log(LOG_ERR,
                 "error registering table handler for cpqNicIfLogMapTable\n");
        goto bail;
    }

    return;                     /* ok */

    /*
     * Some error occurred during registration. Clean up and bail.
     */
  bail:                        /* not ok */

    if (handler)
        netsnmp_handler_free(handler);

    if (cache)
        netsnmp_cache_free(cache);

    if (table_info)
        netsnmp_table_registration_info_free(table_info);

    if (container)
        CONTAINER_FREE(container);

    if (reg_tbl_ret == SNMPERR_SUCCESS)
        if (reg)
            netsnmp_handler_registration_free(reg);
}

/** create a new row in the table */
cpqNicIfLogMapTable_entry *
cpqNicIfLogMapTable_createEntry(netsnmp_container * container,
                                oid cpqNicIfLogMapIndex)
{
    cpqNicIfLogMapTable_entry *entry;

    entry = SNMP_MALLOC_TYPEDEF(cpqNicIfLogMapTable_entry);
    if (!entry)
        return NULL;

    entry->cpqNicIfLogMapIndex = cpqNicIfLogMapIndex;
    entry->oid_index.len = 1;
    entry->oid_index.oids = (oid *) &entry->cpqNicIfLogMapIndex;

    return entry;
}

/** remove a row from the table */
void
cpqNicIfLogMapTable_removeEntry(netsnmp_container * container,
                                cpqNicIfLogMapTable_entry * entry)
{

    if (!entry)
        return;                 /* Nothing to remove */
    CONTAINER_REMOVE(container, entry);
    if (entry)
        SNMP_FREE(entry);       /* XXX - release any other internal resources */
}

/** remove a row from the table */
void
cpqNicIfLogMapTable_freeEntry(cpqNicIfLogMapTable_entry * entry)
{

    if (!entry)
        return;                 /* Nothing to remove */
    SNMP_FREE(entry);           /* XXX - release any other internal resources */
}

/** handles requests for the cpqNicIfLogMapTable table */
int
cpqNicIfLogMapTable_handler(netsnmp_mib_handler *handler,
                            netsnmp_handler_registration *reginfo,
                            netsnmp_agent_request_info *reqinfo,
                            netsnmp_request_info *requests)
{

    netsnmp_request_info *request;
    netsnmp_table_request_info *table_info;
    cpqNicIfLogMapTable_entry *table_entry;

    DEBUGMSGTL(("cpqNicIfLogMapTable:handler", "Processing request (%d)\n",
                reqinfo->mode));

    switch (reqinfo->mode) {
        /*
         * Read-support (also covers GetNext requests)
         */
    case MODE_GET:
        for (request = requests; request; request = request->next) {
            if (request->processed)
                continue;
            table_entry = (cpqNicIfLogMapTable_entry *)
                netsnmp_container_table_extract_context(request);
            table_info = netsnmp_extract_table_info(request);
            if ((NULL == table_entry) || (NULL == table_info)) {
                snmp_log(LOG_ERR,
                         "could not extract table entry or info for cpqNicIfLogMapTable\n");
                snmp_set_var_typed_value(request->requestvb,
                                         SNMP_ERR_GENERR, NULL, 0);
                continue;
            }

            switch (table_info->colnum) {
            case COLUMN_CPQNICIFLOGMAPINDEX:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_INTEGER,
                                           table_entry->
                                           cpqNicIfLogMapIndex);
                break;
            case COLUMN_CPQNICIFLOGMAPIFNUMBER:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
                                         table_entry->
                                         cpqNicIfLogMapIfNumber,
                                         table_entry->
                                         cpqNicIfLogMapIfNumber_len);
                break;
            case COLUMN_CPQNICIFLOGMAPDESCRIPTION:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
                                         table_entry->
                                         cpqNicIfLogMapDescription,
                                         table_entry->
                                         cpqNicIfLogMapDescription_len);
                break;
            case COLUMN_CPQNICIFLOGMAPGROUPTYPE:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_INTEGER,
                                           table_entry->
                                           cpqNicIfLogMapGroupType);
                break;
            case COLUMN_CPQNICIFLOGMAPADAPTERCOUNT:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_INTEGER,
                                           table_entry->
                                           cpqNicIfLogMapAdapterCount);
                break;
            case COLUMN_CPQNICIFLOGMAPADAPTEROKCOUNT:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_INTEGER,
                                           table_entry->
                                           cpqNicIfLogMapAdapterOKCount);
                break;
            case COLUMN_CPQNICIFLOGMAPPHYSICALADAPTERS:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
                                         table_entry->
                                             cpqNicIfLogMapPhysicalAdapters,
                                         table_entry->
                                             cpqNicIfLogMapPhysicalAdapters_len);
                break;
            case COLUMN_CPQNICIFLOGMAPMACADDRESS:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
                                         table_entry->
                                         cpqNicIfLogMapMACAddress,
                                         table_entry->
                                         cpqNicIfLogMapMACAddress_len);
                break;
            case COLUMN_CPQNICIFLOGMAPSWITCHOVERMODE:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_INTEGER,
                                           table_entry->
                                           cpqNicIfLogMapSwitchoverMode);
                break;
            case COLUMN_CPQNICIFLOGMAPCONDITION:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_INTEGER,
                                           table_entry->
                                           cpqNicIfLogMapCondition);
                break;
            case COLUMN_CPQNICIFLOGMAPSTATUS:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_INTEGER,
                                           table_entry->
                                           cpqNicIfLogMapStatus);
                break;
            case COLUMN_CPQNICIFLOGMAPNUMSWITCHOVERS:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_COUNTER,
                                           table_entry->
                                           cpqNicIfLogMapNumSwitchovers);
                break;
            case COLUMN_CPQNICIFLOGMAPHWLOCATION:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
                                         table_entry->
                                         cpqNicIfLogMapHwLocation,
                                         table_entry->
                                         cpqNicIfLogMapHwLocation_len);
                break;
            case COLUMN_CPQNICIFLOGMAPSPEED:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_GAUGE,
                                           table_entry->
                                           cpqNicIfLogMapSpeed);
                break;
            case COLUMN_CPQNICIFLOGMAPVLANCOUNT:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_INTEGER,
                                           table_entry->
                                           cpqNicIfLogMapVlanCount);
                break;
            case COLUMN_CPQNICIFLOGMAPVLANS:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
                                         table_entry->cpqNicIfLogMapVlans,
                                         table_entry->
                                         cpqNicIfLogMapVlans_len);
                break;
            case COLUMN_CPQNICIFLOGMAPLASTCHANGE:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb,
                                           ASN_TIMETICKS,
                                           table_entry->
                                           cpqNicIfLogMapLastChange);
                break;
            case COLUMN_CPQNICIFLOGMAPADVANCEDTEAMING:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_INTEGER,
                                           table_entry->
                                           cpqNicIfLogMapAdvancedTeaming);
                break;
            case COLUMN_CPQNICIFLOGMAPSPEEDMBPS:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_GAUGE,
                                           table_entry->
                                           cpqNicIfLogMapSpeedMbps);
                break;
            case COLUMN_CPQNICIFLOGMAPIPV6ADDRESS:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
                                         table_entry->
                                         cpqNicIfLogMapIPV6Address,
                                         table_entry->
                                         cpqNicIfLogMapIPV6Address_len);
                break;
            case COLUMN_CPQNICIFLOGMAPLACNUMBER:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
                                         table_entry->
                                         cpqNicIfLogMapLACNumber,
                                         table_entry->
                                         cpqNicIfLogMapLACNumber_len);
                break;
            case COLUMN_CPQNICIFLOGMAPPCILOCATION:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
                                         table_entry->
                                         cpqNicIfLogMapPciLocation,
                                         table_entry->
                                         cpqNicIfLogMapPciLocation_len);
                break;
            default:
                netsnmp_set_request_error(reqinfo, request,
                                          SNMP_NOSUCHOBJECT);
                break;
            }
        }
        break;

    }
    return SNMP_ERR_NOERROR;
}

void
cpqNicIfLogMapTable_cache_remove(oid index)
{
    netsnmp_cache  *cpqNicIfLogMapTable_cache = NULL;
    netsnmp_container *iflogmap_container;
    netsnmp_iterator  *it;
    cpqNicIfLogMapTable_entry* entry = NULL;

    DEBUGMSGTL(("internal:cpqNicIfLogMapTable:_cache_reload", "triggered\n"));

    cpqNicIfLogMapTable_cache = netsnmp_cache_find_by_oid(cpqNicIfLogMapTable_oid,
                                            cpqNicIfLogMapTable_oid_len);

    if (cpqNicIfLogMapTable_cache != NULL) {
        iflogmap_container = cpqNicIfLogMapTable_cache->magic;
        it = CONTAINER_ITERATOR(iflogmap_container);

        entry = ITERATOR_FIRST( it );
        while (entry != NULL ) {
            if (entry->cpqNicIfLogMapIndex == index)
                break;
            entry = ITERATOR_NEXT( it );
        }
        ITERATOR_RELEASE( it );
        if (entry != NULL)
            cpqNicIfLogMapTable_removeEntry(iflogmap_container, entry);
    }
}

void
cpqNicIfLogMapTable_cache_reload(int index)
{
    netsnmp_cache  *cpqNicIfLogMapTable_cache = NULL;
    netsnmp_container *iflogmap_container;
    netsnmp_iterator  *it;
    cpqNicIfLogMapTable_entry* entry = NULL;

    DEBUGMSGTL(("internal:cpqNicIfLogMapTable:_cache_reload", "triggered\n"));

    cpqNicIfLogMapTable_cache = netsnmp_cache_find_by_oid(cpqNicIfLogMapTable_oid,
                                            cpqNicIfLogMapTable_oid_len);

    if (cpqNicIfLogMapTable_cache != NULL) {
        iflogmap_container = cpqNicIfLogMapTable_cache->magic;
        it = CONTAINER_ITERATOR(iflogmap_container);

        entry = ITERATOR_FIRST( it );
        while (entry != NULL ) {
            if (entry->cpqNicIfLogMapIndex == index)
                break;
            entry = ITERATOR_NEXT( it );
        }
        ITERATOR_RELEASE( it );
        if (entry != NULL)
            cpqNicIfLogMap_reload_entry(entry);
    }
}

/**
 * @internal
 */
static int
_cache_load(netsnmp_cache * cache, void *vmagic)
{
    netsnmp_container *container;

    DEBUGMSGTL(("internal:cpqNicIfLogMapTable:_cache_load", "called\n"));

    if ((NULL == cache) || (NULL == cache->magic)) {
        snmp_log(LOG_ERR,
                 "invalid cache for cpqNicIfLogMapTable_cache_load\n");
        return -1;
    }
    container = (netsnmp_container *) cache->magic;

    /** should only be called for an invalid or expired cache */
    netsnmp_assert((0 == cache->valid) || (1 == cache->expired));

    /*
     * load cache here (or call function to do it)
     */
    netsnmp_arch_iflog_container_load(container);

    return 0;
}                               /* _cache_load */

/**
 * @Internal
 */
/** remove a row from the table */
static void
cpqNicIfLogMapTable_freeEntry_cb(cpqNicIfLogMapTable_entry * entry,
                                 void *magic)
{

    cpqNicIfLogMapTable_freeEntry(entry);
}

/**
 * @internal
 */
static void
_cache_free(netsnmp_cache * cache, void *magic)
{
    netsnmp_container *container;

    DEBUGMSGTL(("internal:cpqNicIfLogMapTable:_cache_free", "called\n"));

    if ((NULL == cache) || (NULL == cache->magic)) {
        snmp_log(LOG_ERR,
                 "invalid cache in cpqNicIfLogMapTable_cache_free\n");
        return;
    }
    container = (netsnmp_container *) cache->magic;

    /*
     * empty (but don't free) cache here
     */
    CONTAINER_CLEAR(container,
                    (netsnmp_container_obj_func *)
                    cpqNicIfLogMapTable_freeEntry_cb, NULL);
}                               /* _cache_free */

/*
 * Note: this file originally auto-generated by mib2c using
 *  : mib2c.container.conf 17798 2009-10-27 06:44:54Z magfr $
 */

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <net-snmp/agent/table_container.h>

#include "common/scsi_info.h"
#include "cpqSasHbaTable.h"

int HbaCondition;

extern int netsnmp_arch_sashba_container_load(netsnmp_container*);
static void     _cache_free(netsnmp_cache * cache, void *magic);
static int      _cache_load(netsnmp_cache * cache, void *vmagic);

oid       cpqSasHbaTable_oid[] =
        { 1, 3, 6, 1, 4, 1, 232, 5, 5, 1, 1 };
size_t    cpqSasHbaTable_oid_len = OID_LENGTH(cpqSasHbaTable_oid);

/** Initializes the cpqSasHbaTable module */
void
init_cpqSasHbaTable(void)
{
    /*
     * here we initialize all the tables we're planning on supporting 
     */
    initialize_table_cpqSasHbaTable();
}

/** Initialize the cpqSasHbaTable table by defining its contents and how it's structured */
void
initialize_table_cpqSasHbaTable(void)
{
    netsnmp_handler_registration *reg = NULL;
    netsnmp_mib_handler *handler = NULL;
    netsnmp_container *container = NULL;
    netsnmp_table_registration_info *table_info = NULL;
    netsnmp_cache  *cache = NULL;

    int reg_tbl_ret = SNMPERR_SUCCESS;

    DEBUGMSGTL(("cpqSasHbaTable:init",
                "initializing table cpqSasHbaTable\n"));

    reg =
        netsnmp_create_handler_registration("cpqSasHbaTable",
                                            cpqSasHbaTable_handler,
                                            cpqSasHbaTable_oid,
                                            cpqSasHbaTable_oid_len,
                                            HANDLER_CAN_RONLY);
    if (NULL == reg) {
        snmp_log(LOG_ERR,
                 "error creating handler registration for cpqSasHbaTable\n");
        goto bail;
    }

    container = netsnmp_container_find("cpqSasHbaTable:table_container");
    if (NULL == container) {
        snmp_log(LOG_ERR, "error creating container for cpqSasHbaTable\n");
        goto bail;
    }
    container->container_name = strdup("cpqSasHbaTable  container");


    table_info = SNMP_MALLOC_TYPEDEF(netsnmp_table_registration_info);
    if (NULL == table_info) {
        snmp_log(LOG_ERR,
                 "error allocating table registration for cpqSasHbaTable\n");
        goto bail;
    }
    netsnmp_table_helper_add_indexes(table_info, ASN_INTEGER,   /* index: cpqSasHbaIndex */
                                     0);
    table_info->min_column = COLUMN_CPQSASHBAINDEX;
    table_info->max_column = COLUMN_CPQSASHBABIOSVERSION;

    /*************************************************
     *
     * inject container_table helper
     */
    handler = netsnmp_container_table_handler_get(table_info, container,
                                                  TABLE_CONTAINER_KEY_NETSNMP_INDEX);
    if (NULL == handler) {
        snmp_log(LOG_ERR,
                 "error allocating table registration for cpqSasHbaTable\n");
        goto bail;
    }
    if (SNMPERR_SUCCESS != netsnmp_inject_handler(reg, handler)) {
        snmp_log(LOG_ERR,
                 "error injecting container_table handler for cpqSasHbaTable\n");
        goto bail;
    }
    handler = NULL;             /* reg has it, will reuse below */

    /*************************************************
     *
     * inject cache helper
     */
    cache = netsnmp_cache_create(300,    /* timeout in seconds */
                                 _cache_load, _cache_free,
                                 cpqSasHbaTable_oid,
                                 cpqSasHbaTable_oid_len);

    if (NULL == cache) {
        snmp_log(LOG_ERR, "error creating cache for cpqSasHbaTable\n");
        goto bail;
    }
    cache->flags = NETSNMP_CACHE_PRELOAD |
                   NETSNMP_CACHE_DONT_FREE_BEFORE_LOAD |
                   NETSNMP_CACHE_DONT_FREE_EXPIRED |
                   NETSNMP_CACHE_DONT_AUTO_RELEASE |
                   NETSNMP_CACHE_DONT_INVALIDATE_ON_SET;

    cache->magic = container;

    handler = netsnmp_cache_handler_get(cache);
    if (NULL == handler) {
        snmp_log(LOG_ERR,
                 "error creating cache handler for cpqSasHbaTable\n");
        goto bail;
    }

    if (SNMPERR_SUCCESS != netsnmp_inject_handler(reg, handler)) {
        snmp_log(LOG_ERR,
                 "error injecting cache handler for cpqSasHbaTable\n");
        goto bail;
    }
    handler = NULL;             /* reg has it */

    /*
     * register the table
     */
    reg_tbl_ret = netsnmp_register_table(reg, table_info);
    if (reg_tbl_ret != SNMPERR_SUCCESS) {
        snmp_log(LOG_ERR,
                 "error registering table handler for cpqSasHbaTable\n");
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
cpqSasHbaTable_entry *
cpqSasHbaTable_createEntry(netsnmp_container * container,
                           oid cpqSasHbaIndex)
{
    cpqSasHbaTable_entry *entry;

    entry = SNMP_MALLOC_TYPEDEF(cpqSasHbaTable_entry);
    if (!entry)
        return NULL;

    entry->cpqSasHbaIndex = cpqSasHbaIndex;
    entry->oid_index.len = 1;
    entry->oid_index.oids = (oid *) &entry->cpqSasHbaIndex;
    return entry;
}

/** remove a row from the table */
void
cpqSasHbaTable_removeEntry(netsnmp_container * container,
                           cpqSasHbaTable_entry * entry)
{

    if (!entry)
        return;                 /* Nothing to remove */
    CONTAINER_REMOVE(container, entry);
    if (entry)
        SNMP_FREE(entry);       /* XXX - release any other internal resources */
}

/** remove a row from the table */
void
cpqSasHbaTable_freeEntry(cpqSasHbaTable_entry * entry)
{

    if (!entry)
        return;                 /* Nothing to remove */
    SNMP_FREE(entry);           /* XXX - release any other internal resources */
}

/** handles requests for the cpqSasHbaTable table */
int
cpqSasHbaTable_handler(netsnmp_mib_handler *handler,
                       netsnmp_handler_registration *reginfo,
                       netsnmp_agent_request_info *reqinfo,
                       netsnmp_request_info *requests)
{

    netsnmp_request_info *request;
    netsnmp_table_request_info *table_info;
    cpqSasHbaTable_entry *table_entry;

    DEBUGMSGTL(("cpqSasHbaTable:handler", "Processing request (%d)\n",
                reqinfo->mode));

    switch (reqinfo->mode) {
        /*
         * Read-support (also covers GetNext requests)
         */
    case MODE_GET:
        for (request = requests; request; request = request->next) {
            if (request->processed)
                continue;
            table_entry = (cpqSasHbaTable_entry *)
                netsnmp_container_table_extract_context(request);
            table_info = netsnmp_extract_table_info(request);
            if ((NULL == table_entry) || (NULL == table_info)) {
                snmp_log(LOG_ERR,
                         "No table entry or info for cpqSasHbaTable\n");
                snmp_set_var_typed_value(request->requestvb,
                                         SNMP_ERR_GENERR, NULL, 0);
                continue;
            }

            switch (table_info->colnum) {
            case COLUMN_CPQSASHBAINDEX:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_INTEGER,
                                           table_entry->cpqSasHbaIndex);
                break;
            case COLUMN_CPQSASHBAHWLOCATION:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
                                         table_entry->cpqSasHbaHwLocation,
                                         table_entry->
                                         cpqSasHbaHwLocation_len);
                break;
            case COLUMN_CPQSASHBAMODEL:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_INTEGER,
                                           table_entry->cpqSasHbaModel);
                break;
            case COLUMN_CPQSASHBASTATUS:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_INTEGER,
                                           table_entry->cpqSasHbaStatus);
                break;
            case COLUMN_CPQSASHBACONDITION:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_INTEGER,
                                           table_entry->
                                           cpqSasHbaCondition);
                break;
            case COLUMN_CPQSASHBAOVERALLCONDITION:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_INTEGER,
                                           table_entry->
                                           cpqSasHbaOverallCondition);
                break;
            case COLUMN_CPQSASHBASERIALNUMBER:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
                                         table_entry->
                                         cpqSasHbaSerialNumber,
                                         table_entry->
                                         cpqSasHbaSerialNumber_len);
                break;
            case COLUMN_CPQSASHBAFWVERSION:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
                                         table_entry->cpqSasHbaFwVersion,
                                         table_entry->
                                         cpqSasHbaFwVersion_len);
                break;
            case COLUMN_CPQSASHBABIOSVERSION:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
                                         table_entry->cpqSasHbaBiosVersion,
                                         table_entry->
                                         cpqSasHbaBiosVersion_len);
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

/**
 * @internal
 */
static int
_cache_load(netsnmp_cache * cache, void *vmagic)
{
    netsnmp_container *container;

    DEBUGMSGTL(("internal:cpqSasHbaTable:_cache_load", "called\n"));

    if ((NULL == cache) || (NULL == cache->magic)) {
        snmp_log(LOG_ERR, "invalid cache for cpqSasHbaTable_cache_load\n");
        return -1;
    }
    container = (netsnmp_container *) cache->magic;

    /** should only be called for an invalid or expired cache */
    netsnmp_assert((0 == cache->valid) || (1 == cache->expired));

    /*
     * load cache here (or call function to do it)
     */
    netsnmp_arch_sashba_container_load(container);


    return 0;
}                               /* _cache_load */

/**
 * @Internal
 */
/** remove a row from the table */
static void
cpqSasHbaTable_freeEntry_cb(cpqSasHbaTable_entry * entry, void *magic)
{

    cpqSasHbaTable_freeEntry(entry);
}

/**
 * @internal
 */
static void
_cache_free(netsnmp_cache * cache, void *magic)
{
    netsnmp_container *container;

    DEBUGMSGTL(("internal:cpqSasHbaTable:_cache_free", "called\n"));

    if ((NULL == cache) || (NULL == cache->magic)) {
        snmp_log(LOG_ERR, "invalid cache in cpqSasHbaTable_cache_free\n");
        return;
    }
    container = (netsnmp_container *) cache->magic;

    /*
     * empty (but don't free) cache here
     */
    CONTAINER_CLEAR(container,
                    (netsnmp_container_obj_func *)
                    cpqSasHbaTable_freeEntry_cb, NULL);
}                               /* _cache_free */

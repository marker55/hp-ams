#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <net-snmp/agent/hardware/memory.h>

/*
 * cpqHostOs_variables_oid:
 * this is the top level oid that we want to register under.  This
 * is essentially a prefix, with the suffix appearing in the
 * variable below.
 */

#define CPQHO_PHYSICALMEMORYSIZE        1
#define CPQHO_PHYSICALMEMORYFREE        2
#define CPQHO_PAGINGMEMORYSIZE          3
#define CPQHO_PAGINGMEMORYFREE          4

void init_hw_mem(void);

/*
 * variable4 cpqHostOs_variables:
 *   this variable defines function callbacks and type return information
 *   for the cpqHostOs mib section
 */
Netsnmp_Node_Handler handle_cpqHostOsMem;
/** Initializes the cpqHostOsMem module */
void
init_cpqHostOsMem(void)
{
    const oid      cpqHostOsMem_oid[] = { 1, 3, 6, 1, 4, 1, 232, 11, 2, 13 };

    DEBUGMSGTL(("cpqHostOsMem", "Initializing\n"));

    init_hw_mem();

    netsnmp_register_scalar_group(
        netsnmp_create_handler_registration("cpqHostOsMem", handle_cpqHostOsMem,
                                 cpqHostOsMem_oid, OID_LENGTH(cpqHostOsMem_oid),
                                             HANDLER_CAN_RONLY),
                                 1, 4);

}

int
handle_cpqHostOsMem(netsnmp_mib_handler *handler,
                netsnmp_handler_registration *reginfo,
                netsnmp_agent_request_info *reqinfo,
                netsnmp_request_info *requests)
{
    netsnmp_memory_info *mem_info;
    int val;

    /*
     * We just need to handle valid GET requests, as invalid instances
     *   are rejected automatically, and (valid) GETNEXT requests are
     *   converted into the appropriate GET request.
     *
     * We also only ever receive one request at a time.
     */
    switch (reqinfo->mode) {
    case MODE_GET:
        netsnmp_memory_load();
        switch (requests->requestvb->name[ reginfo->rootoid_len - 2 ]) {
        case CPQHO_PAGINGMEMORYSIZE:
            mem_info = netsnmp_memory_get_byIdx( NETSNMP_MEM_TYPE_SWAP, 0 );
            if (!mem_info)
               goto NOSUCH;
            val  =  mem_info->size;     /* swaptotal */
            val /= ((1024*1024)/mem_info->units);
            break;
        case CPQHO_PAGINGMEMORYFREE:
            mem_info = netsnmp_memory_get_byIdx( NETSNMP_MEM_TYPE_SWAP, 0 );
            if (!mem_info)
               goto NOSUCH;
            val  =  mem_info->free;     /* swapfree */
            val /= ((1024*1024)/mem_info->units);
            break;
        case CPQHO_PHYSICALMEMORYSIZE:
            mem_info = netsnmp_memory_get_byIdx( NETSNMP_MEM_TYPE_PHYSMEM, 0 );
            if (!mem_info)
               goto NOSUCH;
            val  =  mem_info->size;     /* memtotal */
            val /=  ((1024*1024)/mem_info->units);
            break;
        case CPQHO_PHYSICALMEMORYFREE:
            mem_info = netsnmp_memory_get_byIdx( NETSNMP_MEM_TYPE_PHYSMEM, 0 );
            if (!mem_info)
               goto NOSUCH;
            val  =  mem_info->free;     /* memfree */
            val /=  ((1024*1024)/mem_info->units);
            break;
        default:
            snmp_log(LOG_ERR, "unknown object (%lu) in handle_memory\n",
                     requests->requestvb->name[ reginfo->rootoid_len - 2 ]);
NOSUCH:
            netsnmp_set_request_error( reqinfo, requests, SNMP_NOSUCHOBJECT );
            return SNMP_ERR_NOERROR;
        }
        /* All non-integer objects (and errors) have already been
         * processed.  So return the integer value.
         */
        snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER,
                                 (u_char *)&val, sizeof(val));
        break;

    default:
        /*
         * we should never get here, so this is a really bad error
         */
        snmp_log(LOG_ERR, "unknown mode (%d) in handle_memory\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}


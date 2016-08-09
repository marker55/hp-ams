/*
 * Note: this file originally auto-generated by mib2c using
 *        $
 */

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <sys/time.h>
#include <sys/types.h>
#include "cpqLinOsMgmt.h"
#include "hpHelper.h"
#include "cpqLinOsDiskTable.h"
#include "cpqLinOsNetworkInterfaceTable.h"
#include "cpqLinOsProcessorTable.h"


oid             cpqLinOsMib_oid[] = { 1, 3, 6, 1, 4, 1, 232, 23, 1 };
size_t          cpqLinOsMib_oid_len = OID_LENGTH(cpqLinOsMib_oid);

oid             cpqLinOsCommon_oid[] = { 1, 3, 6, 1, 4, 1, 232, 23, 2, 1, 4 };
size_t          cpqLinOsCommon_oid_len = OID_LENGTH(cpqLinOsCommon_oid);

oid             cpqLinOsSys_oid[] = { 1, 3, 6, 1, 4, 1, 232, 23, 2, 2 };
size_t          cpqLinOsSys_oid_len = OID_LENGTH(cpqLinOsSys_oid);

oid             cpqLinOsMem_oid[] = { 1, 3, 6, 1, 4, 1, 232, 23, 2, 4 };
size_t          cpqLinOsMem_oid_len = OID_LENGTH(cpqLinOsMem_oid);

oid             cpqLinOsSwap_oid[] = { 1, 3, 6, 1, 4, 1, 232, 23, 2, 6 };
size_t          cpqLinOsSwap_oid_len = OID_LENGTH(cpqLinOsSwap_oid);


#define CPQLINOSMIBREVMAJOR               1
#define CPQLINOSMIBREVMINOR               2
#define CPQLINOSMIBCONDITION              3

#define CPQLINOSCOMMONPOLLFREQ               1
#define CPQLINOSCOMMONOBSERVEDPOLLCYCLE      2
#define CPQLINOSCOMMONOBSERVEDTIMESEC        3
#define CPQLINOSCOMMONOBSERVEDTIMEMSEC       4

#define CPQLINOSSYSTEMUPTIME               2
#define CPQLINOSSYSCONTEXTSWITCHPERSEC     4
#define CPQLINOSSYSPROCESSES               6

#define CPQLINOSMEMTOTAL          2
#define CPQLINOSMEMFREE           3
#define CPQLINOSMEMHIGHTOTAL      4
#define CPQLINOSMEMHIGHFREE       5
#define CPQLINOSMEMLOWTOTAL       6
#define CPQLINOSMEMLOWFREE        7
#define CPQLINOSMEMSWAPTOTAL      8
#define CPQLINOSMEMSWAPFREE       9
#define CPQLINOSMEMCACHED        10
#define CPQLINOSMEMSWAPCACHED    11
#define CPQLINOSMEMACTIVE        12
#define CPQLINOSMEMINACTIVEDIRTY 13
#define CPQLINOSMEMINACTIVECLEAN 14

#define CPQLINOSSWAPINPERSEC       2
#define CPQLINOSSWAPOUTPERSEC      3
#define CPQLINOSPAGESWAPINPERSEC   4
#define CPQLINOSPAGESWAPOUTPERSEC  5
#define CPQLINOSMINFLTPERSEC       6
#define CPQLINOSMAJFLTPERSEC       7

static struct timeval the_time[2];
struct timeval *curr_time = &the_time[0];
struct timeval *prev_time = &the_time[1];

struct cpqLinOsMib    mib;
struct cpqLinOsCommon common;
struct cpqLinOsSys    sys;
struct cpqLinOsMem    mem;
struct cpqLinOsSwap   swap;

extern int cpqLinOsCommon_load(netsnmp_cache * cache, void *vmagic);
extern void cpqLinOsCommon_free(netsnmp_cache * cache, void *vmagic);
extern int cpqLinOsSys_load(netsnmp_cache * cache, void *vmagic);
extern void cpqLinOsSys_free(netsnmp_cache * cache, void *vmagic);
extern int cpqLinOsMem_load(netsnmp_cache * cache, void *vmagic);
extern void cpqLinOsMem_free(netsnmp_cache * cache, void *vmagic);
extern int cpqLinOsSwap_load(netsnmp_cache * cache, void *vmagic);
extern void cpqLinOsSwap_free(netsnmp_cache * cache, void *vmagic);

int cpqPerfPollInt = 30;

extern void     init_cpqLinOsCommon();
extern void     init_cpqLinOsSys();
extern void     init_cpqLinOsMem();
extern void     init_cpqLinOsSwap();
extern void     init_cpqLinOsMgmt();

/** Initializes the cpqLinOsMgmt module */
void init_cpqLinOsMib(void)
{

    netsnmp_register_scalar_group(
        netsnmp_create_handler_registration("cpqLinOsMib", 
                                            cpqLinOsMib_handler,
                                            cpqLinOsMib_oid, 
                                            cpqLinOsMib_oid_len,
                                            HANDLER_CAN_RONLY),
        CPQLINOSMIBREVMAJOR, CPQLINOSMIBCONDITION);

    return;
}

void
init_cpqLinOsCommon(void)
{
    netsnmp_mib_handler *handler = NULL;
    netsnmp_cache  *cache = NULL;
    netsnmp_handler_registration *reg;

    int reg_grp_ret = SNMPERR_SUCCESS;

    reg = netsnmp_create_handler_registration("cpqLinOsCommon", 
                                              cpqLinOsCommon_handler,
                                              cpqLinOsCommon_oid, 
                                              cpqLinOsCommon_oid_len,
                                              HANDLER_CAN_RONLY);
    reg_grp_ret = netsnmp_register_scalar_group(reg, 
                                    CPQLINOSCOMMONPOLLFREQ, 
                                    CPQLINOSCOMMONOBSERVEDTIMEMSEC);

    cache = netsnmp_cache_create(5,    /* timeout in seconds */
                                   cpqLinOsCommon_load, cpqLinOsCommon_free,
                                   cpqLinOsCommon_oid, cpqLinOsCommon_oid_len);

    if (NULL == cache) {
        snmp_log(LOG_ERR, "error creating cache for cpqLinOsCommon\n");
        goto bail;
    }
    cache->flags = NETSNMP_CACHE_PRELOAD |
                   NETSNMP_CACHE_DONT_FREE_EXPIRED |
                   NETSNMP_CACHE_DONT_AUTO_RELEASE |
                   NETSNMP_CACHE_DONT_FREE_BEFORE_LOAD | 
                   NETSNMP_CACHE_DONT_INVALIDATE_ON_SET;

    handler = netsnmp_cache_handler_get(cache);
    if (NULL == handler) {
        snmp_log(LOG_ERR,
                 "error creating cache handler for cpqLinOsCommon\n");
        goto bail;
    }

    if (SNMPERR_SUCCESS != netsnmp_inject_handler(reg, handler)) {
        snmp_log(LOG_ERR,
                 "error injecting cache handler for cpqLinOsCommon\n");
        goto bail;
    }
    handler = NULL;             /* reg has it */
    return;
  bail:                        /* not ok */

    if (handler)
        netsnmp_handler_free(handler);

    if (cache)
        netsnmp_cache_free(cache);

    if (reg_grp_ret == SNMPERR_SUCCESS)
        if (reg)
            netsnmp_handler_registration_free(reg);
    return;

}
void
init_cpqLinOsSys(void)
{
    netsnmp_mib_handler *handler = NULL;
    netsnmp_cache  *cache = NULL;
    netsnmp_handler_registration *reg;

    int reg_grp_ret = SNMPERR_SUCCESS;


    reg = netsnmp_create_handler_registration("cpqLinOsSys", 
                                            cpqLinOsSys_handler,
                                            cpqLinOsSys_oid, 
                                            cpqLinOsSys_oid_len,
                                            HANDLER_CAN_RONLY);
    reg_grp_ret = netsnmp_register_scalar_group(reg,
                                                CPQLINOSSYSTEMUPTIME, 
                                                CPQLINOSSYSPROCESSES);

    cache = netsnmp_cache_create(cpqPerfPollInt,    /* timeout in seconds */
                                   cpqLinOsSys_load, cpqLinOsSys_free,
                                   cpqLinOsSys_oid, cpqLinOsSys_oid_len);
    if (NULL == cache) {
        snmp_log(LOG_ERR, "error creating cache for cpqLinOsSys\n");
        goto bail;
    }
    cache->flags = NETSNMP_CACHE_PRELOAD |
                   NETSNMP_CACHE_DONT_FREE_EXPIRED |
                   NETSNMP_CACHE_DONT_AUTO_RELEASE |
                   NETSNMP_CACHE_DONT_FREE_BEFORE_LOAD |
                   NETSNMP_CACHE_DONT_INVALIDATE_ON_SET;

    handler = netsnmp_cache_handler_get(cache);
    if (NULL == handler) {
        snmp_log(LOG_ERR,
                 "error creating cache handler for cpqLinOsSys\n");
        goto bail;
    }

    if (SNMPERR_SUCCESS != netsnmp_inject_handler(reg, handler)) {
        snmp_log(LOG_ERR,
                 "error injecting cache handler for cpqLinOsSys\n");
        goto bail;
    }
    handler = NULL;             /* reg has it */
    return;
  bail:                        /* not ok */
    if (handler)
        netsnmp_handler_free(handler);

    if (cache)
        netsnmp_cache_free(cache);

    if (reg_grp_ret == SNMPERR_SUCCESS)
        if (reg)
            netsnmp_handler_registration_free(reg);
    return;
}

void
init_cpqLinOsMem(void)
{
    netsnmp_mib_handler *handler = NULL;
    netsnmp_cache  *cache = NULL;
    netsnmp_handler_registration *reg;
    int rc;

    reg = 
        netsnmp_create_handler_registration("cpqLinOsMem", 
                             cpqLinOsMem_handler,
                             cpqLinOsMem_oid, cpqLinOsMem_oid_len,
                             HANDLER_CAN_RONLY);
    rc = netsnmp_register_scalar_group( reg,
                CPQLINOSMEMTOTAL, CPQLINOSMEMINACTIVECLEAN);
    if (rc != SNMPERR_SUCCESS)
        return;

    cache = netsnmp_cache_create(cpqPerfPollInt,    /* timeout in seconds */
                                   cpqLinOsMem_load, cpqLinOsMem_free,
                                   cpqLinOsMem_oid, cpqLinOsMem_oid_len);
    if (NULL == cache) {
        snmp_log(LOG_ERR, "error creating cache for cpqLinOsMem\n");
        goto bail;
    }
    cache->flags = NETSNMP_CACHE_PRELOAD |
                   NETSNMP_CACHE_DONT_FREE_EXPIRED |
                   NETSNMP_CACHE_DONT_AUTO_RELEASE |
                   NETSNMP_CACHE_DONT_FREE_BEFORE_LOAD |
                   NETSNMP_CACHE_DONT_INVALIDATE_ON_SET;

    handler = netsnmp_cache_handler_get(cache);
    if (NULL == handler) {
        snmp_log(LOG_ERR,
                 "error creating cache handler for cpqLinOsMem\n");
        goto bail;
    }

    if (SNMPERR_SUCCESS != netsnmp_inject_handler(reg, handler)) {
        snmp_log(LOG_ERR,
                 "error injecting cache handler for cpqLinOsMem\n");
        goto bail;
    }
    handler = NULL;             /* reg has it */
    return;
  bail:                        /* not ok */
    if (handler)
        netsnmp_handler_free(handler);

    if (cache)
        netsnmp_cache_free(cache);

    if (reg)
        netsnmp_handler_registration_free(reg);
    return;
}

void
init_cpqLinOsSwap(void)
{
    netsnmp_mib_handler *handler = NULL;
    netsnmp_cache  *cache = NULL;
    netsnmp_handler_registration *reg;
    int rc;

    reg = 
        netsnmp_create_handler_registration("cpqLinOsSwap", 
                             cpqLinOsSwap_handler,
                             cpqLinOsSwap_oid, cpqLinOsSwap_oid_len,
                             HANDLER_CAN_RONLY);
    rc = netsnmp_register_scalar_group( reg,
                CPQLINOSSWAPINPERSEC, CPQLINOSMAJFLTPERSEC);
    if (rc != SNMPERR_SUCCESS)
        return;

    cache = netsnmp_cache_create(cpqPerfPollInt,    /* timeout in seconds */
                                   cpqLinOsSwap_load, cpqLinOsSwap_free,
                                   cpqLinOsSwap_oid, cpqLinOsSwap_oid_len);
    if (NULL == cache) {
        snmp_log(LOG_ERR, "error creating cache for cpqLinOsCommon\n");
        goto bail;
    }
    cache->flags = NETSNMP_CACHE_PRELOAD |
                   NETSNMP_CACHE_DONT_FREE_EXPIRED |
                   NETSNMP_CACHE_DONT_AUTO_RELEASE |
                   NETSNMP_CACHE_DONT_FREE_BEFORE_LOAD |
                   NETSNMP_CACHE_DONT_INVALIDATE_ON_SET;

    handler = netsnmp_cache_handler_get(cache);
    if (NULL == handler) {
        snmp_log(LOG_ERR,
                 "error creating cache handler for cpqLinOsCommon\n");
        goto bail;
    }

    if (SNMPERR_SUCCESS != netsnmp_inject_handler(reg, handler)) {
        snmp_log(LOG_ERR,
                 "error injecting cache handler for cpqLinOsCommon\n");
        goto bail;
    }
    handler = NULL;             /* reg has it */
    return;
  bail:                        /* not ok */

    if (handler)
        netsnmp_handler_free(handler);

    if (cache)
        netsnmp_cache_free(cache);

    if (reg)
        netsnmp_handler_registration_free(reg);
    return;

}

void
init_cpqLinOsMgmt(void)
{

    DEBUGMSGTL(("cpqLinOsMgmt", "Initializing\n"));

    //cpqHostMibStatusArray[CPQMIB].major = CPQMIBREVMAJOR;
    //cpqHostMibStatusArray[CPQMIB].minor = CPQMIBREVMINOR;
    //cpqHostMibStatusArray[CPQMIB].cond = MIB_CONDITION_OK;

    init_cpqLinOsMib();
    init_cpqLinOsCommon();
    init_cpqLinOsSys();
    init_cpqLinOsMem();
    init_cpqLinOsSwap();
    initialize_table_cpqLinOsDiskTable();
    initialize_table_cpqLinOsNetworkInterfaceTable();
    initialize_table_cpqLinOsProcessorTable();

    //cpqHoMibHealthStatusArray[CPQMIBHEALTHINDEX] = MIB_CONDITION_OK;
    //cpqHostMibStatusArray[CPQMIB].stat = MIB_STATUS_AVAILABLE;
}
int
cpqLinOsMib_handler(netsnmp_mib_handler          *handler,
                    netsnmp_handler_registration *reginfo,
                    netsnmp_agent_request_info   *reqinfo,
                    netsnmp_request_info         *requests)
{
    long val = 0;

    switch (reqinfo->mode) {
    case MODE_GET:
        switch (requests->requestvb->name[ reginfo->rootoid_len - 2 ]) {
            case CPQLINOSMIBREVMAJOR:
                val = CPQMIBREVMAJOR;
                break;
            case CPQLINOSMIBREVMINOR:
                val = CPQMIBREVMINOR;
                break;
            case CPQLINOSMIBCONDITION:
                val = MIB_CONDITION_OK;
                break;
            default:
                /*
                 * An unsupported/unreadable column (if applicable)
                 */
                snmp_log(LOG_ERR, "unknown object (%lu) in cpqLinOs_handler\n",
                     requests->requestvb->name[ reginfo->rootoid_len - 2 ]);
                netsnmp_set_request_error( reqinfo, requests,
                                            SNMP_NOSUCHOBJECT );
                return SNMP_ERR_NOERROR;
        }
        snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER,
                                 (u_char *)&val, sizeof(val));
        break;

    default:
        snmp_log(LOG_ERR, "unknown mode (%d) in cpqLinOs_handlery\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;

}

int
cpqLinOsCommon_handler(netsnmp_mib_handler          *handler,
               netsnmp_handler_registration *reginfo,
               netsnmp_agent_request_info   *reqinfo,
               netsnmp_request_info         *requests)
{
    long val = 0;

    switch (reqinfo->mode) {
    case MODE_GET:
        switch (requests->requestvb->name[ reginfo->rootoid_len - 2 ]) {
            case CPQLINOSCOMMONPOLLFREQ:
                val = cpqPerfPollInt;
                break;
            case CPQLINOSCOMMONOBSERVEDPOLLCYCLE:
		        val = common.LastObservedPollCycle;
		        break;
            case CPQLINOSCOMMONOBSERVEDTIMESEC:
		        val = common.LastObservedTimeSec;
                break;
            case CPQLINOSCOMMONOBSERVEDTIMEMSEC:
                val = common.LastObservedTimeMSec;
                break;
            default:
                /*
                 * An unsupported/unreadable column (if applicable)
                 */
                snmp_log(LOG_ERR, "unknown object (%lu) in cpqLinOsCommon_handler\n",
                     requests->requestvb->name[ reginfo->rootoid_len - 2 ]);
                netsnmp_set_request_error( reqinfo, requests,
                                            SNMP_NOSUCHOBJECT );
                return SNMP_ERR_NOERROR;
        }
        snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER,
                                 (u_char *)&val, sizeof(val));
        break;

    default:
        snmp_log(LOG_ERR, "unknown mode (%d) in cpqLinOs_handlery\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int
cpqLinOsSys_handler(netsnmp_mib_handler          *handler,
               netsnmp_handler_registration *reginfo,
               netsnmp_agent_request_info   *reqinfo,
               netsnmp_request_info         *requests)
{
    long val = 0;

    switch (reqinfo->mode) {
    case MODE_GET:
        switch (requests->requestvb->name[ reginfo->rootoid_len - 2 ]) {
            case CPQLINOSSYSTEMUPTIME:
                snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                   (u_char *) sys.SystemUpTime,
                                    sys.SystemUpTime_len);
                break;
            case CPQLINOSSYSCONTEXTSWITCHPERSEC:
                val = sys.ContextSwitchesPersec;
                snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER,
                                 (u_char *)&val, sizeof(val));
                break;
            case CPQLINOSSYSPROCESSES:
                val = sys.Processes;
                snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER,
                                 (u_char *)&val, sizeof(val));
                break;
            default:
                /*
                 * An unsupported/unreadable column (if applicable)
                 */
                netsnmp_set_request_error( reqinfo, requests,
                                            SNMP_NOSUCHOBJECT );
                return SNMP_ERR_NOERROR;
        }
        break;

    default:
        snmp_log(LOG_ERR, "unknown mode (%d) in cpqLinOs_handlery\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int
cpqLinOsMem_handler(netsnmp_mib_handler          *handler,
               netsnmp_handler_registration *reginfo,
               netsnmp_agent_request_info   *reqinfo,
               netsnmp_request_info         *requests)
{
    long val = 0;

    switch (reqinfo->mode) {
    case MODE_GET:
        switch (requests->requestvb->name[ reginfo->rootoid_len - 2 ]) {
            case CPQLINOSMEMTOTAL:
                val = mem.Total;
                break;
            case CPQLINOSMEMFREE:
                val = mem.Free;
                break;
            case CPQLINOSMEMHIGHTOTAL:
                val = mem.HighTotal;
                break;
            case CPQLINOSMEMHIGHFREE:
                val = mem.HighFree;
                break;
            case CPQLINOSMEMLOWTOTAL:
                val = mem.LowTotal;
                break;
            case CPQLINOSMEMLOWFREE:
                val = mem.LowFree;
                break;
            case CPQLINOSMEMSWAPTOTAL:
                val = mem.SwapTotal;
                break;
            case CPQLINOSMEMSWAPFREE:
                val = mem.SwapFree;
                break;
            case CPQLINOSMEMCACHED:
                val = mem.Cached;
                break;
            case CPQLINOSMEMSWAPCACHED:
                val = mem.SwapCached;
                break;
            case CPQLINOSMEMACTIVE:
                val = mem.Active;
                break;
            case CPQLINOSMEMINACTIVEDIRTY:
                val = mem.InactiveDirty;
                break;
            case CPQLINOSMEMINACTIVECLEAN:
                val = mem.InactiveClean;
                break;
            default:
                /*
                 * An unsupported/unreadable column (if applicable)
                 */
                snmp_log(LOG_ERR, "unknown object (%lu) in cpqLinOsMem_handler\n",
                     requests->requestvb->name[ reginfo->rootoid_len - 2 ]);
                netsnmp_set_request_error( reqinfo, requests,
                                            SNMP_NOSUCHOBJECT );
                return SNMP_ERR_NOERROR;
        }
        snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER,
                                 (u_char *)&val, sizeof(val));
        break;

    default:
        snmp_log(LOG_ERR, "unknown mode (%d) in cpqLinOs_handlery\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;

}

int
cpqLinOsSwap_handler(netsnmp_mib_handler          *handler,
               netsnmp_handler_registration *reginfo,
               netsnmp_agent_request_info   *reqinfo,
               netsnmp_request_info         *requests)
{
    long val = 0;

    switch (reqinfo->mode) {
    case MODE_GET:
        switch (requests->requestvb->name[ reginfo->rootoid_len - 2 ]) {
            case CPQLINOSSWAPINPERSEC:
                val = swap.SwapInPerSec;
                break;
            case CPQLINOSSWAPOUTPERSEC:
                val = swap.SwapOutPerSec;
                break;
            case CPQLINOSPAGESWAPINPERSEC:
                val = swap.PageSwapInPerSec;
                break;
            case CPQLINOSPAGESWAPOUTPERSEC:
                val = swap.PageSwapOutPerSec;
                break;
            case CPQLINOSMINFLTPERSEC:
                val = swap.MinFltPerSec;
                break;
            case CPQLINOSMAJFLTPERSEC:
                val = swap.MajFltPerSec;
                break;
            default:
                /*
                 * An unsupported/unreadable column (if applicable)
                 */
                snmp_log(LOG_ERR, "unknown object (%lu) in cpqLinOs_handler\n",
                     requests->requestvb->name[ reginfo->rootoid_len - 2 ]);
                netsnmp_set_request_error( reqinfo, requests,
                                            SNMP_NOSUCHOBJECT );
                return SNMP_ERR_NOERROR;
        }
        snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER,
                                 (u_char *)&val, sizeof(val));
        break;

    default:
        snmp_log(LOG_ERR, "unknown mode (%d) in cpqLinOs_handlery\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

/*
 * Note: this file originally auto-generated by mib2c using
 *       version : 15795 $ of $ 
 */
/*
 * standard Net-SNMP includes 
 */
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/types.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <net-snmp/library/large_fd_set.h>
#include <net-snmp/agent/mib_modules.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include "hpHelper.h"

#ifndef NETSNMP_NO_SYSTEMD
#include <net-snmp/library/sd-daemon.h>
#endif

/*
 * include our parent header 
 */
#include "cpqNic/cpqNic.h"
#include "common/smbios.h"

#include <signal.h>
#include <syslog.h>

/* Forward declaration for mibs ti init   */
extern void init_cpqStdPciTable(void);
extern void init_cpqHost(void);
extern void init_cpqScsi(void);
extern void init_cpqIde(void);
extern void init_cpqFibreArray(void);
extern void init_cpqLinOsMgmt(void);

/* Forward declaration for BB log functions   */
extern void LOG_OS_INFORMATION(void );
extern void LOG_OS_BOOT(void );
extern void LOG_OS_SHUTDOWN(void );
extern void LOG_DRIVER(void );
extern void LOG_SERVICE(void );
extern void LOG_PACKAGE(void );
extern void LOG_OS_USAGE(void );
extern void LOG_MEMORY_USAGE(void );
extern void LOG_PROCESSOR_USAGE(void );
extern void LOG_PROCESS_CRASHES(void );

extern int InitSMBIOS(void);
extern int SmbGetSysGen(void);
static int      receive(void);

MibStatusEntry  cpqHostMibStatusArray[20];
unsigned char cpqHoMibHealthStatusArray[CPQHO_MIBHEALTHSTATUSARRAY_LEN];

static int      ams_running;
static int      interval2ping = 15;
int             log_interval   = 0;
int             testtrap_interval = 0;
int             gen8_only     = 1;

char *GenericData = "generic trap data";
struct pci_access *pacc = NULL;

static          RETSIGTYPE
stop_server(int a)
{
    ams_running = 0;
}

static void
usage(void)
{
    printf
        ("usage: hpHelper [-f] [-L] [-D<tokens>] [-I<interval>] [-P<interval>]\n"
         "\t\t[-M<MIB list>] [-T<interval>] [-G<generic data>]\n"
         "\t-f      Do not fork() from the calling shell.\n"
         "\t-L\tDo not open a log file; print all messages to stderr.\n"
         "\t-DTOKEN[,TOKEN,...]\n"
         "\t\tTurn on debugging output for the given TOKEN(s).\n"
         "\t\tWithout any tokens specified, it defaults to printing\n"
         "\t\tall the tokens (which is equivalent to the keyword 'ALL').\n"
         "\t\tYou might want to try ALL for extremely verbose output.\n"
         "\t\tNote: You can't put a space between the -D and the TOKENs.\n"
         "\t-I<interval>\n"
         "\t\tThe <interval> parameter indicates how often performance data\n"
         "\t\twill be logged to Black Box:\n"
         "\t\t\t0 = do not log any performance counters (default)\n"
         "\t\t\t1 = log performance counters every minute\n"
         "\t\t\t2 = log performance counters every 10 minutes\n"
         "\t\t\t3 = log performance counters every hour\n"
         "\t-P<interval>\n"
         "\t\tThe <interval> parameter specifies the number of seconds between\n"
         "\t\tRFC 2741 (AgentX) PING requests\n"
         "\t\t\t0 = PING Request packets are not sent\n"
         "\t\t\t15 = PING Request packets are sent every 15 seconds (default)\n"
         "\t-M<MIB list>\n"
         "\t\t<MIB list> = comma seperated list of HP/Compaq MIBS to register\n"
         "\t\tAvailable MIBS are 0,1,5,11,14,16,18,23,99\n"
         "\t\t\t0 = AHS logging, 99 is for MIBII\n"
         "\t\t\t0,1,5,11,14,16,18,99 = list of MIBS supported (default)\n"
         "\t\t\tEmpty <MIB list> the same as -M99 (enable MIBII)\n"
         "\t-T<interval>\n"
         "\t\tThe <interval> parameter indicates how often test trap will be\n"
         "\t\tsent.  If no <interval> is specified, then the trap will be \n"
         "\t\tsent once at start up\n"
         "\t-G<generic data>\n"
         "\t\tThe value for cpqHoGenericData sent with cpqHoGenericTrap\n"
         "\t\t'generic trap data' = default\n"
         "");
    exit(0);
}

int
main(int argc, char **argv)
{
    /*
     * Defs for arg-handling code: handles setting of policy-related variables 
     */
    int             ch;
    int             dont_fork = 0, use_syslog = 0, kdump_only = 0;
    int             sys_gen = 0;
    char            *transport = "hpilo:";
    int             i;

    int            do_ahs = 1;
    int            do_ide = 1;
    int            do_host = 1;
    int            do_nic = 1;
    int            do_scsi = 1;
    int            do_pmp = 0;
    int            do_se = 1;
    int            do_fca = 1;
    int            do_mibII = 1;
    char *argarg;
    int             prepared_sockets = 0;

#ifndef NETSNMP_NO_SYSTEMD
    /* check if systemd has sockets for us and don't close them */
    prepared_sockets = netsnmp_sd_listen_fds(0);
#endif /* NETSNMP_NO_SYSTEMD */

    /*
     * close all non-standard file descriptors we may have
     * inherited from the shell.
     */
    if (!prepared_sockets) {
        for (i = getdtablesize() - 1; i > 2; --i) {
            (void) close(i);
        }
    }

    snmp_disable_log();

    while ((ch = getopt(argc, argv, "D:fG:kLM::OI:P:T::x:")) != EOF)
        switch (ch) {
        case 'D':
            debug_register_tokens(optarg);
            snmp_set_do_debugging(1);
            break;
        case 'f':
            dont_fork = 1;
            break;
        case 'k':
            kdump_only = 1;
            break;
        case 'h':
            usage();
            exit(0);
            break;
        case 'G':
            GenericData = optarg;
            break;
        case 'I':
            log_interval = atoi(optarg);
            if ((log_interval <0) || (log_interval > 3)) {
                usage();
                exit(0);
            }
            break;
        case 'P':
            interval2ping = atoi(optarg);
            if (interval2ping <0)  {
                usage();
                exit(0);
            }
            break;
        case 'L':
            use_syslog = 0;     /* use stderr */
            break;
        case 'M':
            argarg = optarg;
            do_ahs = 0;
            
            do_ide = 0;
            do_nic = 0;
            do_scsi = 0;
            do_host = 0;
            do_pmp = 0;
            do_se = 0;
            do_fca = 0;
            do_mibII = 1;
            while (argarg != 0) {
                char * mibnum;
                int mibitem;

                do_mibII = 0;
                mibnum = argarg;
                argarg = index(mibnum, ',');
                if (argarg != 0) {
                    *argarg = 0;
                    argarg++;
                }
                mibitem = atoi(mibnum);
                switch (mibitem) {
                    case 0:
                        do_ahs = 1;
                        break; 
                    case 1:
                        do_se = 1;
                        break; 
                    case 5:
                        do_scsi = 1;
                        break; 
                    case 11:
                        do_host = 1;
                        break; 
                    case 14:
                        do_ide = 1;
                        break; 
                    case 16:
                        do_fca = 1;
                        break; 
                    case 18:
                        do_nic = 1;
                        break; 
                    case 23:
                        do_pmp = 1;
                        break; 
                    case 99:
                        do_mibII = 1;
                        break; 
                    default:
                        break; 
                }
            }
            break;
        case 'O':
            gen8_only = 0;     /* allow to run on non supported */
            break;
        case 'T':
            if (optarg != (char *) 0) {
                testtrap_interval = atoi(optarg);
                if ((testtrap_interval < 0) || (testtrap_interval > 1440)) {
                    usage();
                    exit(0);
                }
            }
            break;
        case 'x':
            if (optarg != NULL) {
                transport = optarg;
            } else {
                usage();
            }
            break;
        default:
            fprintf(stderr, "unknown option %c\n", ch);
            usage();
            exit(1);
        }

    /* if the -k option was provided, then also set the dont_fork flag. */
    if (kdump_only) {
        dont_fork = 1;
    }

    if (dont_fork) {
        snmp_enable_stderrlog();
    }

    netsnmp_ds_set_string(NETSNMP_DS_APPLICATION_ID,
                                      NETSNMP_DS_AGENT_X_SOCKET, transport);
    /*
     * make us a agentx client. 
     */
    netsnmp_ds_set_boolean(NETSNMP_DS_APPLICATION_ID,
                                    NETSNMP_DS_AGENT_ROLE, 1);

    if (InitSMBIOS() == 0 ) {
        if (gen8_only == 0) {
            fprintf(stderr,"SM BIOS initialization failed,"
                       " some functionality may not exist\n");
        } else {
            fprintf(stderr,"SM BIOS initialization failed,"
                       " unable to determine system type\n");
            exit(1);
        }
    } else {
        sys_gen = SmbGetSysGen();
        if ((gen8_only) && (sys_gen < 8)) {
            fprintf(stderr,"This program will on run on HP ProLiant Gen8"
                       " or newer platforms\n");
            exit(1);
        }
    }

    /*
     * daemonize 
     */
    if (!dont_fork) {
        snmp_enable_calllog();
        int             rc = netsnmp_daemonize(1, !use_syslog);
        if (rc)
            exit(-1);
    }

    /* Clear out Status array */
    memset(cpqHostMibStatusArray,0,sizeof(cpqHostMibStatusArray));
    memset(cpqHoMibHealthStatusArray, 0, sizeof(cpqHoMibHealthStatusArray));

    if (do_ahs) {
    LOG_PROCESS_CRASHES();
    if (kdump_only) {
        return 0;
    }

    LOG_OS_BOOT();
    }
    openlog("hp-ams",  LOG_CONS | LOG_PID, LOG_DAEMON);
    /*
     * initialize the agent library 
     */
    init_agent("hpHelper");

    netsnmp_ds_set_int(NETSNMP_DS_LIBRARY_ID,
                       NETSNMP_DS_HPILODOMAIN_IML, 4);

    netsnmp_ds_set_int(NETSNMP_DS_APPLICATION_ID,
                       NETSNMP_DS_AGENT_AGENTX_TIMEOUT, 120 * ONE_SEC);

    /* set ping inetrval to interval2pimn - default is 0 */
    netsnmp_ds_set_int(NETSNMP_DS_APPLICATION_ID,
                       NETSNMP_DS_AGENT_AGENTX_PING_INTERVAL, interval2ping);

    /* sometime iLO4 can be slow so double the default to 2 sec if necessary*/
    netsnmp_ds_set_int(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_TIMEOUT, 3); 

    /* if the ilo starts dropping packets retry a couple of times */
    netsnmp_ds_set_int(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_RETRIES, 5);

    init_snmp("hpHelper");

    if (do_mibII)
    init_mib_modules();

    if (do_host)
    init_cpqHost();

    if (do_ahs) {
    LOG_OS_INFORMATION();
    LOG_DRIVER();
    LOG_SERVICE();
    LOG_PACKAGE();
    }

    if (do_se)
        init_cpqStdPciTable();

    if (do_scsi)
    init_cpqScsi();

    if (do_ide)
    init_cpqIde();

    if (do_fca)
    init_cpqFibreArray();

    if (do_nic)
    init_cpqNic();

    if (do_pmp)
        init_cpqLinOsMgmt();

    /*
     * Send coldstart trap if possible.
     */
    send_easy_trap(0, 0);
    syslog(LOG_NOTICE, "hpHelper Started . . "); 
    /*
     * Let systemd know we're up.
     */
#ifndef NETSNMP_NO_SYSTEMD
    netsnmp_sd_notify(1, "READY=1\n");
    if (prepared_sockets)
        /*
         * Clear the environment variable, we already processed all the sockets
         * by now.
         */
        netsnmp_sd_listen_fds(1);
#endif

    if (do_ahs) {
    LOG_OS_USAGE();
    LOG_PROCESSOR_USAGE();
    LOG_MEMORY_USAGE();
    }

    /*
     * In case we recevie a request to stop (kill -TERM or kill -INT) 
     */
    ams_running = 1;
    signal(SIGTERM, stop_server);
    signal(SIGINT, stop_server);


    DEBUGMSGTL(("snmpd/main", "We're up.  Starting to process data.\n"));
    if (!netsnmp_ds_get_boolean(NETSNMP_DS_APPLICATION_ID,
                NETSNMP_DS_AGENT_QUIT_IMMEDIATELY))
        receive();

    snmp_shutdown("hpHelper");

    LOG_OS_SHUTDOWN();

    syslog(LOG_NOTICE, "hpHelper Stopped . . "); 
    exit(0);
}
/*******************************************************************-o-******
 * receive
 *
 * Parameters:
 *
 * Returns:
 *  0   On success.
 *  -1  System error.
 *
 * Infinite while-loop which monitors incoming messages for the agent.
 * Invoke the established message handlers for incoming messages on a per
 * port basis.  Handle timeouts.
 */
static int
receive(void)
{
    int             numfds;

    netsnmp_large_fd_set readfds, writefds, exceptfds;
    struct timeval  timeout, *tvp = &timeout;
    int             count, block, i;

    netsnmp_large_fd_set_init(&readfds, FD_SETSIZE);
    netsnmp_large_fd_set_init(&writefds, FD_SETSIZE);
    netsnmp_large_fd_set_init(&exceptfds, FD_SETSIZE);

    /*
     * you're main loop here... 
     */
    while (ams_running) {
	/* following code from snmpd */
        /*
         * if you use select(), see snmp_select_info() in snmp_api(3) 
         */
        /*
         * default to sleeping for a really long time. INT_MAX
         * should be sufficient (eg we don't care if time_t is
         * a long that's bigger than an int).
         */
        tvp = &timeout;
        tvp->tv_sec = INT_MAX;
        tvp->tv_usec = 0;

        numfds = 0;
        NETSNMP_LARGE_FD_ZERO(&readfds);
        NETSNMP_LARGE_FD_ZERO(&writefds);
        NETSNMP_LARGE_FD_ZERO(&exceptfds);
        block = 0;
        snmp_select_info2(&numfds, &readfds, tvp, &block);
        if (block == 1) {
            tvp = NULL;         /* block without timeout */
	}

        netsnmp_external_event_info2(&numfds, &readfds, &writefds, &exceptfds);

    reselect:
        for (i = 0; i < NUM_EXTERNAL_SIGS; i++) {
            if (external_signal_scheduled[i]) {
                external_signal_scheduled[i]--;
                external_signal_handler[i](i);
            }
        }

        DEBUGMSGTL(("snmpd/select", "select( numfds=%d, ..., tvp=%p)\n",
                    numfds, tvp));
        if (tvp)
            DEBUGMSGTL(("timer", "tvp %ld.%ld\n", (long) tvp->tv_sec,
                        (long) tvp->tv_usec));
        count = netsnmp_large_fd_set_select(numfds, &readfds, &writefds, &exceptfds,
				     tvp);
        DEBUGMSGTL(("snmpd/select", "returned, count = %d\n", count));

        if (count > 0) {
            netsnmp_dispatch_external_events2(&count, &readfds,
                                              &writefds, &exceptfds);

            /* If there are still events leftover, process them */
            if (count > 0) {
              snmp_read2(&readfds);
            }
        } else
            switch (count) {
            case 0:
                snmp_timeout();
                break;
            case -1:
                DEBUGMSGTL(("snmpd/select", "  errno = %d\n", errno));
                if (errno == EINTR) {
                    /*
                     * likely that we got a signal. Check our special signal
                     * flags before retrying select.
                     */
		    if (ams_running) {
                        goto reselect;
		    }
                    continue;
                } else {
                    snmp_log_perror("select");
                }
                return -1;
            default:
                snmp_log(LOG_ERR, "select returned %d\n", count);
                return -1;
            }                   /* endif -- count>0 */

        /*
         * run requested alarms 
         */
        run_alarms();

        netsnmp_check_outstanding_agent_requests();
    }
    /* lets give a seconb or 2 to log the shutdown */
    sleep(0);
    netsnmp_large_fd_set_cleanup(&readfds);
    netsnmp_large_fd_set_cleanup(&writefds);
    netsnmp_large_fd_set_cleanup(&exceptfds);

    /*
     * at shutdown time 
     */


    snmp_log(LOG_INFO, "Received TERM or STOP signal...  shutting down...\n");
    return 0;
}                               /* end receive() */



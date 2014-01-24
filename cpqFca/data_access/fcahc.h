/*
 * fchba data access header
 */
#ifndef NETSNMP_ACCESS_FCHBA_CONFIG_H
#define NETSNMP_ACCESS_FCHBA_CONFIG_H


/* FC MIB HBA status values */
#define FC_HBA_STATUS_OTHER          1
#define FC_HBA_STATUS_OK             2
#define FC_HBA_STATUS_FAILED         3
#define FC_HBA_STATUS_SHUTDOWN       4
#define FC_HBA_STATUS_LOOPDEGRADED   5
#define FC_HBA_STATUS_LOOPFAILED     6
#define FC_HBA_STATUS_NOTCONNECTED   7

/* FC MIB HBA condition values */
#define FC_HBA_CONDITION_OTHER          1
#define FC_HBA_CONDITION_OK             2
#define FC_HBA_CONDITION_DEGRADED       3
#define FC_HBA_CONDITION_FAILED         4

#endif /* NETSNMP_ACCESS_FCHBA_CONFIG_H */

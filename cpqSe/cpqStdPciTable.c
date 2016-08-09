#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <net-snmp/agent/table_container.h>
#include "cpqSePciSlotTable.h"
#include "cpqSePciFunctTable.h"
#include "cpqSePCIeDiskTable.h"

void
init_cpqStdPciTable(void)
{
/*
 * here we initialize all the tables we're planning on supporting
 */

    initialize_table_cpqSePciSlotTable();
    initialize_table_cpqSePciFunctTable();
    initialize_cpqSePCIeDisk();
}


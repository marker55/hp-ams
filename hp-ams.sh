#!/bin/bash
#
# (c) Copyright 2011 Hewlett-Packard Development Company, L.P.
#
# See "man chkconfig" for information on next two lines (Red Hat only)
# chkconfig: - 90 1
# description: hp SNMP Agents. 
#
#
# Following lines are in conformance with LSB 1.2 spec
### BEGIN INIT INFO
# Provides:            amsHelper
# Default-Start:       2 3 4 5
# Required-Start:      $network
# Required-Stop:       $network $syslog
# Default-Stop:        0 1 6
# Description:         starts OS helper for HP AMS
# Short-Description:   HP AMS helper
### END INIT INFO

PATH=/sbin:/usr/sbin:/bin:/usr/bin
NAME="HP Agentless Management Service for ProLiant"
SNAME="amsHelper"
HPAMS_OPTIONS="-I0"

if [ -f /etc/sysconfig/hp-ams ]; then
  . /etc/sysconfig/hp-ams
fi
if [ -f "/etc/default/hp-ams" ]; then
  . /etc/default/hp-ams
fi

case "$1" in
   start)
      modprobe -q pci_slot
      if [ ! -d /sys/class/cpuid ]; then
          modprobe cpuid
      fi
      lsmod|grep -q hpilo
      if [ $? -ne 0 ]; then 
         modprobe hpilo
      fi
      if [ "$ALLOW_CORE" = "y" ]; then 
        mkdir -p /var/log/cores/amsHelper
        echo "/var/log/cores/%e/%p-%s-%t.core" > /proc/sys/kernel/core_pattern
        ulimit -c unlimited
      fi

      if [ -f /sys/module/hpilo/parameters/max_ccb ]; then
          let MAXCCBS=`cat /sys/module/hpilo/parameters/max_ccb`-2
      else
          let MAXCCBS=6
      fi
      let ILODEVS=`lsof +d /dev/hpilo|grep hpilo|wc -l`
      if [ $ILODEVS -ge $MAXCCBS ]; then
         echo "Running $SNAME will exhaust all available iLO devices." 
         echo "Please stop hp-health, hp-snmp-agents or other consumers of iLO devices."
         exit 1
      fi

      RC=0;
      killall -0 $SNAME > /dev/null 2>&1; RC=$?
      if [ "$RC" -eq "0" ]; then
         echo "$SNAME is already running.  Please stop $SNAME or use 'restart'"
         exit 1
      fi
      /sbin/$SNAME $HPAMS_OPTIONS; RC=$?

      if [ "$RC" -eq "0" ]; then
         [ -d /var/lock/subsys ] && touch /var/lock/subsys/hp-ams
      fi
      exit $RC
   ;;
   stop)
      RC=0;
      killall $SNAME > /dev/null 2>&1; RC=$?
      if [ "$RC" -eq "1" ]; then
         [ -d /var/lock/subsys ] && rm -rf /var/lock/subsys/hp-ams
         exit 0
      else
        sleep 3
        killall $SNAME > /dev/null 2>&1; RC=$?
        if [ "$RC" -eq "1" ]; then
           [ -d /var/lock/subsys ] && rm -rf /var/lock/subsys/hp-ams
           exit 0
        else
          sleep 3
          killall -9 $SNAME > /dev/null 2>&1
            [ -d /var/lock/subsys ] && rm -rf /var/lock/subsys/hp-ams
          exit 0
        fi

      fi    
   ;;
   reload)
   ;;
   restart)
      $0 stop
      sleep 5
      $0 start
   ;;
   status)
      RC=0;
      killall -0 $SNAME > /dev/null 2>&1; RC=$?
      if [ "$RC" -eq "0" ]; then
         echo "$SNAME is running..."
         exit 0
      else
         echo "$SNAME is stopped."
         exit 0
      fi
   ;;
   *)
     echo "Usage: /etc/init.d/hp-ams {start|stop|restart|status}"
     exit 1
esac

exit 0 

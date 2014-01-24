#
# Warning: you may need more libraries than are included here on the
# build line.  The agent frequently needs various libraries in order
# to compile pieces of it, but is OS dependent and we can't list all
# the combinations here.  Instead, look at the libraries that were
# used when linking the snmpd master agent and copy those to this
# file.
#

OS ?= linux
NETSNMP ?= net-snmp-5.7.1
CC = gcc

OPT ?= -O2

PWD = `pwd`

# shared library flags (assumes gcc)
MIB=Host
DLFLAGS=-fPIC -shared 
CFLAGS          = -fno-strict-aliasing $(OPT) -g -pipe -Werror -Wp,-D_FORTIFY_SOURCE=2 -fexceptions -fstack-protector --param=ssp-buffer-size=4 -mtune=generic -DNETSNMP_NO_INLINE -Ulinux -Dlinux=linux 
CPPFLAGS = -I. -I.. -I../cpqNic -I../cpqScsi -I../cpqIde $(shell if [ -f ../$(NETSNMP)/net-snmp-config ] ; then ../$(NETSNMP)/net-snmp-config --build-includes ../$(NETSNMP) ; fi) -DHELPER_VERSION=\"$(VERSION)\"
OBJS=cpqHost.o cpqHostMemory.o cpqHoSWRunningTable.o cpqHoSwVerTable.o cpqHoFileSysTable.o cpqHoFwVerTable.o
TARGETS=cpqHost.o 
NETSNMPVERSIONMAJ = $(shell if [ -f ../$(NETSNMP)/net-snmp-config ] ; then  ../$(NETSNMP)/net-snmp-config --version | cut -f1 -d\. ; fi  )
NETSNMPVERSIONMIN = $(shell if [ -f ../$(NETSNMP)/net-snmp-config ] ; then  ../$(NETSNMP)/net-snmp-config --version | cut -f2 -d\. ; fi  )
CFLAGS += -DNETSNMPVERSMINOR=$(NETSNMPVERSIONMIN)
PROVIDES=$(shell make provides)
NETSNMPCONFIG = ../$(NETSNMP)/net-snmp-config
BUILDAGENTLIBS = $(shell if [ -f $(NETSNMPCONFIG) ] ; then  $(NETSNMPCONFIG) --agent-libs ; fi)
BUILDNETSNMPDEPS = $(shell if [ -f $(NETSNMPCONFIG) ] ; then  $(NETSNMPCONFIG) --build-lib-deps ../$(NETSNMP) ; fi)
BUILDNETSNMPCMD =  $(shell if [ -f $(NETSNMPCONFIG) ] ; then  $(NETSNMPCONFIG) --build-command ;fi)
BUILDLIBS = $(BUILDNETSNMPDEPS)  -lm -lresolv -lcrypt $(DISTROLIBS)
EXTRAS = ../common/utils.o ../cpqNic/data_access/nic_util_linux.o


SUBDIRS := libcpqhost data_access

.PHONY: subdirs $(SUBDIRS)
.PHONY: all clean install source_tarball

all: subdirs $(OBJS)

subdirs:
	for d in $(SUBDIRS); do set -e; ${MAKE} -C $$d NETSNMP=$(NETSNMP) CFLAGS="$(CFLAGS)" OS=$(OS) ; done

$(SUBDIRS):
	cd $@ && ${MAKE} OS=$(OS) all;

clean:
	rm -f $(TARGETS) $(OBJS) *.o test_cpq$(MIB)
	for d in $(SUBDIRS); do ${MAKE} -C $$d clean; done

provides: 
	@echo $(addprefix $(DIR), $(OBJS) $(shell for d in $(SUBDIRS); do ${MAKE} -C $$d provides DIR=$$d/ OS=$(OS)|grep -v ^make; done))

test: test_cpq$(MIB)

test_cpq$(MIB): test_cpq$(MIB).o
	$(CC) -o test_cpq$(MIB) $(OBJS) */*.o  test_cpq$(MIB).o $(EXTRAS)  $(BUILDLIBS)  -lc -lrt $(DISTROLIBS)

test_cpq$(MIB).o: test_cpq$(MIB).c
	$(CC) $(CPPFLAGS) -I../net-snmp-5.7.2.pre1/include  -c test_cpq$(MIB).c


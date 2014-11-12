#
# Warning: you may need more libraries than are included here on the
# build line.  The agent frequently needs various libraries in order
# to compile pieces of it, but is OS dependent and we can't list all
# the combinations here.  Instead, look at the libraries that were
# used when linking the snmpd master agent and copy those to this
# file.
#

MIB=Scsi
OS ?= linux
NETSNMP ?= net-snmp-5.7.1
CC=gcc

PWD = `pwd`
LEVEL=../
NETSNMPDIR=$(LEVEL)$(NETSNMP)
NETSNMPINC=$(shell if [ -f $(NETSNMPDIR)/net-snmp-config ] ; then \
                       $(NETSNMPDIR)/net-snmp-config --build-includes $(NETSNMPDIR) ; \
                   fi)

# shared library flags (assumes gcc)
DLFLAGS=-fPIC -shared 
CFLAGS          ?= -fno-strict-aliasing -O2 -g -pipe -Werror -Wp,-D_FORTIFY_SOURCE=2 -fexceptions -fstack-protector --param=ssp-buffer-size=4 -mtune=generic -DNETSNMP_NO_INLINE -Ulinux -Dlinux=linux 
CPPFLAGS = -I. -I.. $(NETSNMPINC)

OBJS=cpqScsi.o cpqSasHbaTable.o cpqSasPhyDrvTable.o
TARGETS=cpqScsi.o cpqSasHbaTable.o cpqSasPhyDrvTable.o
PROVIDES=$(shell make provides)
NETSNMPCONFIG = ../$(NETSNMP)/net-snmp-config
BUILDAGENTLIBS = $(shell if [ -f $(NETSNMPCONFIG) ] ; then  $(NETSNMPCONFIG) --agent-libs ; fi)
BUILDNETSNMPDEPS = $(shell if [ -f $(NETSNMPCONFIG) ] ; then  $(NETSNMPCONFIG) --build-lib-deps ../$(NETSNMP) ; fi)
BUILDNETSNMPCMD =  $(shell if [ -f $(NETSNMPCONFIG) ] ; then  $(NETSNMPCONFIG) --build-command ;fi)
BUILDLIBS = $(BUILDNETSNMPDEPS)  -lm -lresolv -lcrypt $(DISTROLIBS)
EXTRAS = ../common/utils.o ../common/scsi_info.o ../cpqHost/cpqHoFwVerTable.o

SUBDIRS = data_access

.PHONY: subdirs $(SUBDIRS)
.PHONY: all clean install source_tarball

all: $(OBJS)
	for d in $(SUBDIRS); do ${MAKE} -C $$d NETSNMP=$(NETSNMP) OS=$(OS) CFLAGS="$(CFLAGS)" all; done

subdirs:
	 for d in $(SUBDIRS); do set -e; ${MAKE} -C $$d; done

$(SUBDIRS):
	cd $@ && ${MAKE} all;

clean:
	rm -f $(TARGETS) $(OBJS) test_cpq$(MIB)
	for d in $(SUBDIRS); do ${MAKE} -C $$d clean; done

provides:
	@echo $(addprefix $(DIR), $(OBJS) $(shell for d in $(SUBDIRS); do ${MAKE} -C $$d provides OS=$(OS) DIR=$$d/ |grep -v ^make; done))

test: test_cpq$(MIB)

test_cpq$(MIB): test_cpq$(MIB).o
	$(CC) -o test_cpq$(MIB) $(OBJS) */*.o  test_cpq$(MIB).o $(EXTRAS)  $(BUILDLIBS)  -lc -lrt

test_cpq$(MIB).o: test_cpq$(MIB).c
	$(CC) $(CPPFLAGS) -I../net-snmp-5.7.2.pre1/include  -c test_cpq$(MIB).c


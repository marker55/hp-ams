#
# Warning: you may need more libraries than are included here on the
# build line.  The agent frequently needs various libraries in order
# to compile pieces of it, but is OS dependent and we can't list all
# the combinations here.  Instead, look at the libraries that were
# used when linking the snmpd master agent and copy those to this
# file.
#

OS ?= linux
NETSNMP ?= net-snmp-5.5
CC=gcc

PWD = `pwd`
BUILDAGENTLIBS =$(shell ../$(NETSNMP)/net-snmp-config --agent-libs) -lpci -lsysfs
BUILDNETSNMPDEPS =$(shell ../$(NETSNMP)/net-snmp-config --build-lib-deps $(NETSNMP))
BUILDNETSNMPCMD = $(shell ../$(NETSNMP)/net-snmp-config --build-ommand)
BUILDLIBS = $(BUILDNETSNMPDEPS) -lpci -lsysfs -lcrypto -lm -lpthread


# shared library flags (assumes gcc)
DLFLAGS=-fPIC -shared 
CFLAGS   ?= -fno-strict-aliasing -O2 -g -pipe -Wall -Wp,-D_FORTIFY_SOURCE=2 -fexceptions -fstack-protector --param=ssp-buffer-size=4 -mtune=generic -DNETSNMP_NO_INLINE -Ulinux -Dlinux=linux 
CPPFLAGS = -I. -I.. $(shell ../$(NETSNMP)/net-snmp-config --build-includes ../$(NETSNMP))
OBJS=cpqIde.o cpqIdeControllerTable.o cpqIdeAtaDiskTable.o
TARGETS=cpqIde.o cpqIdeControllerTable.o cpqIdeAtaDiskTable.o

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
	rm -f $(TARGETS) $(OBJS)
	for d in $(SUBDIRS); do ${MAKE} -C $$d clean; done

provides:
	@echo $(addprefix $(DIR), $(OBJS) $(shell for d in $(SUBDIRS); do ${MAKE} -s -C $$d provides OS=$(OS) DIR=$$d/; done))



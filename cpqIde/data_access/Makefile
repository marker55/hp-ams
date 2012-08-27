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
DLFLAGS=-fPIC -shared 
CFLAGS          ?= -fno-strict-aliasing $(OPT) -g -pipe -Wall -Wp,-D_FORTIFY_SOURCE=2 -fexceptions -fstack-protector --param=ssp-buffer-size=4 -mtune=generic -DNETSNMP_NO_INLINE -Ulinux -Dlinux=linux 
CPPFLAGS = -I../.. -I. -I.. $(shell if [ -f ../../$(NETSNMP)/net-snmp-config ] ; then  ../../$(NETSNMP)/net-snmp-config --build-includes ../../$(NETSNMP); fi) -I../../cpqHost
OBJS=ide_linux.o

#SUBDIRS := libcpqnic

.PHONY: subdirs $(SUBDIRS)
.PHONY: all clean install source_tarball

all: $(OBJS)

subdirs:
	 for d in $(SUBDIRS); do set -e; ${MAKE} -C $$d; done

$(SUBDIRS):
	cd $@ && ${MAKE} all;

clean:
	rm -f $(TARGETS) $(OBJS)
	for d in $(SUBDIRS); do ${MAKE} -C $$d clean; done

provides:
	@echo $(addprefix $(DIR), $(OBJS) )



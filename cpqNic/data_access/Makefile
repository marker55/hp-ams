OS ?= linux
NETSNMP ?= net-snmp-5.7.1

CPQ_INCLUDES = -I../../$(NETSNMP)/include -I../../common

OPT ?= -O2

CFLAGS   ?= -fno-strict-aliasing $(OPT) -g -pipe -Wall -Wp,-D_FORTIFY_SOURCE=2 -fexceptions -fstack-protector --param=ssp-buffer-size=4 -mtune=generic -DNETSNMP_NO_INLINE -Ulinux -Dlinux=linux 
CPPFLAGS = -I../.. -I. -I..  $(shell if [ -f ../../$(NETSNMP)/net-snmp-config ] ; then  ../../$(NETSNMP)/net-snmp-config --build-includes ../../$(NETSNMP); fi)  -I../../common -I../../cpqHost
#OBJS = common_utilities.o port_map.o 
OBJS = cpqnic.o nic_$(OS).o nic_util_$(OS).o

all:	$(OBJS)

#nic_linux.o:	nic_linux.c nic_linux.h
#	gcc $(CFLAGS) $(CPPFLAGS) -c -D_REENTRANT nic_linux.c -o nic_linux.o

#	ar -r libnic_linux.a nic_linux.o
#	ld -Bdynamic -shared -o libnic_linux.so -ldl nic_linux.o

clean clobber:
	rm -f *.o libnic_linux.so libnic_linux.a get_stats

provides:
	@echo $(addprefix $(DIR), $(OBJS) )

OS ?= linux
NETSNMP ?= net-snmp-5.5

CPQ_INCLUDES = -I../../$(NETSNMP)/include -I../../common

BUILDNETSNMPDEPS =`../../$(NETSNMP)/net-snmp-config --build-lib-deps ../../$(NETSNMP)`
BUILDLIBS = $(BUILDNETSNMPDEPS) -lpci -lsysfs -lcrypto -lm -lpthread
LDFLAGS = $(BUILDLIBS)

CFLAGS   ?= -fno-strict-aliasing -O2 -g -pipe -Wall -Wp,-D_FORTIFY_SOURCE=2 -fexceptions -fstack-protector --param=ssp-buffer-size=4 -mtune=generic -DNETSNMP_NO_INLINE -Ulinux -Dlinux=linux 
CPPFLAGS = -I.. -I. $(shell ../../$(NETSNMP)/net-snmp-config --build-includes ../../$(NETSNMP)) -I../../common
OBJS = common_utilities.o port_map.o 

all:	cpqnic.o $(OBJS)

cpqnic.o:	cpqnic.c cpqnic.h
	gcc $(CFLAGS) $(CPPFLAGS) -c -D_REENTRANT cpqnic.c -o cpqnic.o

#	ar -r libcpqnic.a cpqnic.o
#	ld -Bdynamic -shared -o libcpqnic.so -ldl cpqnic.o

get_stats:	cpqnic.c cpqnic.h
	gcc -g -DUNIT_TEST_GET_STATS -D_REENTRANT  cpqnic.c $(OBJS) \
               ${CPQ_INCLUDES} $(LDPATH) $(LDFLAGS) -o get_stats

clean clobber:
	rm -f *.o libcpqnic.so libcpqnic.a get_stats

provides:
	@echo $(addprefix $(DIR), $(OBJS) cpqnic.o)

OS ?= linux
NETSNMP ?= net-snmp-5.7.1

OPT ?= -O2

CPPFLAGS = -I. -I.. -I../../$(NETSNMP)/include -I../../ -I../../common

CFLAGS ?= -fno-strict-aliasing $(OPT) -g -pipe -Wall -Wp,-D_FORTIFY_SOURCE=2 -fexceptions -fstack-protector --param=ssp-buffer-size=4 -mtune=generic -DNETSNMP_NO_INLINE -Ulinux -Dlinux=linux 

OBJS = pkg_$(OS).o drv_$(OS).o svc_$(OS).o perf_$(OS).o perf.o

all: $(OBJS)

perf.o: perf.c 
	gcc $(CFLAGS) -c -DHP_NETSNMP -D_REENTRANT perf.c ${CPPFLAGS} -o perf.o

pkg_$(OS).o: pkg_$(OS).c  package.h
	gcc $(CFLAGS) -c -DHP_NETSNMP -D_REENTRANT pkg_$(OS).c ${CPPFLAGS} -o pkg_$(OS).o

svc_$(OS).o: svc_$(OS).c  service.h
	gcc $(CFLAGS) -c -DHP_NETSNMP -D_REENTRANT svc_$(OS).c ${CPPFLAGS} -o svc_$(OS).o

drv_$(OS).o: drv_$(OS).c  driver.h
	gcc $(CFLAGS) -c -DHP_NETSNMP -D_REENTRANT drv_$(OS).c ${CPPFLAGS} -o drv_$(OS).o

perf_$(OS).o: perf_$(OS).c
	gcc $(CFLAGS) -c -DHP_NETSNMP -D_REENTRANT perf_$(OS).c ${CPPFLAGS} -o perf_$(OS).o


clean clobber:
	rm -f *.o 

provides:
	@echo $(addprefix $(DIR), $(OBJS)) 

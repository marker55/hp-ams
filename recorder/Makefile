OS ?= linux
NETSNMP ?= net-snmp-5.7.1

OPT ?= -O2

# -I/build/toolchain/lin32/RHEL5/usr/include
CPPFLAGS = -I../$(NETSNMP)/include -I../ -I../common

CFLAGS = -fno-strict-aliasing $(OPT) -g -pipe -Werror -Wp,-D_FORTIFY_SOURCE=2 -fexceptions -fstack-protector --param=ssp-buffer-size=4 -mtune=generic -DNETSNMP_NO_INLINE -Ulinux -Dlinux=linux -D_GNU_SOURCE

OBJS = $(OS).o log.o osinfo.o boot.o driver.o service.o osusage.o memusage.o procusage.o common.o crash.o package.o

SUBDIRS = data_access

.PHONY: subdirs $(SUBDIRS)
.PHONY: all clean 

all: $(OBJS) subdirs

subdirs:
	for d in $(SUBDIRS); do set -e; ${MAKE} NETSNMP=$(NETSNMP) OS=$(OS) CFLAGS="$(CFLAGS)" -C $$d; done

$(SUBDIRS):
	cd $@ && ${MAKE} all;

$(OS).o: $(OS).c recorder.h recorder_types.h
	gcc $(CFLAGS) -c -DHP_NETSNMP -D_REENTRANT $(OS).c ${CPPFLAGS} -o $(OS).o

common.o: common.c recorder.h recorder_types.h
	gcc $(CFLAGS) -c -DHP_NETSNMP -D_REENTRANT common.c ${CPPFLAGS} -o common.o

log.o: log.c recorder.h recorder_types.h
	gcc $(CFLAGS) -c -DHP_NETSNMP -D_REENTRANT log.c ${CPPFLAGS} -o log.o

osusage.o: osusage.c recorder.h recorder_types.h
	gcc $(CFLAGS) -c -DHP_NETSNMP -D_REENTRANT osusage.c ${CPPFLAGS} -o osusage.o

memusage.o: memusage.c recorder.h recorder_types.h
	gcc $(CFLAGS) -c -DHP_NETSNMP -D_REENTRANT memusage.c ${CPPFLAGS} -o memusage.o

procusage.o: procusage.c recorder.h recorder_types.h
	gcc $(CFLAGS) -c -DHP_NETSNMP -D_REENTRANT procusage.c ${CPPFLAGS} -o procusage.o

osinfo.o: osinfo.c recorder.h recorder_types.h
	gcc $(CFLAGS) -c -DHP_NETSNMP -D_REENTRANT osinfo.c ${CPPFLAGS} -o osinfo.o

driver.o: driver.c recorder.h recorder_types.h
	gcc $(CFLAGS) -c -DHP_NETSNMP -D_REENTRANT driver.c ${CPPFLAGS} -o driver.o

service.o: service.c recorder.h recorder_types.h
	gcc $(CFLAGS) -c -DHP_NETSNMP -D_REENTRANT service.c ${CPPFLAGS} -o service.o

boot.o: boot.c recorder.h recorder_types.h
	gcc $(CFLAGS) -c -DHP_NETSNMP -D_REENTRANT boot.c ${CPPFLAGS} -o boot.o

clean clobber:
	rm -f *.o 
	for d in $(SUBDIRS); do ${MAKE} -C $$d clean; done


provides:
	@echo $(addprefix $(DIR), $(OBJS) $(shell for d in $(SUBDIRS); do ${MAKE} -s -C $$d provides OS=$(OS) DIR=$$d/; done))

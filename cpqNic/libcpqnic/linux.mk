
ifeq ($(ARCH),x86_64)
BITS=64
endif

DIR_LEVEL=../../$(CMANIC_DIR_LEVEL)

SNMP_FLAGS = $(shell if [ -x /usr/bin/net-snmp-config-$(ARCH) ]; then \
              net-snmp-config-$(ARCH) --agent-libs ; else    \
              net-snmp-config --agent-libs ;      \
              fi)
CPQ_INCLUDES=-I../../common

LDFLAGS := $(SNMP_FLAGS)           \
          ../../common/libcommon_utilities.a    \
          $(DIR_LEVEL)/shared/cmacommon/libcmacommon$(BITS).so       \
          $(DIR_LEVEL)/shared/cmapeer/libcmapeer$(BITS).so           \
          $(DIR_LEVEL)/shared/hpasmintrfc/libhpasmintrfc$(BITS).so   \
          $(DIR_LEVEL)/shared/cpqci/linux/libcpqci$(BITS).so 

all:	cpqnic.o get_stats

cpqnic.o:  ../../common/libcommon_utilities.a cpqnic.c cpqnic.h
	$(CC) -c -D_REENTRANT -DHP_NETSNMP cpqnic.c ${CPQ_INCLUDES} $(C32FLAGS) -o cpqnic.o

../../common/libcommon_utilities.a:
	make -e -C ../../common -f linux.mk all

#	ar -r libcpqnic.a cpqnic.o
#	ld -Bdynamic -shared -o libcpqnic.so -ldl cpqnic.o

get_stats:	cpqnic.c cpqnic.h
	$(CC) -g -DUNIT_TEST_GET_STATS -D_REENTRANT -DHP_NETSNMP cpqnic.c ${CPQ_INCLUDES} $(C32FLAGS) $(LDPATH) $(LDFLAGS) -o get_stats

clean clobber:
	rm -f *.o libcpqnic.so libcpqnic.a get_stats

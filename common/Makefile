OS ?= Makefile
NETSNMP ?= net-snmp-5.7.1

OPT ?= -O2

CPQ_INCLUDES = 
CFLAGS = -fno-strict-aliasing $(OPT) -g -pipe -Werror -Wp,-D_FORTIFY_SOURCE=2 -fexceptions -fstack-protector --param=ssp-buffer-size=4 -mtune=generic -DNETSNMP_NO_INLINE -Ulinux -Dlinux=linux -I. -I $(shell if [ -f ../$(NETSNMP)/net-snmp-config ] ; then ../$(NETSNMP)/net-snmp-config --build-includes ../$(NETSNMP) ;fi)

OBJS = utils.o smbios.o scsi_info.o

all:    $(OBJS)

scsi_info.o:    scsi_info.c
	gcc -c $(CFLAGS)  scsi_info.c ${CPQ_INCLUDES} -o scsi_info.o

smbios.o:        smbios.c smbios.h
	gcc -c $(CFLAGS) smbios.c ${CPQ_INCLUDES} -o smbios.o

utils.o:        utils.c utils.h
	gcc -c $(CFLAGS) utils.c ${CPQ_INCLUDES} -o utils.o

#       ar -r libcpqnic.a cpqnic.o
#       ld -Bdynamic -shared -o libcpqnic.so -ldl cpqnic.o

clean clobber:
	rm -f *.o libcpqnic.so libcpqnic.a get_stats

provides:
	@echo $(addprefix $(DIR), $(OBJS)) 

test:

OS ?= Makefile
NETSNMP ?= net-snmp-5.7.1

OPT ?= -O2

LEVEL=../
NETSNMPDIR=$(LEVEL)$(NETSNMP)
NETSNMPINC=$(shell if [ -f $(NETSNMPDIR)/net-snmp-config ] ; then \
                       $(NETSNMPDIR)/net-snmp-config --build-includes $(NETSNMPDIR) ; \
                   fi)

CFLAGS ?= -fno-strict-aliasing $(OPT) -g -pipe -Werror -Wp,-D_FORTIFY_SOURCE=2 -fexceptions -fstack-protector --param=ssp-buffer-size=4 -mtune=generic -DNETSNMP_NO_INLINE -Ulinux -Dlinux=linux 
CPPFLAGS = -I.. -I. $(NETSNMPINC)
OBJS = utils.o smbios.o scsi_info.o nl_udev.o

all:    $(OBJS)

scsi_info.o:    scsi_info.c
	gcc -c $(CFLAGS)  $(CPPFLAGS) scsi_info.c -o scsi_info.o

smbios.o:        smbios.c smbios.h
	gcc -c $(CFLAGS) $(CPPFLAGS) smbios.c -o smbios.o

utils.o:        utils.c utils.h
	gcc -c $(CFLAGS) $(CPPFLAGS) utils.c -o utils.o

nl_udev.o:        nl_udev.c nl_udev.h
	gcc -c $(CFLAGS) $(CPPFLAGS) nl_udev.c -o nl_udev.o

#       ar -r libcpqnic.a cpqnic.o
#       ld -Bdynamic -shared -o libcpqnic.so -ldl cpqnic.o

clean clobber:
	rm -f *.o libcpqnic.so libcpqnic.a get_stats

provides:
	@echo $(addprefix $(DIR), $(OBJS)) 

test:

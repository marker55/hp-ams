OS ?= linux
NETSNMP ?= net-snmp-5.7.2
LEVEL = ../../
NETSNMPDIR = $(LEVEL)$(NETSNMP)
NETSNMPINC = $(shell if [ -f $(NETSNMPDIR)/net-snmp-config ] ; then  \
						$(NETSNMPDIR)/net-snmp-config --build-includes $(NETSNMPDIR) ;\
						fi)

distro ?= $(shell if [ -f /etc/redhat-release ] ; then \
                     echo "_redhat"; \
                  elif [ -f /etc/SuSE-release ] ; then \
                     echo "_suse"; \
                  elif [ -f /etc/lsb-release ] ; then \
                     echo "_ubuntu"; \
                  elif [ -f /etc/debian_version ] ; then \
                     echo "_ubuntu"; \
                  else \
                     echo "_ubuntu"; fi )

OPT ?= -O2

OBJS = distro.o
CFLAGS  ?= -fno-strict-aliasing $(OPT) -g -pipe -Wall -Wp,-D_FORTIFY_SOURCE=2 -fexceptions -fstack-protector --param=ssp-buffer-size=4 -mtune=generic -DNETSNMP_NO_INLINE -Ulinux -Dlinux=linux
CPPFLAGS = -I../.. $(NETSNMPINC) -D$(distro)

all:	$(OBJS)

distro.o:     distro$(distro).c
	gcc -c  $(CFLAGS) $(CPPFLAGS) distro$(distro).c  -o distro.o

provides:
	@echo $(addprefix $(DIR),$(OBJS) )

clean clobber:
	rm -f *.o 

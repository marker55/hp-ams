# Overridden during automated builds
COMP_NAME    ?= hp-ams
COMP_VER     ?= 2.1.0
COMP_PKG_REV ?= 666

NAME         ?= $(COMP_NAME)
VERSION      ?= $(COMP_VER)
BUILD_NUMBER ?= $(COMP_PKG_REV)

ARCH ?= $(shell uname -m)
MAKE := make ARCH=${ARCH}
#
RPMSOURCES=$(shell rpm --eval %{_sourcedir})
RPMRPMS=$(shell rpm --eval %{_rpmdir})
RPM_OPT_FLAGS=$(shell  if [ -f /etc/redhat-release ] ; then \
						rpm --eval %{__global_cflags} ; \
                       elif [ -f /etc/SuSE-release ] ; then \
                        rpm --eval %{optflags} ; \
					   else  \
						echo "-O2" ; \
					   fi)
					    
INSTALL=install
DIRINSTALL=install -d
ifeq "$(RPM_BUILD_ROOT)" ""
PREFIX ?= /usr/local
else
PREFIX ?= /
DESTDIR = $(RPM_BUILD_ROOT)
endif
SBINDIR=$(DESTDIR)$(PREFIX)/sbin
ETCDIR=$(DESTDIR)$(PREFIX)/etc
INITDIR=$(ETCDIR)/init.d
SYSTEMD= $(shell rpm --eval %{?_unitdir:1}%{\!?_unitdir:0})
UNITDIR= $(shell rpm --eval %{?_unitdir})
HPAMSDIR=$(DESTDIR)$(PREFIX)/opt/hp/hp-ams
MANDIR:=$(shell if [ -d $(PREFIX)/share/man ] ; then \
                    echo $(DESTDIR)/$(PREFIX)/share/man ; \
                elif [ -d $(PREFIX)/usr/share/man ] ; then \
                    echo $(DESTDIR)/$(PREFIX)/usr/share/man ; \
                else \
                    echo $(DESTDIR)/$(PREFIX)/usr/man; fi )
DISTROLIBS ?= $(shell if [ -f /etc/redhat-release ] ; then \
                     echo "-lrpmio -lrpm -lpci"; \
                  elif [ -f /etc/SuSE-release ] ; then \
                     echo "-lrpmio -lrpm -lpci"; \
                  elif [ -f /etc/lsb-release ] ; then \
                     echo "-lrt"; \
                  elif [ -f /etc/debian_version ] ; then \
                     echo "-lrt"; \
                  else \
                     echo ""; fi )

export SHELL=/bin/bash

OS = linux
NETSNMP ?= net-snmp-5.7.2
NETSNMPCONFIG = $(NETSNMP)/net-snmp-config
export OS NETSNMP VERSION
NETSNMPVERSIONMIN = $(shell echo $(NETSNMP)| cut -f2 -d\.  )
MIBS="host agentx/subagent mibII hardware/memory"
NOTMIBS="mibII/system_mib mibII/snmp_mib_5_5 mibII/vacm_vars mibII/vacm_conf mibII/snmp mibII/tcp mibII/udp mibII/at mibII/sysORTable mibII/icmp mibII/ipv6 mibII/setSerialNo ip-mib/inetNetToMediaTable host/hrh_storage host/hr_disk host/hrh_filesys host/hr_network host/hr_other host/hr_partition host/hr_print target agent_mibs cd_snmp notification notification-log-mib disman/event disman/schedule snmpv3mibs mibII/vacm utilities/execute mibII/tcpTable ucd-snmp/dlmod" 
CONF_OPTIONS=--enable-read-only --disable-set-support --disable-agent --disable-privacy --without-openssl $(shell if [ $(SYSTEMD) = 1 ] ; then echo "--with-systemd" ; fi)
LEVEL=./
NETSNMPDIR=$(LEVEL)$(NETSNMP)
NETSNMPINC=$(shell if [ -f $(NETSNMPCONFIG) ] ; then \
                       $(NETSNMPCONFIG) --build-includes $(NETSNMPDIR) ; \
                   fi)

NETSNMPTAR = $(NETSNMP).tar.gz
CC = gcc

PWD = `pwd`
CPPFLAGS = -I. -I ./include $(NETSNMPINC)  -I$(NETSNMP)/agent/mibgroup/mibII
CFLAGS=$(shell if [ -f $(NETSNMPCONFIG) ] ; then  $(NETSNMPCONFIG) --cflags ; fi)
LDFLAGS=$(shell if [ -f $(NETSNMPCONFIG) ] ; then $(NETSNMPCONFIG) --ldflags ; fi)

BUILDAGENTLIBS = $(shell if [ -f $(NETSNMPCONFIG) ] ; then  $(NETSNMPCONFIG) --agent-libs ; fi)   
BUILDNETSNMPDEPS = $(shell if [ -f $(NETSNMPCONFIG) ] ; then  $(NETSNMPCONFIG) --build-lib-deps $(NETSNMP) ; fi)
BUILDNETSNMPCMD =  $(shell if [ -f $(NETSNMPCONFIG) ] ; then  $(NETSNMPCONFIG) --build-command ;fi)
BUILDLIBS = $(BUILDNETSNMPDEPS)  -lm -lresolv -lcrypt $(DISTROLIBS)

TARFILE=$(NAME)-$(VERSION).tar.gz

OBJS=hpHelper.o 

TARGETS=hpHelper

AMSDIRS = cpqHost cpqNic cpqSe cpqFca cpqScsi cpqIde cpqPerf common recorder

SUBDIRS = $(NETSNMP) $(AMSDIRS)
 
OBJS2=$(shell for d in $(AMSDIRS); do ${MAKE} -C $$d provides OS=$(OS) DIR=$$d/|grep -v ^make; done)

all: subdirs hpHelper 

net-snmp-patch: net-snmp-patch-stamp
net-snmp-patch-stamp: net-snmp-untar-stamp
	cd $(NETSNMP) ; for i in ../patches/$(NETSNMP)-*.patch ; do \
            if [ -f $$i ] ; then \
	    	echo "patching net-snmp with $$i"; \
	    	patch -p1 < $$i ; \
	    fi; \
	    done  
	cd $(NETSNMP) ; for i in ../patches/$(OS)/$(NETSNMP)-*.patch ; do \
            if [ -f $$i ] ; then \
	    	echo "patching net-snmp with $$i"; \
	    	patch -p1 < $$i ;  \
	    fi \
	    done  
	touch $@

net-snmp-untar: net-snmp-untar-stamp
net-snmp-untar-stamp: $(NETSNMPTAR)
	tar xovfz $(NETSNMPTAR) 
	touch $@

net-snmp-configure: net-snmp-configure-stamp
net-snmp-configure-stamp: net-snmp-patch-stamp
	cd $(NETSNMP) ; ./configure \
            --with-cflags="$(RPM_OPT_FLAGS)  -D_RPM_4_4_COMPAT -DNETSNMP_NO_INLINE" \
            --with-sys-location=Unknown \
            --with-ldflags="-Wl,-z,relro -Wl,-z,now" \
            --with-sys-contact=root@localhost \
            --with-logfile=/var/log/snmpd.log \
            --with-nl \
            --with-persistent-directory=/var/net-snmp \
            --enable-ucd-snmp-compatibility \
            --enable-static \
            --disable-shared \
            --with-pic \
            --disable-embedded-perl \
            --without-perl-modules \
            --with-default-snmp-version=2 \
            --enable-mfd-rewrites \
            --with-out-mib-modules=$(NOTMIBS) \
	    --with-mib-modules=$(MIBS) \
            --with-out-transports="TCPIPv6 UDPIPv6 SSH TCP Alias" \
            --with-transports="HPILO UDP Unix" \
	    --disable-manuals \
	    --disable-applications \
	    --disable-md5 \
	    --disable-scripts \
	    --disable-mib-loading $(CONF_OPTIONS)
	touch $@

subdirs: $(SUBDIRS) net-snmp-configure-stamp
	@if test "$(SUBDIRS)" != ""; then \
		it="$(SUBDIRS)" ; \
		for i in $$it ; do \
			echo "making all in `pwd`/$$i"; \
			export CFLAGS=`$(NETSNMPCONFIG) --cflags`; \
			( cd $$i ; $(MAKE)  OS=$(OS) VERSION=$(VERSION)) ; \
			if test $$? != 0 ; then \
				exit 1 ; \
			fi  \
		done \
	fi

test: hpHelper $(SUBDIRS) net-snmp-configure-stamp
	for d in $(AMSDIRS); do echo $$d ; if [ -d $$d ] ; then ${MAKE} -C $$d test DISTROLIBS=$(DISTROLIBS) NETSNMP=$(NETSNMP) OS=$(OS);fi; done

testHelper: testHelper.o $(BUILDNETSNMPDEPS)
	(CC) -o testHelper testHelper.o $(BUILDLIBS)

hpHelper.o: hpHelper.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o hpHelper.o -c hpHelper.c

hpHelper: $(OBJS) $(OBJS2) $(BUILDNETSNMPDEPS)
	$(CC) -o hpHelper $(LDFLAGS) $(OBJS) $(OBJS2) $(BUILDLIBS)

clean:
	rm -f $(TARGETS) $(OBJS) 
			for d in $(SUBDIRS); do echo $$d ; if [ -d $$d ] ; then ${MAKE} -C $$d clean;fi; done
			rm -f  debian/changelog
	echo "Finished cleaning"

distclean: clean
	rm -rf tmp
	rm -rf $(TARFILE)
	rm -rf $(NAME).spec
	rm -rf *-stamp
	rm -rf $(NETSNMP)
	rm -rf debian/build
	rm -rf debian/hp-ams
	echo $(OBJS2)

$(NAME).spec:   $(NAME).spec.in
	sed "s/\%VERSION\%/$(VERSION)/g" < $< > $<.tmp
	sed -i "s/\%RELEASE\%/$(subst %,\%,$(BUILD_NUMBER))/g" $<.tmp
	mv $<.tmp $@

debian/changelog: debian/changelog.in
	sed "s/\%VERSION\%/$(VERSION)/g" < $< > $<.tmp
	sed -i "s/\%RELEASE\%/$(subst %,\%,$(BUILD_NUMBER))/g" $<.tmp
	sed -i "s/\%DATE\%/`date -R`/g" $<.tmp
	mv $<.tmp $@

install: all
	$(DIRINSTALL) -m 755 $(SBINDIR) $(ETCDIR)/sysconfig $(HPAMSDIR)
	$(DIRINSTALL) -m 755 $(MANDIR)/man8
	$(INSTALL) -m 755 ./hpHelper $(SBINDIR)
	$(INSTALL) -m 644 ./doc/hpHelper.8 $(MANDIR)/man8
	$(INSTALL) -m 644 ./hp-ams.license $(HPAMSDIR)
	$(INSTALL) -m 644 ./hp-ams.config $(ETCDIR)/sysconfig/hp-ams 
	if [ $(SYSTEMD) = 1 ] ; then \
		   echo UNITDIR=$(UNITDIR). ; \
	       $(DIRINSTALL) -m 755 $(DESTDIR)/$(UNITDIR) ; \
           $(INSTALL) -m 644 ./hp-ams.service $(DESTDIR)/$(UNITDIR)/hp-ams.service ; \
	else \
		   echo NOT UNITDIR=$(UNITDIR). ; \
	       $(DIRINSTALL) -m 755 $(INITDIR) ; \
           $(INSTALL) -m 755 ./hp-ams.sh $(INITDIR)/hp-ams ; \
	fi
	gzip -f $(MANDIR)/man8/hpHelper.8


source_tarball: $(TARFILE)
$(TARFILE): debian/changelog $(NAME).spec
	mkdir -p tmp/$(NAME)-$(VERSION)
	tar -c * --exclude '*/.svn' \
	         --exclude tmp \
                 --exclude '*~' \
                 --exclude '#*#' \
                 --exclude $(NAME).spec \
                 --exclude '.*.swp' | (cd tmp/$(NAME)-$(VERSION) && tar x)
	$(MAKE) -C tmp/$(NAME)-$(VERSION) distclean
	$(MAKE) -C tmp/$(NAME)-$(VERSION) \
          COMP_NAME=$(COMP_NAME) COMP_VER=$(COMP_VER) \
          COMP_PKG_REV=$(COMP_PKG_REV) $(NAME).spec
	tar -C tmp -cz $(NAME)-$(VERSION) > $@


provides:
	@echo $(OBJS2)

.PHONY: subdirs $(SUBDIRS) all clean install source_tarball


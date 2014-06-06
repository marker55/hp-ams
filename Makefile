# Overridden during automated builds
COMP_NAME    ?= hp-ams
COMP_VER     ?= 2.0.0
COMP_PKG_REV ?= 1

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
					    
#
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
HPAMSDIR=$(DESTDIR)$(PREFIX)/opt/hp/hp-ams
DOCDIR:=$(shell if [ -d $(PREFIX)/share/ ] ; then \
                    echo $(DESTDIR)/$(PREFIX)/share/doc/$(NAME)-$(VERSION) ; \
                elif [ -d $(PREFIX)/usr/share/ ] ; then \
                    echo $(DESTDIR)/$(PREFIX)/usr/share/doc/$(NAME)-$(VERSION) ; \
                else \
                    echo $(DESTDIR)/$(PREFIX)/usr/doc/$(NAME)-$(VERSION); fi )
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

#
export SHELL=/bin/bash

OS = linux
NETSNMP ?= net-snmp-5.7.2
NETSNMPCONFIG = $(NETSNMP)/net-snmp-config
export OS NETSNMP VERSION
NETSNMPVERSIONMIN = $(shell echo $(NETSNMP)| cut -f2 -d\.  )
ifeq "$(NETSNMPVERSIONMIN)" "7"
MIBS="host agentx/subagent mibII mibII/ipv6"
NOTMIBS="mibII/vacm_vars mibII/vacm_conf target agent_mibs cd_snmp notification notification-log-mib disman/event disman/schedule snmpv3mibs mibII/vacm utilities/execute mibII/tcpTable" 
OPTIONS=--enable-read-only --disable-set-support --disable-agent --disable-privacy --without-openssl
else
MIBS="hardware/fsys"
NOTMIBS="target agent_mibs cd_snmp notification notification-log-mib disman/event disman/schedule snmpv3mibs" 
OPTIONS=""
endif

NETSNMPTAR = $(NETSNMP).tar.gz
CC = gcc

PWD = `pwd`
CPPFLAGS = -I. -I ./include  $(shell  if [ -f $(NETSNMPCONFIG) ] ; then $(NETSNMPCONFIG) --build-includes $(NETSNMP); fi) -I$(NETSNMP)/agent/mibgroup/mibII
BUILDAGENTLIBS = $(shell if [ -f $(NETSNMPCONFIG) ] ; then  $(NETSNMPCONFIG) --agent-libs ; fi)   
BUILDNETSNMPDEPS = $(shell if [ -f $(NETSNMPCONFIG) ] ; then  $(NETSNMPCONFIG) --build-lib-deps $(NETSNMP) ; fi)
BUILDNETSNMPCMD =  $(shell if [ -f $(NETSNMPCONFIG) ] ; then  $(NETSNMPCONFIG) --build-command ;fi)
BUILDLIBS = $(BUILDNETSNMPDEPS)  -lm -lresolv -lcrypt $(DISTROLIBS)

TARFILE=$(NAME)-$(VERSION).tar.gz

OBJS=hpHelper.o 

TARGETS=hpHelper

AMSDIRS = cpqHost cpqNic cpqSe cpqFca cpqScsi cpqIde common recorder

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
            --enable-static --disable-shared \
            --with-cflags="$(RPM_OPT_FLAGS)  -D_RPM_4_4_COMPAT -DNETSNMP_NO_INLINE" \
            --with-ldflags="-Wl,-z,relro -Wl,-z,now" \
            --with-sys-location=Unknown \
            --with-sys-contact=root@localhost \
            --with-logfile=/var/log/snmpd.log \
            --with-nl \
            --with-persistent-directory=/var/lib/net-snmp \
            --enable-ucd-snmp-compatibility \
            --with-pic \
            --disable-embedded-perl \
            --without-perl-modules \
            --with-default-snmp-version=2 \
            --enable-mfd-rewrites \
            --with-out-mib-modules=$(NOTMIBS) \
	    --with-mib-modules=$(MIBS) \
            --with-out-transports="TCPIPv6 UDPIPv6 SSH TCP Alias" \
            --with-transports="HPILO Unix" \
	    --disable-manuals \
	    --disable-applications \
	    --disable-md5 \
	    --disable-scripts \
	    --disable-mib-loading $(OPTIONS)
	touch $@

subdirs: $(SUBDIRS) net-snmp-configure-stamp
	@if test "$(SUBDIRS)" != ""; then \
		it="$(SUBDIRS)" ; \
		for i in $$it ; do \
			echo "making all in `pwd`/$$i"; \
			export CFLAGS=`$(NETSNMPCONFIG) --cflags`; \
			export LDFLAGS=`$(NETSNMPCONFIG) --ldflags`; \
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

hpHelper: $(OBJS) $(OBJS2) $(BUILDNETSNMPDEPS)
	$(CC) -o hpHelper `${NETSNMPCONFIG} --cflags` `${NETSNMPCONFIG} --ldflags` $(OBJS) $(OBJS2) $(BUILDLIBS)

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
	$(DIRINSTALL) -m 755 $(SBINDIR) $(INITDIR) $(HPAMSDIR) $(DOCDIR)
	$(DIRINSTALL) -m 755 $(MANDIR)/man8
	$(INSTALL) -m 755 ./hpHelper $(SBINDIR)
	$(INSTALL) -m 755 ./hp-ams.sh $(INITDIR)/hp-ams
	$(INSTALL) -m 644 ./doc/hpHelper.8 $(MANDIR)/man8
	$(INSTALL) -m 644 ./LICENSE $(DOCDIR)
	$(INSTALL) -m 644 $(NETSNMP)/COPYING $(DOCDIR)

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
	tar -C tmp/$(NAME)-$(VERSION) -xozf $(NETSNMPTAR)
	if [ -f tmp/$(NAME)-$(VERSION)/$(NETSNMP)/agent/mibgroup/host/data_access/swrun_darwin.c ] ; then \
	     rm -rf tmp/$(NAME)-$(VERSION)/$(NETSNMP)/agent/mibgroup/host/data_access/swrun_darwin.c ;\
         tar -C  tmp/$(NAME)-$(VERSION) -cz $(NETSNMP) > $(NETSNMPTAR) ; \
	fi     
	rm -rf tmp/$(NAME)-$(VERSION)/$(NETSNMP)
	tar -C tmp -cz $(NAME)-$(VERSION) > $@

provides:
	@echo $(OBJS2)

.PHONY: subdirs $(SUBDIRS) all clean install source_tarball


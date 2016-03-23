GNU_ATOMICS = no

ifeq ($(ARCH),i386)
GNU_ATOMICS = yes
LIB_ATOMIC=-latomic
CONFIG_LIBATOMIC=y
endif
ifeq ($(ARCH),mips)
GNU_ATOMICS = yes
LIB_ATOMIC=-latomic
CONFIG_LIBATOMIC=y
endif
squid-configure:
	cd squid && ./configure --target=$(ARCH)-linux --host=$(ARCH)-linux --prefix=/usr --libdir=/usr/lib CFLAGS="$(COPTS) -DNEED_PRINTF -L$(TOP)/openssl -lssl -lcrypto -pthread $(LIB_ATOMIC)" CPPFLAGS="$(COPTS) -DNEED_PRINTF -pthread -L$(TOP)/openssl -lcrypto -lssl $(LIB_ATOMIC)" CXXFLAGS="$(COPTS) -DNEED_PRINTF -pthread -L$(TOP)/openssl  $(LIB_ATOMIC)" \
	CC="$(ARCH)-linux-uclibc-gcc $(COPTS)" \
	ac_cv_header_linux_netfilter_ipv4_h=yes \
	ac_cv_epoll_works=yes \
	squid_cv_gnu_atomics=$(GNU_ATOMICS) \
	--datadir=/usr/lib/squid \
	--libexecdir=/usr/libexec/squid \
	--sysconfdir=/etc/squid \
	--enable-shared \
	--enable-static \
	--enable-x-accelerator-vary \
	--with-pthreads \
	--with-dl \
	--enable-icmp \
	--enable-kill-parent-hack \
	--enable-arp-acl \
	--enable-ssl \
	--enable-htcp \
	--enable-err-languages=English \
	--enable-default-err-language=English \
	--enable-linux-netfilter \
	--enable-icmp \
	--disable-wccp \
	--disable-wccpv2 \
	--disable-snmp \
	--disable-htcp \
	--enable-underscores \
	--enable-cache-digests \
	--enable-referer-log \
	--enable-delay-pools \
	--enable-useragent-log \
	--with-openssl=$(TOP)/openssl \
	--disable-external-acl-helpers \
	--disable-arch-native \
	--enable-auth-negotiate \
	--enable-auth-ntlm \
	--enable-auth-digest \
	--enable-auth-basic="RADIUS" \
	--enable-epoll \
	--with-krb5-config=no \
	--with-maxfd=4096
	
squid:
	make -C squid
#	make -C squid/plugins/squid_radius_auth 

squid-install:
	make  -C squid install DESTDIR=$(INSTALLDIR)/squid	
	rm -rf $(INSTALLDIR)/squid/usr/share
	rm -rf $(INSTALLDIR)/squid/usr/include
	chmod 4755 $(INSTALLDIR)/squid/usr/libexec/squid/*
#	make -C squid/plugins/squid_radius_auth install DESTDIR=$(INSTALLDIR)/squid

squid-clean:
	make -C squid clean
#	make -C squid/plugins/squid_radius_auth clean

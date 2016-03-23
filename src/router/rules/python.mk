python-configure: libffi-configure libffi libffi-install
	cd python && ./configure --host=$(ARCH)-linux --build=$(ARCH) --sysconfdir=/etc \
		--enable-shared \
		--enable-static \
		--prefix=/usr \
		--without-cxx-main \
		--with-system-ffi="$(INSTALLDIR)/libffi/usr" \
		--with-threads \
		--without-ensurepip \
		--enable-ipv6 \
		CONFIG_SITE="$(TOP)/python/site/config.site" \
		OPT="$(COPTS) -I$(TOP)/openssl/include -I$(TOP)/zlib" \
		LDFLAGS="$(COPTS) -L$(TOP)/openssl -L$(TOP)/zlib -L$(TOP)/python" \
		CFLAGS="$(COPTS) -I$(TOP)/openssl/include -I$(TOP)/zlib" \
		CXXFLAGS="$(COPTS) -I$(TOP)/openssl/include -I$(TOP)/zlib" \
		CC="$(ARCH)-linux-uclibc-gcc $(COPTS)" \
		LIBFFI_INCLUDEDIR="$(INSTALLDIR)/libffi/usr/lib/libffi-3.2.1/include"



python-clean:
	make -C python clean

python: libffi libffi-install
	make -C python python Parser/pgen
	make -C python sharedmods

python-install:
	make -C python install DESTDIR=$(INSTALLDIR)/python
	rm -rf $(INSTALLDIR)/python/usr/include
	rm -rf $(INSTALLDIR)/python/usr/share
	rm -f $(INSTALLDIR)/python/usr/lib/python3.4/config-3.4m/*.a
#18M...
	rm -rf $(INSTALLDIR)/python/usr/lib/python3.4/test

	rm -rf $(INSTALLDIR)/libffi/usr/lib/pkgconfig
	rm -rf $(INSTALLDIR)/libffi/usr/lib/libffi-3.2.1
	rm -rf $(INSTALLDIR)/libffi/usr/share
	rm -f $(INSTALLDIR)/libffi/usr/lib/*.a
	rm -f $(INSTALLDIR)/libffi/usr/lib/*.la


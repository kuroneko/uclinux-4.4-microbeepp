#
# Build using configure.
#

all: build/Makefile
	$(MAKE) -C build

build/Makefile: makefile
	-chmod u+x configure
	-rm -rf build
	-mkdir build
	( \
		cd build; \
		CC="$(CC) $(CFLAGS)" \
		LDFLAGS="$(LDFLAGS)" \
		LIBS="$(LDLIBS)" \
		ac_cv_locale_works=yes \
			sh ../configure $(CONFIGURE_OPTS) \
				--enable-dependency-tracking \
				--disable-shared \
			; \
	)

clean:
	rm -rf build

romfs:

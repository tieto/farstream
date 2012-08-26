DISTCHECK_CONFIGURE_FLAGS=--enable-gtk-doc --enable-introspection

SUBDIRS_EXT =

SUBDIRS = 			\
	farstream 		\
	gst $(SUBDIRS_EXT)	\
	transmitters		\
	tests 			\
	docs			\
	common			\
	common-modified		\
	m4			\
	examples
#	pkgconfig

DIST_SUBDIRS = 			\
	farstream		\
	gst $(SUBDIRS_EXT)	\
	transmitters		\
	tests 			\
	docs			\
	common			\
	common-modified		\
	m4			\
	examples
#	pkgconfig

EXTRA_DIST = \
	AUTHORS COPYING NEWS README ChangeLog autogen.sh \
	$(pcinfiles)

ACLOCAL_AMFLAGS = -I m4 -I common/m4

check-valgrind:
	cd tests/check && make check-valgrind

if HAVE_GST_CHECK
check-torture:
	cd tests/check && make torture
else
check-torture:
	true
endif

pcverfiles = farstream-@FS_APIVERSION@.pc \
             farstream-@FS_APIVERSION@-uninstalled.pc
pcinfiles = farstream.pc.in \
            farstream-uninstalled.pc.in

%-@FS_APIVERSION@.pc: %.pc
	cp $< $@

%-@FS_APIVERSION@-uninstalled.pc: %-uninstalled.pc
	cp $< $@

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = farstream-@FS_APIVERSION@.pc

noinst_DATA = farstream-@FS_APIVERSION@-uninstalled.pc

CLEANFILES = $(pcverfiles)

DISTCLEANFILES = $(pcinfiles:.in=)

dist-hook:
	chmod u+w ${distdir}/ChangeLog
	if test -d .git; then git log >${distdir}/ChangeLog; fi

include $(top_srcdir)/common/coverage/lcov.mak

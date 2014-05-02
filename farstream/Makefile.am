libfarstreamincludedir = $(includedir)/farstream-@FS_APIVERSION@/farstream

libfarstreaminclude_HEADERS = \
		fs-candidate.h \
		fs-codec.h \
		fs-participant.h \
		fs-session.h \
		fs-stream.h \
		fs-conference.h \
		fs-transmitter.h \
		fs-stream-transmitter.h \
		fs-plugin.h \
		fs-element-added-notifier.h \
		fs-utils.h \
		fs-rtp.h

nodist_libfarstreaminclude_HEADERS = \
		fs-enumtypes.h


lib_LTLIBRARIES = libfarstream-@FS_APIVERSION@.la

BUILT_SOURCES = \
	$(nodist_libfarstream_@FS_APIVERSION@_la_SOURCES) \
	$(nodist_libfarstreaminclude_HEADERS)

CLEANFILES = $(BUILT_SOURCES)

libfarstream_@FS_APIVERSION@_la_SOURCES = \
		fs-candidate.c \
		fs-codec.c \
		fs-participant.c \
		fs-session.c \
		fs-stream.c \
		fs-conference.c \
		fs-transmitter.c \
		fs-stream-transmitter.c \
		fs-plugin.c \
		fs-element-added-notifier.c \
		fs-utils.c \
		fs-rtp.c \
		fs-private.h

nodist_libfarstream_@FS_APIVERSION@_la_SOURCES = \
		fs-enumtypes.c


libfarstream_@FS_APIVERSION@_la_CFLAGS = \
	$(FS_INTERNAL_CFLAGS) $(FS_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GST_PLUGINS_BASE_CFLAGS) \
	$(GST_BASE_CFLAGS) \
	$(GST_CFLAGS)
libfarstream_@FS_APIVERSION@_la_LIBADD = \
	$(GLIB_LIBS) \
	$(GST_BASE_LIBS) \
	$(GST_LIBS)
libfarstream_@FS_APIVERSION@_la_LDFLAGS = \
	$(FS_LIB_LDFLAGS) \
	$(FS_ALL_LDFLAGS) \
	$(FS_LT_LDFLAGS) 


public_headers = fs-candidate.h \
		fs-codec.h \
		fs-participant.h \
		fs-session.h \
		fs-stream.h \
		fs-conference.h \
		fs-utils.h

glib_enum_headers=$(public_headers)
glib_enum_define=FS
glib_gen_prefix=_fs
glib_gen_basename=fs

include $(top_srcdir)/common-modified/gst-glib-gen.mak

if HAVE_INTROSPECTION
include $(INTROSPECTION_MAKEFILE)
introspection_sources = \
	$(libfarstream_@FS_APIVERSION@_la_SOURCES) \
	$(nodist_libfarstreaminclude_HEADERS) \
	$(libfarstreaminclude_HEADERS)

INTROSPECTION_GIRS = Farstream-@FS_APIVERSION@.gir
Farstream-@FS_APIVERSION@.gir: libfarstream-@FS_APIVERSION@.la
Farstream_@FS_API_VERSION@_gir_NAMESPACE = Farstream
Farstream_@FS_API_VERSION@_gir_VERSION = @FS_APIVERSION@
Farstream_@FS_API_VERSION@_gir_LIBS = libfarstream-@FS_APIVERSION@.la
Farstream_@FS_API_VERSION@_gir_FILES = $(introspection_sources)
Farstream_@FS_API_VERSION@_gir_INCLUDES = GObject-2.0 Gst-@GST_API_VERSION@
Farstream_@FS_API_VERSION@_gir_CFLAGS = $(FS_INTERNAL_CFLAGS)
Farstream_@FS_API_VERSION@_gir_SCANNERFLAGS = --identifier-prefix=fs_ \
	--identifier-prefix=Fs --add-init-section="gst_init(NULL,NULL);"
Farstream_@FS_API_VERSION@_gir_LDFLAGS = $(GST_LIBS)

girdir = $(datadir)/gir-1.0
dist_gir_DATA = Farstream-@FS_APIVERSION@.gir
typelibdir = $(libdir)/girepository-1.0
typelib_DATA = Farstream-@FS_APIVERSION@.typelib
CLEANFILES += $(dist_gir_DATA) $(typelib_DATA)
endif

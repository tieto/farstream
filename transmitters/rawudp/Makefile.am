
plugindir = $(FS_PLUGIN_PATH)

plugin_LTLIBRARIES = librawudp-transmitter.la

# sources used to compile this lib
librawudp_transmitter_la_SOURCES = \
	fs-rawudp-transmitter.c \
	fs-rawudp-stream-transmitter.c \
	fs-rawudp-component.c


# flags used to compile this plugin
librawudp_transmitter_la_CFLAGS = \
	$(FS_INTERNAL_CFLAGS) \
	$(FS_CFLAGS) \
	$(GST_CFLAGS) \
	$(NICE_CFLAGS) \
	$(GUPNP_CFLAGS) \
	$(GIO_CFLAGS)

librawudp_transmitter_la_LDFLAGS = $(FS_PLUGIN_LDFLAGS)
librawudp_transmitter_la_LIBTOOLFLAGS = $(PLUGIN_LIBTOOLFLAGS)
librawudp_transmitter_la_LIBADD = \
	$(top_builddir)/farstream/libfarstream-@FS_APIVERSION@.la \
	$(FS_LIBS) \
	$(GST_LIBS) \
	$(NICE_LIBS) \
	$(GUPNP_LIBS) \
	$(GIO_LIBS) \
	-lgstnet-@GST_API_VERSION@

noinst_HEADERS = \
	fs-rawudp-transmitter.h \
	fs-rawudp-stream-transmitter.h \
	fs-rawudp-component.h

glib_enum_define=FS_RAWUDP
glib_gen_prefix=_fs_rawudp
glib_gen_basename=fs-rawudp

include $(top_srcdir)/common/gst-glib-gen.mak


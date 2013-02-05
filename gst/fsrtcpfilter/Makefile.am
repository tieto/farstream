plugin_LTLIBRARIES = libfsrtcpfilter.la

libfsrtcpfilter_la_SOURCES = fs-rtcp-filter.c

noinst_HEADERS = fs-rtcp-filter.h

libfsrtcpfilter_la_CFLAGS = \
	$(FS_CFLAGS) \
	$(GST_BASE_CFLAGS) \
	$(GST_PLUGINS_BASE_CFLAGS) \
	$(GST_CFLAGS)
libfsrtcpfilter_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libfsrtcpfilter_la_LIBADD = \
	$(FS_LIBS) \
	-lgstrtp-@GST_API_VERSION@ \
	$(GST_BASE_LIBS) \
	$(GST_PLUGINS_BASE_LIBS) \
	$(GST_LIBS)

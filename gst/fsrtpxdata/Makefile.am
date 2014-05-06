plugin_LTLIBRARIES = libfsrtpxdata.la

libfsrtpxdata_la_SOURCES = fsrtpxdata.c fsrtpxdatapay.c fsrtpxdatadepay.c
libfsrtpxdata_la_CFLAGS = \
	$(FS_CFLAGS) \
	$(GST_BASE_CFLAGS) \
	$(GST_CFLAGS)
libfsrtpxdata_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libfsrtpxdata_la_LIBADD = \
	$(FS_LIBS) \
	$(GST_BASE_LIBS) \
	$(GST_LIBS) \
	-lgstrtp-@GST_API_VERSION@

noinst_HEADERS = fsrtpxdatapay.h fsrtpxdatadepay.h

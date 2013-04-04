
if HAVE_GIO_UNIX
SIMPLE_CALL_SHM = simple-call-shm
else
SIMPLE_CALL_SHM =
endif

noinst_PROGRAMS = simple-call $(SIMPLE_CALL_SHM)


AM_CFLAGS = \
	-I$(top_srcdir)/gst/fsrtpconference/ \
	$(FS_INTERNAL_CFLAGS) \
	$(FS_CFLAGS) \
	$(GST_CFLAGS) \
	$(CFLAGS)

LDADD = \
	$(top_builddir)/farstream/libfarstream-@FS_APIVERSION@.la \
	$(GST_LIBS) 


simple_call_shm_CFLAGS = \
	$(AM_CFLAGS) \
	$(GIO_UNIX_CFLAGS)

simple_call_shm_LDADD = \
	$(LDADD) \
	$(GIO_UNIX_LIBS)

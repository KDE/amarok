kde_module_LTLIBRARIES = libamarok_mtp-mediadevice.la
kde_services_DATA = amarok_mtp-mediadevice.desktop

INCLUDES = \
    -I$(top_srcdir)/amarok/src \
    -I$(top_builddir)/amarok/src \
    -I$(top_srcdir)/amarok/src/amarokcore \
    -I$(top_builddir)/amarok/src/amarokcore \
    -I$(top_srcdir)/amarok/src/engine \
    -I$(top_builddir)/amarok/src/engine \
    -I$(top_srcdir)/amarok/src/mediadevice \
    $(TAGLIB_CFLAGS) \
    $(all_includes)

METASOURCES = AUTO

libamarok_mtp_mediadevice_la_LIBADD  = \
    $(top_builddir)/amarok/src/libamarok.la \
    $(LIBMTP_LIBS) 

libamarok_mtp_mediadevice_la_LDFLAGS = \
    $(KDE_PLUGIN) \
    $(all_libraries)

libamarok_mtp_mediadevice_la_SOURCES = \
    mtpmediadevice.cpp

noinst_HEADERS = \
    mtpmediadevice.h


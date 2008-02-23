kde_module_LTLIBRARIES = libamarok_ipod-mediadevice.la
kde_services_DATA = amarok_ipod-mediadevice.desktop

INCLUDES = \
    -I$(top_srcdir)/amarok/src \
    -I$(top_builddir)/amarok/src \
    -I$(top_srcdir)/amarok/src/amarokcore \
    -I$(top_builddir)/amarok/src/amarokcore \
    -I$(top_srcdir)/amarok/src/engine \
    -I$(top_builddir)/amarok/src/engine \
    -I$(top_srcdir)/amarok/src/mediadevice \
    $(LIBGPOD_CFLAGS) \
    $(TAGLIB_CFLAGS) \
    $(all_includes)

METASOURCES = AUTO

libamarok_ipod_mediadevice_la_LIBADD  = \
    $(top_builddir)/amarok/src/libamarok.la

libamarok_ipod_mediadevice_la_LDFLAGS = \
    $(KDE_PLUGIN) \
    $(LIBGPOD_LIBS) \
    $(all_libraries)

libamarok_ipod_mediadevice_la_SOURCES = \
    ipodmediadevice.cpp

noinst_HEADERS = \
    ipodmediadevice.h


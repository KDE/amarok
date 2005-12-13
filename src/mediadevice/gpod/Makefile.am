kde_module_LTLIBRARIES = libamarok_gpod-mediadevice.la
kde_services_DATA = amarok_gpod-mediadevice.desktop

INCLUDES = \
    -I$(top_srcdir)/amarok/src \
    -I$(top_srcdir)/amarok/src/amarokcore \
    -I$(top_builddir)/amarok/src/amarokcore \
    -I$(top_srcdir)/amarok/src/engine \
    -I$(top_builddir)/amarok/src/engine \
    -I$(top_srcdir)/amarok/src/mediadevice \
    $(LIBGPOD_CFLAGS) \
    $(taglib_includes) \
    $(all_includes)

METASOURCES = AUTO

libamarok_gpod_mediadevice_la_LIBADD  = \
    $(top_builddir)/amarok/src/libamarok.la

libamarok_gpod_mediadevice_la_LDFLAGS = \
    $(KDE_PLUGIN) \
    $(LIBGPOD_LIBS) \
    $(all_libraries)

libamarok_gpod_mediadevice_la_SOURCES = \
    kiomediadevice.cpp \
    gpodmediadevice.cpp

noinst_HEADERS = \
    kiomediadevice.h \
    gpodmediadevice.h


kde_module_LTLIBRARIES = libamarok_gpod-mediadevice.la
kde_services_DATA = amarok_gpod-mediadevice.desktop

INCLUDES = \
    -I$(top_srcdir)/amarok/src \
    -I$(top_srcdir)/amarok/src/amarokcore \
    -I$(top_builddir)/amarok/src/amarokcore \
    -I$(top_srcdir)/amarok/src/engine \
    -I$(top_builddir)/amarok/src/engine \
    -I.. \
    $(LIBGPOD_CFLAGS) \
    $(taglib_includes) \
    $(all_includes)

METASOURCES        = AUTO

libamarok_gpod_mediadevice_la_LIBADD  = ../../libamarok.la
libamarok_gpod_mediadevice_la_LDFLAGS  = -module $(KDE_PLUGIN) $(all_libraries) $(LIBGPOD_LIBS)

libamarok_gpod_mediadevice_la_SOURCES =  \
                     gpodmediadevice.cpp

noinst_HEADERS = \
                     gpodmediadevice.h



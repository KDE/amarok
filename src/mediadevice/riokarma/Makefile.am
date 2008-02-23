kde_module_LTLIBRARIES = libamarok_riokarma-mediadevice.la
kde_services_DATA = amarok_riokarma-mediadevice.desktop

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

libamarok_riokarma_mediadevice_la_LIBADD  = \
    $(top_builddir)/amarok/src/libamarok.la \
    -ltag_c -lz -lusb -lkarma

libamarok_riokarma_mediadevice_la_LDFLAGS = \
    $(KDE_PLUGIN) \
    $(all_libraries)

libamarok_riokarma_mediadevice_la_SOURCES = \
    riokarmamediadevice.cpp

noinst_HEADERS = \
    riokarmamediadevice.h


kde_module_LTLIBRARIES = libamarok_ifp-mediadevice.la
kde_services_DATA = amarok_ifp-mediadevice.desktop

INCLUDES = \
    -I$(top_srcdir)/amarok/src \
    -I$(top_srcdir)/amarok/src/amarokcore \
    -I$(top_srcdir)/amarok/src/statusbar \
    -I$(top_srcdir)/amarok/src/engine \
    -I$(top_srcdir)/amarok/src/plugin \
    -I$(top_srcdir)/amarok/src/mediadevice \
    $(IFP_CFLAGS) \
    $(TAGLIB_CFLAGS) \
    $(all_includes)

libamarok_ifp_mediadevice_la_LIBADD = \
    $(top_builddir)/amarok/src/libamarok.la \
    -lusb -lifp \
    -lkdeui -lkdecore -lkio

libamarok_ifp_mediadevice_la_LDFLAGS = \
    $(KDE_PLUGIN) \
    $(all_libraries)

libamarok_ifp_mediadevice_la_SOURCES = \
    ifpmediadevice.cpp

METASOURCES = AUTO

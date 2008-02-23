kde_module_LTLIBRARIES = libamarok_generic-mediadevice.la
kde_services_DATA = amarok_generic-mediadevice.desktop

INCLUDES = \
    -I$(top_srcdir)/amarok/src \
    -I$(top_srcdir)/amarok/src/amarokcore \
    -I$(top_builddir)/amarok/src/amarokcore \
    -I$(top_srcdir)/amarok/src/engine \
    -I$(top_srcdir)/amarok/src/engine \
    -I$(top_srcdir)/amarok/src/mediadevice \
    $(TAGLIB_CFLAGS) \
    $(all_includes)

METASOURCES = AUTO

libamarok_generic_mediadevice_la_LIBADD  = \
    $(top_builddir)/amarok/src/libamarok.la \
    -lkdeui -lkdecore -lkio

libamarok_generic_mediadevice_la_LDFLAGS = \
    $(KDE_PLUGIN) \
    $(all_libraries)

libamarok_generic_mediadevice_la_SOURCES = \
    genericmediadeviceconfigdialog.ui\
    genericmediadevice.cpp

noinst_HEADERS = \
    genericmediadeviceconfigdialog.ui.h\
    genericmediadevice.h


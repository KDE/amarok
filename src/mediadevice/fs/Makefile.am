kde_module_LTLIBRARIES = libamarok_fs-mediadevice.la
kde_services_DATA = amarok_fs-mediadevice.desktop

INCLUDES = \
    -I$(top_srcdir)/amarok/src \
    -I$(top_srcdir)/amarok/src/amarokcore \
    -I$(top_srcdir)/amarok/src/amarokcore \
    -I$(top_srcdir)/amarok/src/engine \
    -I$(top_srcdir)/amarok/src/engine \
    -I$(top_srcdir)/amarok/src/mediadevice \
    $(taglib_includes) \
    $(all_includes)

METASOURCES = AUTO

libamarok_fs_mediadevice_la_LIBADD  = \
    $(top_builddir)/amarok/src/libamarok.la \
    -lkdeui -lkdecore -lkio

libamarok_fs_mediadevice_la_LDFLAGS = \
    $(KDE_PLUGIN) \
    $(all_libraries)

libamarok_fs_mediadevice_la_SOURCES = \
    fsmediadevice.cpp

noinst_HEADERS = \
    fsmediadevice.h

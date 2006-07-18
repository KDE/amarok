kde_module_LTLIBRARIES = libamarok_nfs-device.la
kde_services_DATA = amarok_nfs-device.desktop

INCLUDES = \
    -I$(top_builddir)/amarok/src/amarokcore \
    -I$(top_srcdir)/amarok/src/amarokcore \
    -I$(top_builddir)/amarok/src/amarokcore \
    -I$(top_srcdir)/amarok/src/plugin \
    -I$(top_builddir)/amarok/src \
    -I$(top_srcdir)/amarok/src \
     $(all_includes)

METASOURCES = AUTO

libamarok_nfs_device_la_LIBADD = \
    $(top_builddir)/amarok/src/libamarok.la \
    -lkdecore

libamarok_nfs_device_la_LDFLAGS = \
    $(KDE_PLUGIN) \
    $(all_libraries)







noinst_HEADERS = nfsdevicehandler.h
libamarok_nfs_device_la_SOURCES = nfsdevicehandler.cpp

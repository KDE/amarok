SUBDIRS = \
    gst-plugins

kde_module_LTLIBRARIES = \
    libamarok_gstengine_plugin.la

INCLUDES = \
    -I$(top_srcdir)/amarok/amarok/engine \
    -I$(top_srcdir)/amarok/amarok/plugin \
    -I$(top_srcdir)/amarok/amarok \
    $(CFLAGS_GST) \
    $(all_includes)

libamarok_gstengine_plugin_la_LIBADD = \
    $(top_builddir)/amarok/amarok/engine/libengine.la \
    $(top_builddir)/amarok/amarok/plugin/libplugin.la \
    $(LIB_KDECORE) \
    $(LIB_GST)

libamarok_gstengine_plugin_la_SOURCES = \
    gstengine.cpp

libamarok_gstengine_plugin_la_LDFLAGS = \
    -module \
    -no-undefined \
    $(KDE_PLUGIN) \
    $(all_libraries) 

METASOURCES = \
    AUTO

kde_services_DATA = \
    amarok_gstengine_plugin.desktop


  
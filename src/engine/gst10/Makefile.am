kde_module_LTLIBRARIES = \
    libamarok_gst10engine_plugin.la

SUBDIRS = \
    config 

INCLUDES = \
    -I$(top_srcdir)/amarok/src/engine \
    -I$(top_srcdir)/amarok/src/engine/gst/config \
    -I$(top_srcdir)/amarok/src/plugin \
    -I$(top_srcdir)/amarok/src \
    $(CFLAGS_GST10) \
    $(all_includes)

libamarok_gst10engine_plugin_la_LIBADD = \
    $(top_builddir)/amarok/src/engine/libengine.la \
    $(top_builddir)/amarok/src/engine/gst/config/libgstconfig.la \
    $(top_builddir)/amarok/src/plugin/libplugin.la \
    $(LIB_KFILE) \
    $(LIB_KDECORE)

libamarok_gst10engine_plugin_la_LDFLAGS = \
    $(LIB_GST10) \
    $(KDE_PLUGIN) \
    $(all_libraries)

libamarok_gst10engine_plugin_la_SOURCES = \
    gstconfigdialog.cpp \
    gstconfigdialogbase.ui \
    gstengine.cpp \
    streamsrc.cpp

METASOURCES = \
    AUTO

kde_services_DATA = \
    amarok_gst10engine_plugin.desktop

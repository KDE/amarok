kde_module_LTLIBRARIES = \
    libamarok_gst10engine_plugin.la

SUBDIRS = \
    config equalizer


INCLUDES = \
    -I$(top_srcdir)/amarok/src/engine/gst10/config \
    -I$(top_srcdir)/amarok/src/plugin \
    -I$(top_srcdir)/amarok/src/amarokcore \
    -I$(top_srcdir)/amarok/src \
    $(CFLAGS_GST10) \
    $(all_includes)

AM_CPPFLAGS = \
    -DGST_KIOSTREAMS

libamarok_gst10engine_plugin_la_LIBADD = \
    $(top_builddir)/amarok/src/libamarok.la \
    $(top_builddir)/amarok/src/engine/gst10/config/libgstconfig.la \
    $(top_builddir)/amarok/src/engine/gst10/equalizer/libgstequalizer.la \
    $(top_builddir)/amarok/src/plugin/libplugin.la \
    $(LIB_KFILE) \
    $(LIB_KDECORE)

libamarok_gst10engine_plugin_la_LDFLAGS = \
    $(LIB_GST10) \
    $(KDE_PLUGIN) \
    $(all_libraries)

libamarok_gst10engine_plugin_la_SOURCES = \
    gstconfigdialog.cpp \
    streamprovider.cpp \
    streamsrc.cpp \
    gstconfigdialogbase.ui \
    gstengine.cpp

METASOURCES = \
    AUTO

kde_services_DATA = \
    amarok_gst10engine_plugin.desktop

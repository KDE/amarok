kde_module_LTLIBRARIES = \
    libamarok_gstengine_plugin.la

SUBDIRS = \
    config \
    equalizer

INCLUDES = \
    -I$(top_srcdir)/amarok/src/engine \
    -I$(top_srcdir)/amarok/src/engine/gst/config \
    -I$(top_srcdir)/amarok/src/plugin \
    -I$(top_srcdir)/amarok/src \
    $(CFLAGS_GST) \
    $(all_includes)

libamarok_gstengine_plugin_la_LIBADD = \
    $(top_builddir)/amarok/src/engine/libengine.la \
    $(top_builddir)/amarok/src/engine/gst/config/libgstconfig.la \
    $(top_builddir)/amarok/src/engine/gst/equalizer/libgstequalizer.la \
    $(top_builddir)/amarok/src/plugin/libplugin.la \
    $(LIB_KFILE) \
    $(LIB_KDECORE)

libamarok_gstengine_plugin_la_LDFLAGS = \
    $(LIB_GST) \
    $(KDE_PLUGIN) \
    $(all_libraries)

libamarok_gstengine_plugin_la_SOURCES = \
    adapter.c \
    gstconfigdialog.cpp \
    gstconfigdialogbase.ui \
    gstengine.cpp \
    streamsrc.cpp

METASOURCES = \
    AUTO

kde_services_DATA = \
    amarok_gstengine_plugin.desktop

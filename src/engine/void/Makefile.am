kde_module_LTLIBRARIES = \
    libamarok_void-engine_plugin.la

INCLUDES = \
    -I$(top_srcdir)/amarok/src/plugin \
    -I$(top_srcdir)/amarok/src \
    $(all_includes)

libamarok_void_engine_plugin_la_LIBADD = \
    $(top_builddir)/amarok/src/libamarok.la \
    $(top_builddir)/amarok/src/plugin/libplugin.la \
    $(LIB_KDECORE)

libamarok_void_engine_plugin_la_SOURCES = \
    void-engine.cpp

libamarok_void_engine_plugin_la_LDFLAGS = \
    $(KDE_PLUGIN) \
    $(all_libraries)

METASOURCES = \
    AUTO

kde_services_DATA = \
    amarok_void-engine_plugin.desktop

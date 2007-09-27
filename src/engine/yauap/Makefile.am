kde_module_LTLIBRARIES = \
    libamarok_yauap-engine_plugin.la

INCLUDES = \
    -I$(top_srcdir)/amarok/src/plugin \
    -I$(top_srcdir)/amarok/src \
    $(CFLAGS_YAUAP) \
    $(all_includes)



libamarok_yauap_engine_plugin_la_LIBADD = \
    $(top_builddir)/amarok/src/libamarok.la \
    $(top_builddir)/amarok/src/plugin/libplugin.la \
    $(LIB_YAUAP)\
    $(LIB_KDECORE)


libamarok_yauap_engine_plugin_la_SOURCES = \
    yauap-engine.cpp

libamarok_yauap_engine_plugin_la_LDFLAGS = \
    $(KDE_PLUGIN) \
    $(all_libraries)

METASOURCES = AUTO

kde_services_DATA = \
    amarok_yauap-engine_plugin.desktop

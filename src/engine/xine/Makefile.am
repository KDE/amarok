kde_module_LTLIBRARIES = libamarok_xine-engine.la
kde_services_DATA = amarok_xine-engine.desktop amarok_xineengine_plugin.desktop

INCLUDES = -I$(top_srcdir)/amarok/src $(CFLAGS_XINE) $(all_includes)

libamarok_xine_engine_la_LIBADD = \
    $(top_builddir)/amarok/src/engine/libengine.la \
    $(top_builddir)/amarok/src/plugin/libplugin.la \
    $(LIB_KDECORE) $(LIB_KDEUI) \
    $(LIBS_XINE)

libamarok_xine_engine_la_SOURCES = xine-scope.c xine-engine.cpp xine-config.cpp
libamarok_xine_engine_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)

METASOURCES  = AUTO

kde_module_LTLIBRARIES = libamarok_aKode-engine.la
kde_services_DATA = amarok_aKode-engine.desktop

INCLUDES = -I$(top_srcdir)/amarok/src $(CFLAGS_AKODE) $(all_includes)

libamarok_aKode_engine_la_LIBADD = \
    $(top_builddir)/amarok/src/libamarok.la \
    $(top_builddir)/amarok/src/plugin/libplugin.la \
    -lkdecore \
    $(LIBS_AKODE)

libamarok_aKode_engine_la_SOURCES = akode-engine.cpp akode-scope.cpp
libamarok_aKode_engine_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)

METASOURCES  = AUTO

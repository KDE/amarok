kde_module_LTLIBRARIES = libamarok_aKode-engine.la
kde_services_DATA = amarok_aKode-engine.desktop

INCLUDES = -I$(top_srcdir)/amarok/src $(all_includes)

libamarok_aKode_engine_la_LIBADD = \
    $(top_builddir)/amarok/src/engine/libengine.la \
    $(top_builddir)/amarok/src/plugin/libplugin.la \
    -lkdecore -lakode

libamarok_aKode_engine_la_SOURCES = akode-engine.cpp
libamarok_aKode_engine_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)

METASOURCES  = AUTO

kde_module_LTLIBRARIES = libamarok_xine-engine.la
kde_services_DATA = amarok_xine-engine.desktop

INCLUDES = -I$(top_srcdir)/amarok/src $(all_includes)

libamarok_xine_engine_la_LIBADD = \
    $(top_builddir)/amarok/src/engine/libengine.la \
    $(top_builddir)/amarok/src/plugin/libplugin.la \
    $(LIB_KDECORE) $(LIB_KDEUI) \
    -lxine -lz -lnsl -lpthread

libamarok_xine_engine_la_SOURCES = xine-engine.cpp
libamarok_xine_engine_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)

METASOURCES  = AUTO
KDE_CXXFLAGS = $(USE_EXCEPTIONS)

kde_module_LTLIBRARIES = \
    libamarok_artsengine_plugin.la

INCLUDES = \
    -I$(top_builddir)/amarok/amarok/amarokarts \
    -I$(top_srcdir)/amarok/amarok/engine \
    -I$(top_srcdir)/amarok/amarok/plugin \
    -I$(top_srcdir)/amarok/amarok \
    -I$(kde_includes)/arts \
    $(all_includes)

libamarok_artsengine_plugin_la_LIBADD = \
    $(top_builddir)/amarok/amarok/amarokarts/libamarokarts.la \
    $(top_builddir)/amarok/amarok/engine/libengine.a \
    $(top_builddir)/amarok/amarok/plugin/libplugin.a \
    $(LIB_KFILE) $(LIB_KDEUI) $(LIB_KDECORE) \
    $(arts_libs)

libamarok_artsengine_plugin_la_SOURCES = \
    artsengine.cpp

libamarok_artsengine_plugin_la_LDFLAGS = \
    -module \
    -no-undefined \
    $(KDE_PLUGIN) \
    $(all_libraries) 

METASOURCES = \
    AUTO

kde_services_DATA = \
    amarok_artsengine_plugin.desktop



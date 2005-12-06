kde_module_LTLIBRARIES = \
    libamarok_artsengine_plugin.la

SUBDIRS = \
    amarokarts
    
INCLUDES = \
    -I$(top_builddir)/amarok/src/engine/arts/amarokarts \
    -I$(top_srcdir)/amarok/src/engine \
    -I$(top_srcdir)/amarok/src/plugin \
    -I$(top_srcdir)/amarok/src \
    -I$(kde_includes)/arts \
    $(all_includes)

libamarok_artsengine_plugin_la_LIBADD = \
    $(top_builddir)/amarok/src/engine/arts/amarokarts/libamarokarts.la \
    $(top_builddir)/amarok/src/engine/libengine.la \
    $(top_builddir)/amarok/src/plugin/libplugin.la \
    $(LIB_KFILE) $(LIB_KDEUI) $(LIB_KDECORE) \
    $(arts_libs)

libamarok_artsengine_plugin_la_SOURCES = \
    artseffects.cpp \
    artsengine.cpp

libamarok_artsengine_plugin_la_LDFLAGS = \
    -module \
    -no-undefined \
    $(KDE_PLUGIN) \
    $(all_libraries) 

METASOURCES = \
    AUTO

artsengine.lo: amarokarts/amarokarts.h

kde_services_DATA = \
    amarok_artsengine_plugin.desktop



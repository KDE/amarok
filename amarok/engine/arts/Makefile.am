INCLUDES = \
    -I$(top_builddir)/amarok/amarok/amarokarts \
    -I$(top_srcdir)/amarok/amarok/engine \
    -I$(kde_includes)/arts \
    $(all_includes)

artsengine_la_LIBADD = \
    $(top_builddir)/amarok/amarok/amarokarts/libamarokarts.la \
    $(top_builddir)/amarok/amarok/engine/libengine.a \
    $(arts_libs)

kde_module_LTLIBRARIES = artsengine.la

artsengine_la_SOURCES = artsengine.cpp

artsengine_la_LDFLAGS = $(all_libraries) -module -avoid-version -no-undefined

METASOURCES = AUTO

plugin_DATA = artsengine.plugin
plugindir = $(kde_datadir)/amarok


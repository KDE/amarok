kde_module_LTLIBRARIES = \
    libamarok_nmmengine_plugin.la

INCLUDES = \
    -I$(top_srcdir)/amarok/src \
    $(CFLAGS_NMM) \
    $(all_includes)

libamarok_nmmengine_plugin_la_LIBADD = \
    $(top_builddir)/amarok/src/engine/libengine.la \
    $(top_builddir)/amarok/src/plugin/libplugin.la \
    $(LIB_KDECORE) $(LIB_KFILE) \
    -lnmmbase -lnmmgraphmgr -lnmmiprogress -lnmmiaudiodevice -lnmmigeneral

libamarok_nmmengine_plugin_la_SOURCES = \
    nmm_engine.cpp

libamarok_nmmengine_plugin_la_LDFLAGS = \
    -module \
    $(KDE_PLUGIN) \
    $(LDFLAGS_NMM) \
    $(all_libraries)

METASOURCES = \
    AUTO

KDE_CXXFLAGS = $(USE_EXCEPTIONS)

kde_services_DATA = \
    amarok_nmmengine_plugin.desktop

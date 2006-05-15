kde_module_LTLIBRARIES = libamarok_kdemmengine_plugin.la
kde_services_DATA = amarok_kdemmengine_plugin.desktop

INCLUDES = \
    -I$(top_srcdir)/amarok/src/engine \
    -I$(top_srcdir)/amarok/src/plugin \
    -I$(top_srcdir)/amarok/src \
    $(CFLAGS_MAS) $(all_includes)

libamarok_kdemmengine_plugin_la_LIBADD = \
    $(top_builddir)/amarok/src/libamarok.la \
    $(top_builddir)/amarok/src/plugin/libplugin.la \
    $(LIB_KFILE) $(LIB_KDEUI) $(LIB_KDECORE) \
    $(LIBS_KDEMM) -lkdemm

libamarok_kdemmengine_plugin_la_SOURCES = \
    kdemmengine.cpp

libamarok_kdemmengine_plugin_la_LDFLAGS = \
    -module \
    -no-undefined \
    $(KDE_PLUGIN) \
    $(all_libraries)

METASOURCES = AUTO




kde_module_LTLIBRARIES = libamarok_masengine_plugin.la
kde_services_DATA = amarok_masengine_plugin.desktop

INCLUDES = \
    -I$(top_srcdir)/amarok/src/engine \
    -I$(top_srcdir)/amarok/src/plugin \
    -I$(top_srcdir)/amarok/src \
    -I/usr/local/mas/include \
    $(all_includes)

libamarok_masengine_plugin_la_LIBADD = \
    $(top_builddir)/amarok/src/engine/libengine.la \
    $(top_builddir)/amarok/src/plugin/libplugin.la \
    $(LIB_KFILE) $(LIB_KDEUI) $(LIB_KDECORE) \
    -L/usr/local/mas/lib -lmasc -lmas $(mas_libs)

libamarok_masengine_plugin_la_SOURCES = \
    masengine.cpp

libamarok_masengine_plugin_la_LDFLAGS = \
    -module \
    -no-undefined \
    $(KDE_PLUGIN) \
    $(all_libraries) 

METASOURCES = AUTO




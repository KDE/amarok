kde_module_LTLIBRARIES = \
    libamarok_sqlite_dbengine_plugin.la

SUBDIRS = \
    sqlite

INCLUDES = \
    -I$(top_srcdir)/amarok/src/database/sqlite/sqlite \
    -I$(top_srcdir)/amarok/src/database \
    -I$(top_srcdir)/amarok/src/plugin \
    -I$(top_srcdir)/amarok/src/engine \
    -I$(top_srcdir)/amarok/src/amarokcore \
    -I$(top_srcdir)/amarok/src/statusbar \
    -I$(top_srcdir)/amarok/src \
    $(all_includes) $(taglib_includes)

libamarok_sqlite_dbengine_plugin_la_LIBADD = \
    $(top_builddir)/amarok/src/database/sqlite/sqlite/libsqlite.la \
    $(top_builddir)/amarok/src/database/libdbengine.la \
    $(top_builddir)/amarok/src/plugin/libplugin.la \
    $(LIB_KDECORE)

libamarok_sqlite_dbengine_plugin_la_SOURCES = \
    sqlite_dbengine.cpp

libamarok_sqlite_dbengine_plugin_la_LDFLAGS = \
    -module \
    -no-undefined \
    $(KDE_PLUGIN) \
    $(all_libraries) 

METASOURCES = \
    AUTO

kde_services_DATA = \
    amarok_sqlite_dbengine_plugin.desktop



kde_module_LTLIBRARIES = libamarok_xine-engine.la
kde_services_DATA = amarok_xine-engine.desktop

INCLUDES = \
    -I$(top_srcdir)/amarok/src \
    $(CFLAGS_XINE) \
    $(all_includes)

libamarok_xine_engine_la_LIBADD = \
    $(top_builddir)/amarok/src/engine/libengine.la \
    $(top_builddir)/amarok/src/plugin/libplugin.la \
    -lkdeui -lkdecore \
    $(LIBS_XINE)

libamarok_xine_engine_la_SOURCES = \
    xine-scope.c \
    xinecfg.kcfgc \
    xine-engine.cpp \
    xineconfigbase.ui \
    xine-config.cpp

libamarok_xine_engine_la_LDFLAGS = \
    $(KDE_PLUGIN) \
    $(all_libraries)

kde_kcfg_DATA = \
    xinecfg.kcfg

METASOURCES  = AUTO

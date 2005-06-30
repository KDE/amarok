kde_module_LTLIBRARIES = \
    libamarok_helixengine_plugin.la

SUBDIRS = \
    helix-sp

AM_CPPFLAGS = \
    -D_UNIX \
    -Wall -Wreturn-type \
    -fno-exceptions \
    -I$(top_srcdir)/amarok/src/engine \
    -I$(top_srcdir)/amarok/src/ \
    -I$(top_srcdir)/amarok/src/engine/helix/helix-sp \
    -I$(top_srcdir)/amarok/src/engine/helix/helix-sp/helix-include/runtime \
    -I$(top_srcdir)/amarok/src/engine/helix/helix-sp/helix-include/common/include \
    -I$(top_srcdir)/amarok/src/engine/helix/helix-sp/helix-include/client/include \
    -I$(top_srcdir)/amarok/src/engine/helix/helix-sp/helix-include/common/container \
    -I$(top_srcdir)/amarok/src/engine/helix/helix-sp/helix-include/common/system \
    -I$(top_srcdir)/amarok/src/engine/helix/helix-sp/helix-include/common/dbgtool \
    -I$(top_srcdir)/amarok/src/engine/helix/helix-sp/helix-include/common/util \
    -include $(top_srcdir)/amarok/src/engine/helix/helix-sp/helixdefines.h \
    $(all_includes)

libamarok_helixengine_plugin_la_SOURCES = \
    helix-engine.cpp \
    helix-config.cpp

libamarok_helixengine_plugin_la_LIBADD = \
    $(top_builddir)/amarok/src/engine/helix/helix-sp/libhelix-sp.la \
    $(top_builddir)/amarok/src/engine/libengine.la \
    $(top_builddir)/amarok/src/plugin/libplugin.la \
    -lkdeui -lkdecore

libamarok_helixengine_plugin_la_LDFLAGS = \
     -module \
    $(KDE_PLUGIN) \
    $(all_libraries)

METASOURCES = \
    AUTO

kde_services_DATA = \
    amarok_helixengine_plugin.desktop

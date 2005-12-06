kde_module_LTLIBRARIES = \
    libamarok_helixengine_plugin.la

SUBDIRS = \
    helix-sp \
    config

AM_CPPFLAGS = \
    -D_UNIX \
    -Wall -Wreturn-type \
    -fno-exceptions \
    -I$(top_srcdir)/amarok/src/engine/helix/helix-sp \
    -I$(top_srcdir)/amarok/src/engine/helix/config \
    -I$(top_srcdir)/amarok/src/ \
    -include $(top_srcdir)/amarok/src/engine/helix/helix-sp/helixdefines.h \
    $(all_includes)

libamarok_helixengine_plugin_la_SOURCES = \
    helix-engine.cpp \
    helix-configdialog.cpp

# remove first 3 ar libs before committing!!:
#    $(top_builddir)/amarok/src/engine/helix/logutillib.a
#    $(top_builddir)/amarok/src/engine/helix/debuglib.a
#    $(top_builddir)/amarok/src/engine/helix/runtlib.a

libamarok_helixengine_plugin_la_LIBADD = \
    $(top_builddir)/amarok/src/engine/helix/helix-sp/libhelix-sp.la \
    $(top_builddir)/amarok/src/engine/helix/config/libhelixconfig.la \
    $(top_builddir)/amarok/src/engine/libengine.la \
    $(top_builddir)/amarok/src/plugin/libplugin.la \
    -lkdeui -lkdecore $(ALSALIB_LIBS)

libamarok_helixengine_plugin_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)

METASOURCES = \
    AUTO

kde_services_DATA = \
    amarok_helixengine_plugin.desktop

helix-configdialog.lo: config/helixconfig.h

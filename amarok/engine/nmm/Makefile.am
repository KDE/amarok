kde_module_LTLIBRARIES = \
    libamarok_nmmengine_plugin.la

INCLUDES = \
    -I$(top_srcdir)/amarok/amarok/engine \
    -I$(top_srcdir)/amarok/amarok/plugin \
    -I$(top_srcdir)/amarok/amarok \
    $(CFLAGS_NMM) \
    $(all_includes)

libamarok_nmmengine_plugin_la_LIBADD = \
    $(top_builddir)/amarok/amarok/engine/libengine.a \
    $(top_builddir)/amarok/amarok/plugin/libplugin.a \
    $(LIB_KUTILS) $(LIB_KFILE) $(LIB_KDEUI) $(LIB_KDECORE) \
    -lnmmbase -lnmmfile -lnmmaudio -lnmmmpegread -lnmmmpegaudiodec -lnmmoggvorbis

libamarok_nmmengine_plugin_la_SOURCES = \
    nmm_engine.cpp

libamarok_nmmengine_plugin_la_LDFLAGS = \
    -module \
    $(KDE_PLUGIN) \
    $(all_libraries) 

METASOURCES = \
    AUTO

kde_services_DATA = \
    amarok_nmmengine_plugin.desktop


    
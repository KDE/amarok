INCLUDES = \
    -I$(top_srcdir)/amarok/amarok/engine \
    -I$(top_srcdir)/amarok/amarok \
    $(CFLAGS_NMM) \
    $(all_includes)

nmm_engine_la_LIBADD = \
    $(top_builddir)/amarok/amarok/engine/libengine.a \
    $(LIB_KDECORE) \
    -lnmmbase -lnmmfile -lnmmaudio -lnmmmpegread -lnmmmpegaudiodec -lnmmoggvorbis

KDE_CXXFLAGS = $(USE_EXCEPTIONS)

kde_module_LTLIBRARIES = nmm_engine.la

nmm_engine_la_SOURCES = nmm_engine.cpp
nmm_engine_la_LDFLAGS = $(all_libraries) -module -avoid-version -no-undefined

METASOURCES = AUTO

plugin_DATA = nmm_engine.plugin
plugindir = $(kde_datadir)/amarok

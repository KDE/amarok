INCLUDES = \
    -I$(top_builddir)/amarok/src \
    -I$(top_builddir)/amarok/src/amarokcore \
    -I$(top_srcdir)/amarok/src \
    -I$(top_srcdir)/amarok/src/engine \
    -I$(top_srcdir)/amarok/src/plugin \
    -I$(top_srcdir)/amarok/src/statusbar \
    -I$(top_srcdir)/amarok/src/mediadevice \
    $(all_includes)

noinst_LTLIBRARIES = \
    libamarokcore.la

noinst_HEADERS = \
    amarokconfig.h \
    amarokdcophandler.h

libamarokcore_la_SOURCES = \
    amarokdcopiface.skel \
    amarokdcophandler.cpp \
    amarokconfig.kcfgc \
    crashhandler.cpp

METASOURCES = \
    AUTO

kde_kcfg_DATA = \
    amarok.kcfg

INCLUDES = \
    -I$(top_builddir)/amarok/src \
    -I$(top_builddir)/amarok/src/amarokcore \
    -I$(top_srcdir)/amarok/src \
    -I$(top_srcdir)/amarok/src/engine \
    -I$(top_srcdir)/amarok/src/plugin \
    $(all_includes)

noinst_LTLIBRARIES = \
    libamarokcore.la

noinst_HEADERS = \
    amarokconfig.h \
    amarokdcophandler.h

libamarokcore_la_SOURCES = \
    main.cpp \
    amarokdcopiface.skel \
    amarokdcophandler.cpp \
    amarokconfig.kcfgc

METASOURCES = \
    AUTO

kde_kcfg_DATA = \
    amarok.kcfg


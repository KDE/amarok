INCLUDES = \
    -I$(top_srcdir)/amarok/amarok \
    -I$(top_srcdir)/amarok/amarok/engine \
    -I$(top_srcdir)/amarok/amarok/plugin \
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


noinst_LTLIBRARIES = libmediadevice.la

INCLUDES = \
    -I$(top_srcdir)/amarok/src/plugin \
    -I$(top_srcdir)/amarok/src \
    -I$(top_srcdir)/amarok/src/engine \
    -I$(top_srcdir)/amarok/src/amarokcore \
    -I$(top_srcdir)/amarok/src/statusbar \
    $(LIBGPOD_CFLAGS) \
    $(IFP_CFLAGS) \
    $(taglib_includes) \
    $(all_includes)

if with_libgpod
    IPOD_SUBDIR = gpod
endif

if with_ifp
    IFP_SUBDIR = ifp
endif

libmediadevice_la_SOURCES = \
    mediabrowser.cpp \
    kiomediadevice.cpp

noinst_HEADERS = mediabrowser.h

METASOURCES = AUTO

SUBDIRS = \
    . \
    $(IPOD_SUBDIR) \
    $(IFP_SUBDIR)


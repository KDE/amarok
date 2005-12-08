INCLUDES = \
    -I$(top_srcdir)/amarok/src \
    -I$(top_srcdir)/amarok/src/amarokcore \
    -I$(top_builddir)/amarok/src/amarokcore \
    -I$(top_srcdir)/amarok/src/engine \
    -I$(top_builddir)/amarok/src/engine \
    -I.. \
    $(LIBGPOD_CFLAGS) \
    $(taglib_includes) \
    $(all_includes)

METASOURCES        = AUTO

libgpodmediadevice_la_LDFLAGS  = $(all_libraries)

noinst_LTLIBRARIES = libgpodmediadevice.la

libgpodmediadevice_la_SOURCES =  \
                     gpodmediadevice.cpp

noinst_HEADERS = \
                     gpodmediadevice.h

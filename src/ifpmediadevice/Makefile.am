INCLUDES = \
    -I$(top_srcdir)/amarok/src \
    -I$(top_srcdir)/amarok/src/amarokcore \
    -I$(top_builddir)/amarok/src/amarokcore \
    -I$(top_srcdir)/amarok/src/engine \
    -I$(top_builddir)/amarok/src/engine \
    -I.. \
    $(IFP_CFLAGS) \
    $(taglib_includes) \
    $(all_includes)
    
METASOURCES        = AUTO

libifpmediadevice_la_LIBADD = -lusb -lifp

libifpmediadevice_la_LDFLAGS  = $(all_libraries)

noinst_LTLIBRARIES = libifpmediadevice.la

libifpmediadevice_la_SOURCES =  \
                     ifpmediadevice.cpp

noinst_HEADERS = \
                     ifpmediadevice.h
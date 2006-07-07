noinst_LTLIBRARIES = libdaapreader.la

INCLUDES = \
        -I$(top_srcdir)/amarok/src \
        $(all_includes)

libdaapreader_la_SOURCES = reader.cpp

libdaapreader_la_LIBADD = \
        $(LIB_QT) \
        $(LIB_KDECORE) \
        $(LIB_KDEUI) \
        $(top_builddir)/amarok/src/mediadevice/daap/daapreader/authentication/libauthentication.la

libdaapreader_la_LDFLAGS = $(all_libraries) $(KDE_RPATH)

METASOURCES = AUTO

noinst_HEADERS = reader.h

SUBDIRS = authentication

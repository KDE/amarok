kde_module_LTLIBRARIES = libamarok_daap-mediadevice.la
kde_services_DATA = amarok_daap-mediadevice.desktop

INCLUDES = \
    -I$(top_srcdir)/amarok/src \
    -I$(top_builddir)/amarok/src/amarokcore \
    -I$(top_builddir)/amarok/src \
    -I$(top_srcdir)/amarok/src/amarokcore \
    -I$(top_srcdir)/amarok/src/engine \
    -I$(top_srcdir)/amarok/src/engine \
    -I$(top_srcdir)/amarok/src/mediadevice \
    $(TAGLIB_CFLAGS) \
    $(all_includes)

METASOURCES = AUTO

libamarok_daap_mediadevice_la_LIBADD  = \
    $(top_builddir)/amarok/src/libamarok.la \
    $(top_builddir)/amarok/src/mediadevice/daap/daapreader/libdaapreader.la \
    $(DNSSD_LIBS) ${LIB_KDEUI} ${LIB_KDECORE} ${LIB_KIO}

libamarok_daap_mediadevice_la_LDFLAGS = \
    $(KDE_PLUGIN) \
    $(all_libraries)

libamarok_daap_mediadevice_la_SOURCES = \
    addhostbase.ui \
    daapclient.cpp \
    daapserver.cpp \
    proxy.cpp

noinst_HEADERS = \
    daapclient.h \
    daapserver.h \
    proxy.h

bin_SCRIPTS = amarok_daapserver.rb

SUBDIRS = daapreader mongrel

amarokrubylibdir = \
    $(kde_datadir)/amarok/ruby_lib

amarokrubylib_DATA = \
    codes.rb

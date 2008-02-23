kde_services_DATA = amarok_njb-mediadevice.desktop


INCLUDES = \
	-I$(top_srcdir)/amarok/src \
	-I$(top_builddir)/amarok/src \
	-I$(top_srcdir)/amarok/src/amarokcore \
	-I$(top_builddir)/amarok/src/amarokcore \
	-I$(top_srcdir)/amarok/src/engine \
	-I$(top_builddir)/amarok/src/engine \
	-I$(top_srcdir)/amarok/src/mediadevice \
	$(TAGLIB_CFLAGS) \
	$(all_includes)
 
METASOURCES = AUTO
kde_module_LTLIBRARIES = libamarok_njb-mediadevice.la

libamarok_njb_mediadevice_la_LIBADD = $(top_builddir)/amarok/src/libamarok.la \
	-lkdeui -lkdecore -lnjb

libamarok_njb_mediadevice_la_LDFLAGS = $(KDE_PLUGIN) $(all_libraries)

noinst_HEADERS = njbmediadevice.h playlist.h track.h

libamarok_njb_mediadevice_la_SOURCES = njbmediadevice.cpp playlist.cpp \
	track.cpp

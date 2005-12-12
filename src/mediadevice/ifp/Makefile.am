kde_module_LTLIBRARIES = libamarok_ifp-mediadevice.la
kde_services_DATA = amarok_ifp-mediadevice.desktop

INCLUDES = \
    -I$(top_srcdir)/amarok/src \
    -I$(top_srcdir)/amarok/src/amarokcore \
    -I$(top_srcdir)/amarok/src/statusbar \
    -I$(top_srcdir)/amarok/src/engine \
	-I$(top_srcdir)/amarok/src/plugin \
    -I.. \
    $(IFP_CFLAGS) \
    $(taglib_includes) \
    $(all_includes)
    
METASOURCES        = AUTO

libamarok_ifp_mediadevice_la_LIBADD = \
 	$(top_builddir)/amarok/src/plugin/libplugin.la \
	$(top_builddir)/amarok/src/engine/libengine.la \
	-lusb -lifp \
	-lkdeui -lkdecore -lkio
	
libamarok_ifp_mediadevice_la_LDFLAGS = -module $(all_libraries)

libamarok_ifp_mediadevice_la_SOURCES = ifpmediadevice.cpp


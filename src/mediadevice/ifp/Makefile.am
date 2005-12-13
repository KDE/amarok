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
	-lusb -lifp \
        ../../libamarok.la \
	-lkdeui -lkdecore -lkio
	
libamarok_ifp_mediadevice_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)

libamarok_ifp_mediadevice_la_SOURCES = ifpmediadevice.cpp


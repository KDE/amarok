SUBDIRS = images amarokarts analyzers titleproxy data

INCLUDES = $(all_includes) -I$(kde_includes)/arts $(taglib_includes)
METASOURCES = AUTO

EXTRA_DIST = browserwidget.h browserwin.h \
	effectwidget.h expandbutton.h \
	Options1.ui Options2.ui Options3.ui Options4.ui playerapp.h \
	playerwidget.h playlistitem.h \
	playlistwidget.h

bin_PROGRAMS = amarok

amarok_SOURCES = main.cpp playerapp.cpp playerwidget.cpp browserwin.cpp browserwidget.cpp playlistwidget.cpp playlistitem.cpp expandbutton.cpp effectwidget.cpp amarokdcopiface.skel configdlg.cpp amarokbutton.cpp amarokfilelist.cpp streambrowser.cpp Options1.ui Options2.ui Options3.ui Options4.ui playlistloader.cpp

amarok_LDADD = ./titleproxy/libtitleproxy.a ./analyzers/libanalyzers.a ./amarokarts/libamarokarts.la -lqtmcop -lkmedia2_idl -lartsflow -lsoundserver_idl -lartskde -lartsgui -lartsgui_kde -lartsmodules $(LIB_KFILE) $(LIB_KDEUI) $(LIB_KDECORE) $(taglib_libs)

# $(LIBSOCKET)
amarok_LDFLAGS = $(all_libraries) $(KDE_RPATH)

noinst_HEADERS = Options1.h Options2.h Options3.h Options4.h browserwidget.h browserwin.h effectwidget.h expandbutton.h playerapp.h playerwidget.h playlistitem.h playlistwidget.h configdlg.h amarokbutton.h amarokfilelist.h amarokbutton.h amarokdcopiface.h amarokfilelist.h browserwidget.h browserwin.h configdlg.h debugareas.h effectwidget.h expandbutton.h playerapp.h playerwidget.h playlistitem.h playlistwidget.h amarokbutton.h amarokdcopiface.h amarokfilelist.h browserwidget.h browserwin.h configdlg.h debugareas.h effectwidget.h expandbutton.h playerapp.h playerwidget.h playlistitem.h playlistwidget.h playlistloader.h

KDE_ICON = AUTO

rcdir = $(kde_datadir)/amarok
rc_DATA = amarokui.rc

configdir = $(kde_confdir)
config_DATA = amarokrc

applnk_DATA =  amarok.desktop
applnkdir = $(kde_appsdir)/Multimedia

messages: rc.cpp
	LIST=`find . -name \*.h -o -name \*.hh -o -name \*.H -o -name \*.hxx -o -name \*.hpp -o -name \*.cpp -o -name \*.cc -o -name \*.cxx -o -name \*.ecpp -o -name \*.C`; \
	if test -n "$$LIST"; then \
	 $(XGETTEXT) $$LIST -o $(podir)/amarok.pot; \
	fi

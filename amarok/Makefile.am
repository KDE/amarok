SUBDIRS = images amarokarts analyzers titleproxy data engine

INCLUDES = -I$(kde_includes)/arts $(taglib_includes) -I$(top_builddir)/amarok/amarok/ui $(all_includes)

bin_PROGRAMS = amarok

amarok_SOURCES = main.cpp browserwin.cpp streambrowser.cpp playlistwidget.cpp playlistitem.cpp playlistloader.cpp expandbutton.cpp effectwidget.cpp amarokdcopiface.skel amarokdcophandler.cpp amarokbutton.cpp amarokfilelist.cpp amarokconfig.kcfgc ui/Options1.ui ui/Options2.ui ui/Options3.ui ui/Options4.ui ui/Options5.ui amarokslider.cpp amaroksystray.cpp osd.cpp filebrowser.cpp kactionselector.cpp  kbookmarkhandler.cpp fht.cpp playerapp.cpp playerwidget.cpp

amarok_LDADD = ./engine/libengine.a ./titleproxy/libtitleproxy.a ./analyzers/libanalyzers.a ./amarokarts/libamarokarts.la -lqtmcop -lkmedia2_idl -lartsflow -lsoundserver_idl -lartskde -lartsgui -lartsgui_kde -lartsmodules -lGL $(LIB_KUTILS) $(LIB_KFILE) $(LIB_KDEUI) $(LIB_KDECORE) $(taglib_libs) $(LIB_GST)

# $(LIBSOCKET)
amarok_LDFLAGS = $(all_libraries) $(KDE_RPATH)

METASOURCES = AUTO

KDE_ICON = AUTO

rcdir = $(kde_datadir)/amarok
rc_DATA = amarokui.rc

kde_kcfg_DATA = amarok.kcfg

configdir = $(kde_confdir)
config_DATA = amarokrc

# FIXME KDE 3.2 uses the new freedesktop.org standard. we must replace the applnk target with:
#       xdg_apps_DATA = amarok.desktop
applnk_DATA =  amarok.desktop
applnkdir = $(kde_appsdir)/Multimedia

messages: rc.cpp
	LIST=`find . -name \*.h -o -name \*.hh -o -name \*.H -o -name \*.hxx -o -name \*.hpp -o -name \*.cpp -o -name \*.cc -o -name \*.cxx -o -name \*.ecpp -o -name \*.C`; \
	if test -n "$$LIST"; then \
	 $(XGETTEXT) $$LIST -o $(podir)/amarok.pot; \
	fi

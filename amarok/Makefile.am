SUBDIRS = images amarokarts data

INCLUDES= $(all_includes) -I$(kde_includes)/taglib -I$(kde_includes)/arts
METASOURCES = AUTO

EXTRA_DIST = browserwidget.h browserwin.h \
	effectwidget.h expandbutton.h \
	Options1.ui Options2.ui Options3.ui playerapp.h \
	playerwidget.h playlistitem.h \
	playlistwidget.h viswidget.h

bin_PROGRAMS = amarok

amarok_SOURCES = main.cpp viswidget.cpp playlistwidget.cpp 	playlistitem.cpp playerwidget.cpp playerapp.cpp 	Options1.ui Options2.ui Options3.ui expandbutton.cpp effectwidget.cpp 	browserwin.cpp browserwidget.cpp amarokdcopiface.skel configdlg.cpp
amarok_LDADD = ./amarokarts/libamarokarts.la -lqtmcop -lkmedia2_idl \
	-lartsflow -lsoundserver_idl -lartskde -lartsgui -lartsgui_kde \
	-lartsmodules -ltag $(LIB_KFILE) $(LIB_KDEUI) $(LIB_KDECORE)
# $(LIBSOCKET)
amarok_LDFLAGS = $(all_libraries) $(KDE_RPATH)

noinst_HEADERS = Options1.h Options2.h Options3.h browserwidget.h browserwin.h 	effectwidget.h expandbutton.h playerapp.h 	playerwidget.h playlistitem.h playlistwidget.h	viswidget.h configdlg.h

install-data-local:
	$(mkinstalldirs) $(kde_icondir)/locolor/32x32/apps/
	$(INSTALL_DATA) $(srcdir)/lo32-app-amarok.png $(kde_icondir)/locolor/32x32/apps/amarok.png
	$(mkinstalldirs) $(kde_icondir)/locolor/16x16/apps/
	$(INSTALL_DATA) $(srcdir)/lo16-app-amarok.png $(kde_icondir)/locolor/16x16/apps/amarok.png
	$(mkinstalldirs) $(kde_icondir)/default.kde/48x48/apps/
	$(INSTALL_DATA) $(srcdir)/lo48-app-amarok.png $(kde_icondir)/default.kde/48x48/apps/amarok.png
	$(mkinstalldirs) $(kde_icondir)/default.kde/64x64/apps/
	$(INSTALL_DATA) $(srcdir)/lo64-app-amarok.png $(kde_icondir)/default.kde/64x64/apps/amarok.png

uninstall-local:
	-rm -f $(kde_icondir)/default.kde/64x64/apps/amarok.png
	-rm -f $(kde_icondir)/default.kde/48x48/apps/amarok.png
	-rm -f $(kde_icondir)/locolor/32x32/apps/amarok.png
	-rm -f $(kde_icondir)/locolor/16x16/apps/amarok.png

rcdir = $(kde_datadir)/amarok
rc_DATA = amarokui.rc

applnk_DATA =  amarok.desktop
applnkdir = $(kde_appsdir)/Multimedia

messages: rc.cpp
	LIST=`find . -name \*.h -o -name \*.hh -o -name \*.H -o -name \*.hxx -o -name \*.hpp -o -name \*.cpp -o -name \*.cc -o -name \*.cxx -o -name \*.ecpp -o -name \*.C`; \
	if test -n "$$LIST"; then \
	 $(XGETTEXT) $$LIST -o $(podir)/amarok.pot; \
	fi

SUBDIRS = images amarokarts data

INCLUDES= $(all_includes)
METASOURCES = AUTO

EXTRA_DIST = browserwidget.h browserwin.h \
	effectwidget.h expandbutton.h \
	Options1.ui playerapp.h \
	playerwidget.h playlistitem.h \
	playlistwidget.h viswidget.h

bin_PROGRAMS = amarok

amarok_SOURCES = main.cpp viswidget.cpp playlistwidget.cpp \
	playlistitem.cpp playerwidget.cpp playerapp.cpp \
	Options1.ui expandbutton.cpp effectwidget.cpp \
	browserwin.cpp browserwidget.cpp
amarok_LDADD = ./amarokarts/libamarokarts.la -lqtmcop -lkmedia2_idl \
	-lartsflow -lsoundserver_idl -lartskde -lartsgui -lartsgui_kde \
	-lartsmodules $(LIB_KFILE) $(LIB_KDEUI) $(LIB_KDECORE)
# $(LIBSOCKET)
amarok_LDFLAGS = $(all_libraries) $(KDE_RPATH)

noinst_HEADERS = Options1.h browserwidget.h browserwin.h \
	effectwidget.h expandbutton.h playerapp.h \
	playerwidget.h playlistitem.h playlistwidget.h\
	viswidget.h

KDE_ICONS = AUTO

rcdir = $(kde_datadir)/amarok
rc_DATA = amarokui.rc

applnk_DATA =  amarok.desktop
applnkdir = $(kde_appsdir)/Multimedia

messages: rc.cpp
	LIST=`find . -name \*.h -o -name \*.hh -o -name \*.H -o -name \*.hxx -o -name \*.hpp -o -name \*.cpp -o -name \*.cc -o -name \*.cxx -o -name \*.ecpp -o -name \*.C`; \
	if test -n "$$LIST"; then \
	 $(XGETTEXT) $$LIST -o $(podir)/amarok.pot; \
	fi

####### kdevelop will overwrite this part!!! (begin)##########
bin_PROGRAMS = amarok

## INCLUDES were found outside kdevelop specific part

amarok_SOURCES = main.cpp viswidget.cpp playlistwidget.cpp playlistitem.cpp playerwidget.cpp playerapp.cpp Options1.ui expandbutton.cpp effectwidget.cpp browserwin.cpp browserwidget.cpp
amarok_LDADD = ./amarokarts/libamarokarts.la -lqtmcop -lkmedia2_idl -lartsflow -lsoundserver_idl -lartskde -lartsgui -lartsgui_kde -lartsmodules $(LIB_KFILE) $(LIB_KDEUI) $(LIB_KDECORE) $(LIB_QT) $(LIBSOCKET)

SUBDIRS = images amarokarts data 

EXTRA_DIST = amarok.desktop lo64-app-amarok.png lo48-app-amarok.png lo32-app-amarok.png lo16-app-amarok.png browserwidget.cpp browserwidget.h browserwin.cpp browserwin.h effectwidget.cpp effectwidget.h expandbutton.cpp expandbutton.h Options1.ui playerapp.cpp playerapp.h playerwidget.cpp playerwidget.h playlistitem.cpp playlistitem.h playlistwidget.cpp playlistwidget.h viswidget.cpp viswidget.h main.cpp 

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

####### kdevelop will overwrite this part!!! (end)############
# These paths are KDE specific. Use them:
# kde_appsdir         Where your application's menu entry (.desktop) should go to.
# kde_icondir         Where your icon should go to - better use KDE_ICON.
# kde_sounddir        Where your sounds should go to.
# kde_htmldir         Where your docs should go to. (contains lang subdirs)
# kde_datadir         Where you install application data. (Use a subdir)
# kde_locale          Where translation files should go to. (contains lang subdirs)
# kde_cgidir          Where cgi-bin executables should go to.
# kde_confdir         Where config files should go to (system-wide ones with default values).
# kde_mimedir         Where mimetypes .desktop files should go to.
# kde_servicesdir     Where services .desktop files should go to.
# kde_servicetypesdir Where servicetypes .desktop files should go to.
# kde_toolbardir      Where general toolbar icons should go to (deprecated, use KDE_ICON).
# kde_wallpaperdir    Where general wallpapers should go to.
# kde_templatesdir    Where templates for the "New" menu (Konqueror/KDesktop) should go to.
# kde_bindir          Where executables should go to. Use bin_PROGRAMS or bin_SCRIPTS.
# kde_libdir          Where shared libraries should go to. Use lib_LTLIBRARIES.
# kde_moduledir       Where modules (e.g. parts) should go to. Use kde_module_LTLIBRARIES.
# kde_styledir        Where Qt/KDE widget styles should go to (new in KDE 3).
# kde_designerdir     Where Qt Designer plugins should go to (new in KDE 3).

# set the include path for X, qt and KDE
INCLUDES= $(all_includes)

METASOURCES = AUTO

# the library search path. 
amarok_LDFLAGS = $(all_libraries) $(KDE_RPATH)

rcdir = $(kde_datadir)/amarok
rc_DATA = amarokui.rc

messages: rc.cpp
	LIST=`find . -name \*.h -o -name \*.hh -o -name \*.H -o -name \*.hxx -o -name \*.hpp -o -name \*.cpp -o -name \*.cc -o -name \*.cxx -o -name \*.ecpp -o -name \*.C`; \
	if test -n "$$LIST"; then \
	 $(XGETTEXT) $$LIST -o $(podir)/amarok.pot; \
	fi

noinst_HEADERS = Options1.h browserwidget.h browserwin.h effectwidget.h expandbutton.h playerapp.h playerwidget.h playlistitem.h playlistwidget.h viswidget.h
applnkMultimedia_DATA =  amarok.desktop
applnkMultimediadir = $(kde_appsdir)/Multimedia

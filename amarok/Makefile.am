SUBDIRS = amarokcore amarokarts analyzers titleproxy engine images data

INCLUDES = \
        -I$(kde_includes)/arts \
        $(taglib_includes) \
        $(all_includes) \
        -Iamarokcore \
        -I$(srcdir)/amarokcore

bin_PROGRAMS = amarok

amarok_SOURCES = playlistwidget.cpp playlisttooltip.cpp playlistloader.cpp playlistitem.cpp playlistbrowser.cpp \
                 browserwin.cpp streambrowser.cpp expandbutton.cpp \
                 filebrowser.cpp kactionselector.cpp kbookmarkhandler.cpp amarokfilelist.cpp \
                 playerwidget.cpp osd.cpp effectwidget.cpp amarokbutton.cpp amarokslider.cpp amaroksystray.cpp \
                 Options1.ui Options2.ui Options3.ui Options4.ui Options5.ui \
                 playerapp.cpp fht.cpp amarokconfigdialog.cpp

amarok_LDADD = \
        $(top_builddir)/amarok/amarok/amarokcore/libamarokcore.a \
        $(top_builddir)/amarok/amarok/engine/libengine.a \
        $(top_builddir)/amarok/amarok/titleproxy/libtitleproxy.a \
        $(top_builddir)/amarok/amarok/analyzers/libanalyzers.a \
        $(top_builddir)/amarok/amarok/amarokarts/libamarokarts.la \
        -lqtmcop -lkmedia2_idl -lartsflow -lsoundserver_idl -lartskde \
        -lartsgui -lartsgui_kde -lartsmodules $(LIB_KUTILS) $(LIB_KFILE) \
        $(LIB_KDEUI) $(LIB_KDECORE) $(taglib_libs) $(LIB_GST)

# $(LIBSOCKET)
amarok_LDFLAGS = $(all_libraries) $(KDE_RPATH)

METASOURCES = AUTO
KDE_ICON = AUTO

rcdir = $(kde_datadir)/amarok
rc_DATA = amarokui.rc

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

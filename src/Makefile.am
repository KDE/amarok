if with_arts
    AMAROKARTS_BUILD = amarokarts
endif

bin_PROGRAMS = amarokapp

SUBDIRS = \
    amarokcore \
    $(AMAROKARTS_BUILD) \
    analyzers \
    data \
    plugin \
    engine \
    images \
    loader \
    sqlite \
    titleproxy \
    vis

INCLUDES = \
    -I$(top_builddir)/amarok/src/amarokarts \
    -I$(top_builddir)/amarok/src/amarokcore \
    -I$(top_srcdir)/amarok/src/amarokcore \
    -I$(top_srcdir)/amarok/src/analyzers \
    -I$(top_srcdir)/amarok/src/engine \
    -I$(top_srcdir)/amarok/src/plugin \
    -I$(top_srcdir)/amarok/src/sqlite \
    -I$(top_srcdir)/amarok/src/titleproxy \
    -I$(kde_includes)/arts \
    $(taglib_includes) \
    $(all_includes)

amarokapp_SOURCES = Options1.ui     Options2.ui     Options3.ui     Options4.ui     Options5.ui     configdialog.cpp     amarokfilelist.cpp     actionclasses.cpp     sliderwidget.cpp     systray.cpp     browserbar.cpp     playlistwindow.cpp     collectionbrowser.cpp     directorylist.cpp     directorylistbase.ui     effectwidget.cpp     enginecontroller.cpp     engineobserver.cpp     fht.cpp     filebrowser.cpp     kbookmarkhandler.cpp     metabundle.cpp     osd.cpp     app.cpp     playerwindow.cpp     playlistbrowser.cpp     playlistitem.cpp     playlistloader.cpp     tracktooltip.cpp     playlist.cpp     pluginmanager.cpp     searchbrowser.cpp     socketserver.cpp     statusbar.cpp     streambrowser.cpp     threadweaver.cpp contextbrowser.cpp collectiondb.cpp

amarokapp_LDADD = $(top_builddir)/amarok/src/amarokcore/libamarokcore.la $(top_builddir)/amarok/src/analyzers/libanalyzers.la $(top_builddir)/amarok/src/engine/libengine.la $(top_builddir)/amarok/src/plugin/libplugin.la $(top_builddir)/amarok/src/sqlite/libsqlite.la $(top_builddir)/amarok/src/titleproxy/libtitleproxy.la $(LIB_KHTML) $(LIB_KUTILS) $(LIB_KFILE) $(LIB_KDEUI) $(LIB_KDECORE) $(taglib_libs) $(gl_libs)

amarokapp_LDFLAGS = $(all_libraries) $(KDE_RPATH)

METASOURCES = \
    AUTO

KDE_ICON = \
    AUTO

rcdir = \
    $(kde_datadir)/amarok
rc_DATA = \
    amarokui.rc

configdir = \
    $(kde_confdir)
config_DATA = \
    amarokrc

xdg_apps_DATA = \
    amarok.desktop

kde_servicetypes_DATA = \
    amarok_plugin.desktop

messages: rc.cpp
    LIST=`find . -name \*.h -o -name \*.hh -o -name \*.H -o -name \*.hxx -o -name \*.hpp -o -name \*.cpp -o -name \*.cc -o -name \*.cxx -o -name \*.ecpp -o -name \*.C`; \
    if test -n "$$LIST"; then \
        $(XGETTEXT) $$LIST -o $(podir)/amarok.pot; \
    fi

noinst_HEADERS =  contextbrowser.h collectiondb.h

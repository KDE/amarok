if with_included_sqlite
    SQLITE_SUBDIR = sqlite
    LIB_SQLITE_LOCAL = $(top_builddir)/amarok/src/sqlite/libsqlite.la
    sqlite_includes = -I$(top_srcdir)/amarok/src/sqlite
endif

bin_PROGRAMS = amarokapp

SUBDIRS = \
    amarokcore \
    $(SQLITE_SUBDIR) \
    analyzers \
    data \
    plugin \
    engine \
    images \
    loader \
    vis

INCLUDES = \
    -I$(top_builddir)/amarok/src/amarokcore \
    -I$(top_srcdir)/amarok/src/amarokcore \
    -I$(top_srcdir)/amarok/src/analyzers \
    -I$(top_srcdir)/amarok/src/engine \
    -I$(top_srcdir)/amarok/src/plugin \
    -I$(kde_includes)/arts \
    $(taglib_includes) \
    $(sqlite_includes) \
    $(all_includes)

amarokapp_SOURCES = \
    Options1.ui \
    Options2.ui \
    Options3.ui \
    Options4.ui \
    Options5.ui \
    actionclasses.cpp \
    amarokfilelist.cpp \
    amazonsearch.ui \
    app.cpp \
    browserbar.cpp \
    collectionbrowser.cpp \
    collectiondb.cpp \
    configdialog.cpp \
    contextbrowser.cpp \
    coverfetcher.cpp \
    covermanager.cpp \
    directorylist.cpp \
    directorylistbase.ui \
    effectwidget.cpp \
    enginecontroller.cpp \
    engineobserver.cpp \
    fht.cpp \
    filebrowser.cpp \
    kbookmarkhandler.cpp \
    ktrm.cpp \
    metabundle.cpp \
    osd.cpp \
    playerwindow.cpp \
    playlist.cpp \
    playlistbrowser.cpp \
    playlistitem.cpp \
    playlistloader.cpp \
    playlistwindow.cpp \
    pluginmanager.cpp \
    scriptmanager.cpp \
    scriptmanager_selector.cpp \
    scriptmanagerbase.ui \
    searchbrowser.cpp \
    sliderwidget.cpp \
    socketserver.cpp \
    statusbar.cpp \
    streamprovider.cpp \
    systray.cpp \
    tagdialog.cpp \
    tagdialogbase.ui \
    threadweaver.cpp \
    tracktooltip.cpp \
    welcomebrowser.cpp

amarokapp_LDADD = \
    $(top_builddir)/amarok/src/amarokcore/libamarokcore.la \
    $(top_builddir)/amarok/src/analyzers/libanalyzers.la \
    $(top_builddir)/amarok/src/engine/libengine.la \
    $(top_builddir)/amarok/src/plugin/libplugin.la \
    $(LIB_KUTILS) $(LIB_KFILE) $(LIB_KDEUI) $(LIB_KDECORE) $(LIB_KHTML) $(LIB_KJSEMBED) \
    $(taglib_libs) $(gl_libs) $(LIB_SQLITE) $(LIB_SQLITE_LOCAL) $(LIB_MUSICBRAINZ)

amarokapp_LDFLAGS = \
    $(all_libraries) \
    $(KDE_RPATH)

METASOURCES = \
    AUTO

KDE_ICON = \
    AUTO

rcdir = \
    $(kde_datadir)/amarok

rc_DATA = \
    amarokui.rc \
    amarokui_xmms.rc \
    amarokui_first.rc

configdir = \
    $(kde_confdir)

config_DATA = \
    amarokrc

xdg_apps_DATA = \
    amarok.desktop

kde_servicetypes_DATA = \
    amarok_plugin.desktop

servicemenudir = \
    $(kde_datadir)/konqueror/servicemenus

servicemenu_DATA = \
    amarok_append.desktop

messages: rc.cpp
	$(EXTRACTRC) `find . -name "*.rc" -o -name "*.ui"` > rc.cpp
	LIST=`find . -name \*.h -o -name \*.hh -o -name \*.H -o -name \*.hxx -o -name \*.hpp -o -name \*.cpp -o -name \*.cc -o -name \*.cxx -o -name \*.ecpp -o -name \*.C`; \
	if test -n "$$LIST"; then \
		$(XGETTEXT) $$LIST -o $(podir)/amarok.pot; \
	fi

install-exec-hook:
	@echo ""
	@echo "=========================="
	@echo "=  amaroK - INSTALLED  ================================"
	@echo "=========================="
	@echo "="
	@echo "=  Type amarok to start!"
	@echo "="
	@echo "=  If you have problems, please consult the README;"
	@echo "=  if the problems continue join us on #amarok."
	@echo "="
	@echo "======================================================="



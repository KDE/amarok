if with_included_sqlite
    SQLITE_SUBDIR = sqlite
    LIB_SQLITE_LOCAL = $(top_builddir)/amarok/src/sqlite/libsqlite.la
    sqlite_includes = -I$(top_srcdir)/amarok/src/sqlite
endif

if with_konqsidebar
    KONQSIDEBAR_SUBDIR = konquisidebar
endif

if with_libgpod
    IPOD_SUBDIR = gpodmediadevice
    IPOD_LIBS = gpodmediadevice/libgpodmediadevice.la
endif

#if with_ifp
    IFP_SUBDIR = ifpmediadevice
    IFP_LIBS = ifpmediadevice/libifpmediadevice.la -lusb -lifp
#endif

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
    scripts \
    themes \
    vis \
    metadata \
    $(KONQSIDEBAR_SUBDIR) \
    $(IPOD_SUBDIR) \
    $(IFP_SUBDIR) \
    statusbar \
    collectionscanner


INCLUDES = \
    -I$(top_builddir)/amarok/src/amarokcore \
    -I$(top_srcdir)/amarok/src/amarokcore \
    -I$(top_srcdir)/amarok/src/analyzers \
    -I$(top_srcdir)/amarok/src/engine \
    -I$(top_srcdir)/amarok/src/plugin \
    -I$(top_srcdir)/amarok/src/statusbar \
    -I$(kde_includes)/arts \
    $(taglib_includes) \
    $(sqlite_includes) \
    $(mysql_includes) \
    $(postgresql_includes) \
    $(LIBGPOD_CFLAGS) \
    $(IFP_CFLAGS) \
    $(EXSCALIBAR_CFLAGS) \
    $(all_includes)

amarokapp_SOURCES = \
    Options1.ui \
    Options2.ui \
    Options4.ui \
    Options5.ui \
    Options7.ui \
    Options8.ui \
    actionclasses.cpp \
    app.cpp \
    browserbar.cpp \
    clicklineedit.cpp \
    collectionbrowser.cpp \
    collectiondb.cpp \
    configdialog.cpp \
    contextbrowser.cpp \
    coverfetcher.cpp \
    covermanager.cpp \
    pixmapviewer.cpp \
    cuefile.cpp \
    dbsetup.ui  \
    dbsetup.ui.h \
    directorylist.cpp \
    effectwidget.cpp \
    enginecontroller.cpp \
    engineobserver.cpp \
    equalizergraph.cpp \
    equalizerpresetmanager.cpp \
    equalizersetup.cpp \
    fht.cpp \
    filebrowser.cpp \
    firstrunwizard.ui \
    k3bexporter.cpp \
    kbookmarkhandler.cpp \
    ktrm.cpp \
    mediabrowser.cpp \
    metabundle.cpp \
    multitabbar.cpp \
    moodbar.cpp \
    newdynamic.ui \
    osd.cpp \
    party.cpp \
    partydialogbase.ui \
    playerwindow.cpp \
    playlist.cpp \
    playlistbrowser.cpp \
    playlistbrowseritem.cpp \
    playlistitem.cpp \
    playlistloader.cpp \
    playlistselection.cpp \
    playlistwindow.cpp \
    pluginmanager.cpp \
    podcastsettings.cpp \
    podcastsettingsbase.ui \
    prettypopupmenu.cpp \
    queuemanager.cpp \
    refreshimages.cpp \
    scancontroller.cpp \
    scriptmanager.cpp \
    scriptmanagerbase.ui \
    scrobbler.cpp \
    sliderwidget.cpp \
    smartplaylisteditor.cpp \
    socketserver.cpp \
    statistics.cpp \
    statisticsbase.ui \
    streamprovider.cpp \
    systray.cpp \
    tagdialog.cpp \
    tagdialogbase.ui \
    threadweaver.cpp \
    tracktooltip.cpp \
    trackpickerdialog.cpp \
    trackpickerdialogbase.ui

amarokapp_LDADD = \
    $(IPOD_OBJS) \
    $(top_builddir)/amarok/src/amarokcore/libamarokcore.la \
    $(top_builddir)/amarok/src/analyzers/libanalyzers.la \
    $(top_builddir)/amarok/src/engine/libengine.la \
    $(top_builddir)/amarok/src/plugin/libplugin.la \
    $(top_builddir)/amarok/src/statusbar/libstatusbar.la \
    $(top_builddir)/amarok/src/metadata/libmetadata.la \
    $(LIB_KUTILS) $(LIB_KFILE) $(LIB_KDEUI) $(LIB_KDECORE) $(LIB_KHTML) $(LIB_KNEWSTUFF) \
    $(taglib_libs) $(gl_libs) $(LIB_SQLITE) $(LIB_SQLITE_LOCAL) $(EXSCALIBAR_LIBS) \
    $(LIB_TUNEPIMP) $(LIBGPOD_LIBS) \
    $(IPOD_LIBS) $(IFP_LIBS) \
    $(mysql_libs) \
    $(postgresql_libs)

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
    amarokui_xmms.rc


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


profiledatadir = \
    $(kde_datadir)/profiles

profiledata_DATA = \
    amarok.profile.xml


messages: rc.cpp
	$(EXTRACTRC) `find . -name "*.rc" -o -name "*.ui" -o -name "*.kcfg"` > rc.cpp
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



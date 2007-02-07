
FIND_PATH( KONQSIDEBAR_INCLUDE_DIR konqsidebarplugin.h
  ${KDE4_INCLUDE_DIR}
  /usr/include
  /usr/local/include
)

if(KONQSIDEBAR_INCLUDE_DIR)
#   add_subdirectory( konquisidebar ) #disabling for now
else(KONQSIDEBAR_INCLUDE_DIR)
    MESSAGE(STATUS "You're missing the kdebase header files")
    MESSAGE(STATUS "Konqueror sidebar will not be compiled.")
endif(KONQSIDEBAR_INCLUDE_DIR)

ADD_DEFINITIONS(${TAGLIB_CFLAGS})

add_subdirectory( amarokcore )
#add_subdirectory( analyzers )
add_subdirectory( data )
add_subdirectory( images )
#add_subdirectory( loader )
add_subdirectory( scripts )
add_subdirectory( themes )
add_subdirectory( vis )
add_subdirectory( metadata )
add_subdirectory( engine )
add_subdirectory( mediadevice )
add_subdirectory( collectionscanner )
#add_subdirectory( device ) #not necessary at the moment, enable again when devicemanager has been ported to Solid


set(libsqlite_la_SRCS )
if(SQLITE_FOUND)
	set(libsqlite_SRCS
			    sqlite/alter.c
			    sqlite/analyze.c
			    sqlite/attach.c
			    sqlite/auth.c
			    sqlite/btree.c
			    sqlite/build.c
			    sqlite/callback.c
			    sqlite/complete.c
			    sqlite/date.c
			    sqlite/delete.c
			    sqlite/expr.c
			    sqlite/func.c
			    sqlite/hash.c
			    sqlite/insert.c
			    sqlite/legacy.c
			    sqlite/main.c
			    sqlite/opcodes.c
			    sqlite/os.c
			    sqlite/os_unix.c
			    sqlite/os_win.c
			    sqlite/pager.c
			    sqlite/parse.c
			    sqlite/pragma.c
			    sqlite/prepare.c
			    sqlite/printf.c
			    sqlite/random.c
			    sqlite/select.c
			    sqlite/table.c
				sqlite/tokenize.c
			    sqlite/trigger.c
			    sqlite/update.c
			    sqlite/utf.c
			    sqlite/util.c
			    sqlite/vacuum.c
				sqlite/vdbe.c
				sqlite/vdbeapi.c
				sqlite/vdbeaux.c
				sqlite/vdbefifo.c
				sqlite/vdbemem.c
				sqlite/where.c
)
endif(SQLITE_FOUND)


include_directories(
	${CMAKE_SOURCE_DIR}/amarok/src/amarokcore
#	${CMAKE_SOURCE_DIR}/amarok/src/klegendary
#	${CMAKE_SOURCE_DIR}/amarok/src/analyzers
	${CMAKE_SOURCE_DIR}/amarok/src/plugin
	${CMAKE_SOURCE_DIR}/amarok/src/statusbar
	${CMAKE_SOURCE_DIR}/amarok/src/mediadevice
	${CMAKE_SOURCE_DIR}/amarok/src
	${CMAKE_BINARY_DIR}/amarok/src #for amarokconfig.h
	${KDE4_INCLUDE_DIR}
	${QT_INCLUDES}  )


set(libamarokcore_SRCS
#	${CMAKE_SOURCE_DIR}/amarok/src/amarokcore/amarokdcophandler.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/amarokcore/crashhandler.cpp
)

set(libplugin_SRCS
	${CMAKE_SOURCE_DIR}/amarok/src/plugin/plugin.cpp
	)

# set(libanalyzers_SRCS
#     ${CMAKE_SOURCE_DIR}/amarok/src/analyzers/analyzerbase.cpp
#     ${CMAKE_SOURCE_DIR}/amarok/src/analyzers/analyzerfactory.cpp
#     ${CMAKE_SOURCE_DIR}/amarok/src/analyzers/baranalyzer.cpp
#     ${CMAKE_SOURCE_DIR}/amarok/src/analyzers/blockanalyzer.cpp
#     ${CMAKE_SOURCE_DIR}/amarok/src/analyzers/glanalyzer.cpp
#     ${CMAKE_SOURCE_DIR}/amarok/src/analyzers/glanalyzer2.cpp
#     ${CMAKE_SOURCE_DIR}/amarok/src/analyzers/glanalyzer3.cpp
#     ${CMAKE_SOURCE_DIR}/amarok/src/analyzers/sonogram.cpp
#     ${CMAKE_SOURCE_DIR}/amarok/src/analyzers/turbine.cpp
#     ${CMAKE_SOURCE_DIR}/amarok/src/analyzers/boomanalyzer.cpp
# )

set(libstatusbar_SRCS
    ${CMAKE_SOURCE_DIR}/amarok/src/statusbar/statusBarBase.cpp
    ${CMAKE_SOURCE_DIR}/amarok/src/statusbar/statusbar.cpp
    ${CMAKE_SOURCE_DIR}/amarok/src/statusbar/overlayWidget.cpp
    ${CMAKE_SOURCE_DIR}/amarok/src/statusbar/popupMessage.cpp
    ${CMAKE_SOURCE_DIR}/amarok/src/statusbar/progressBar.cpp
    ${CMAKE_SOURCE_DIR}/amarok/src/statusbar/squeezedtextlabel.cpp
    ${CMAKE_SOURCE_DIR}/amarok/src/statusbar/queueLabel.cpp
	)

########### next target ###############

set(amarok_LIB_SRCS ${libamarokcore_SRCS} ${libplugin_SRCS} ${libstatusbar_SRCS} ${libmetadata_SRCS}
        ${libsqlite_SRCS}
        # ${libanalyzers_SRCS}
   Options1.ui.h
   actionclasses.cpp
   app.cpp
   atomicstring.cpp
   clicklineedit.cpp
   collectionbrowser.cpp
   collectiondb.cpp
   columnlist.cpp
   configdialog.cpp
   contextbrowser.cpp
   coverfetcher.cpp
   covermanager.cpp
   cuefile.cpp
   dbsetup.ui.h
   deletedialog.cpp
   deviceconfiguredialog.cpp
   devicemanager.cpp
   directorylist.cpp
   dynamicmode.cpp
   editfilterdialog.cpp
   enginebase.cpp
   enginecontroller.cpp
   engineobserver.cpp
   equalizergraph.cpp
   equalizerpresetmanager.cpp
   equalizersetup.cpp
   expression.cpp
   fht.cpp
   filebrowser.cpp
   hintlineedit.cpp
   htmlview.cpp
   iconloader.cpp
   k3bexporter.cpp
   kbookmarkhandler.cpp
   ktrm.cpp
   lastfm.cpp
   mediabrowser.cpp
   mediadevicemanager.cpp
   medium.cpp
   mediumpluginmanager.cpp
   metabundle.cpp
   metabundlesaver.cpp
   moodbar.cpp
   mountpointmanager.cpp
   mydiroperator.cpp
   osd.cpp
   pixmapviewer.cpp
   playlist.cpp
   playlistbrowser.cpp
   playlistbrowseritem.cpp
   playlistitem.cpp
   playlistloader.cpp
   playlistselection.cpp
   playlistwindow.cpp
   pluginmanager.cpp
   podcastsettings.cpp
   prettypopupmenu.cpp
   queuemanager.cpp
   refreshimages.cpp
   scancontroller.cpp
   scriptmanager.cpp
   scrobbler.cpp
   sidebar.h
   sidebarwidget.cpp
   sliderwidget.cpp
   smartplaylisteditor.cpp
   socketserver.cpp
   statistics.cpp
   systray.cpp
   tagdialog.cpp
   tagguesser.cpp
   threadmanager.cpp
   tooltip.cpp
   trackpickerdialog.cpp
   tracktooltip.cpp
   transferdialog.cpp
   xmlloader.cpp
   xspfplaylist.cpp )

kde4_add_kcfg_files(amarok_LIB_SRCS ${CMAKE_SOURCE_DIR}/amarok/src/amarokcore/amarokconfig.kcfgc)



kde4_automoc(${amarok_LIB_SRCS})

kde4_add_ui3_files(amarok_LIB_SRCS
   Options1.ui
   Options2.ui
   Options4.ui
   Options5.ui
   Options7.ui
   Options8.ui
   dbsetup.ui
   deletedialogbase.ui
#   firstrunwizard.ui
   newdynamic.ui
   organizecollectiondialog.ui
   podcastsettingsbase.ui
   scriptmanagerbase.ui
   tagdialogbase.ui
#   tagguesserconfigdialog.ui
   trackpickerdialogbase.ui )

kde4_add_ui_files(amarok_LIB_SRCS
    tagguesserconfigdialog.ui )

kde4_add_library(amarok SHARED ${amarok_LIB_SRCS})

target_link_libraries(amarok
        ${KDE4_KUTILS_LIBS} ${KDE4_KDEUI_LIBS} ${KDE4_KHTML_LIBS} ${KDE4_KNEWSTUFF_LIBS}
        ${TAGLIB_LIBRARIES}
        ${SQLITE_LIBRARIES}
        ${KDE4_KDE3SUPPORT_LIBS}
        amarok_taglib
)

set_target_properties(amarok PROPERTIES VERSION 1.0.0 SOVERSION 1 )
install(TARGETS amarok DESTINATION ${LIB_INSTALL_DIR} )


########### next target ###############

set(amarokapp_SRCS main.cpp )



kde4_automoc(${amarokapp_SRCS})

kde4_add_executable(amarokapp ${amarokapp_SRCS})

target_link_libraries(amarokapp  ${KDE4_KDECORE_LIBS} amarok )

install(TARGETS amarokapp DESTINATION ${BIN_INSTALL_DIR})


########### install files ###############

install( FILES amarok.desktop DESTINATION ${XDG_APPS_DIR} )
install(FILES  amarok.profile.xml DESTINATION ${DATA_INSTALL_DIR}/profiles )
install(FILES amarokrc DESTINATION ${CONFIG_INSTALL_DIR})
install( FILES amarok_plugin.desktop DESTINATION ${SERVICETYPES_INSTALL_DIR})
install(FILES  amarokui.rc      amarokui_xmms.rc DESTINATION ${DATA_INSTALL_DIR}/amarok)
install(FILES  amarok_append.desktop DESTINATION ${DATA_INSTALL_DIR}/konqueror/servicemenus)

kde4_install_icons( ${ICON_INSTALL_DIR} )



#original Makefile.am contents follow:

#if with_included_sqlite
#    SQLITE_SUBDIR = sqlite
#    LIB_SQLITE_LOCAL = $(top_builddir)/amarok/src/sqlite/libsqlite.la
#    sqlite_includes = -I$(top_srcdir)/amarok/src/sqlite
#endif
#
#if with_konqsidebar
#    KONQSIDEBAR_SUBDIR = konquisidebar
#endif
#
#
#lib_LTLIBRARIES = libamarok.la
#
#SUBDIRS = \
#    amarokcore \
#    $(SQLITE_SUBDIR) \
#    analyzers \
#    data \
#    plugin \
#    images \
#    loader \
#    scripts \
#    themes \
#    vis \
#    metadata \
#    $(KONQSIDEBAR_SUBDIR) \
#    statusbar \
#    . \
#    engine \
#    mediadevice \
#    collectionscanner
#
#
#INCLUDES = \
#    -I$(top_builddir)/amarok/src/amarokcore \
#    -I$(top_srcdir)/amarok/src/amarokcore \
#    -I$(top_srcdir)/amarok/src/klegendary \
#    -I$(top_srcdir)/amarok/src/analyzers \
#    -I$(top_srcdir)/amarok/src/plugin \
#    -I$(top_srcdir)/amarok/src/statusbar \
#    -I$(top_srcdir)/amarok/src/mediadevice \
#    -I$(top_srcdir)/amarok/src \
#    -I$(kde_includes)/arts \
#    $(taglib_includes) \
#    $(sqlite_includes) \
#    $(mysql_includes) \
#    $(postgresql_includes) \
#    $(EXSCALIBAR_CFLAGS) \
#    $(all_includes)
#
#libamarok_la_SOURCES = \
#    Options1.ui \
#    Options1.ui.h \
#    Options2.ui \
#    Options4.ui \
#    Options5.ui \
#    Options7.ui \
#    Options8.ui \
#    actionclasses.cpp \
#    app.cpp \
#    browserbar.cpp \
#    collectionbrowser.cpp \
#    collectiondb.cpp \
#    columnlist.cpp \
#    configdialog.cpp \
#    contextbrowser.cpp \
#    coverfetcher.cpp \
#    covermanager.cpp \
#    cuefile.cpp \
#    dbsetup.ui  \
#    dbsetup.ui.h \
#    deviceconfiguredialog.cpp \
#    devicemanager.cpp \
#    directorylist.cpp \
#    dynamicmode.cpp \
#    enginebase.cpp \
#    enginecontroller.cpp \
#    engineobserver.cpp \
#    equalizergraph.cpp \
#    equalizerpresetmanager.cpp \
#    equalizersetup.cpp \
#    expression.cpp \
#    fht.cpp \
#    filebrowser.cpp \
#    firstrunwizard.ui \
#    htmlview.cpp \
#    iconloader.cpp \
#    k3bexporter.cpp \
#    kbookmarkhandler.cpp \
#    ktrm.cpp \
#    mediabrowser.cpp \
#    medium.cpp \
#    mediumpluginmanager.cpp \
#    metabundle.cpp \
#    moodbar.cpp \
#    multitabbar.cpp \
#    mydiroperator.cpp \
#    newdynamic.ui \
#    organizecollectiondialog.ui \
#    osd.cpp \
#    pixmapviewer.cpp \
#    playlist.cpp \
#    playlistbrowser.cpp \
#    playlistbrowseritem.cpp \
#    playlistitem.cpp \
#    playlistloader.cpp \
#    playlistselection.cpp \
#    playlistwindow.cpp \
#    pluginmanager.cpp \
#    podcastsettings.cpp \
#    podcastsettingsbase.ui \
#    prettypopupmenu.cpp \
#    queuemanager.cpp \
#    refreshimages.cpp \
#    scancontroller.cpp \
#    scriptmanager.cpp \
#    scriptmanagerbase.ui \
#    scrobbler.cpp \
#    sliderwidget.cpp \
#    smartplaylisteditor.cpp \
#    socketserver.cpp \
#    statistics.cpp \
#    systray.cpp \
#    tagdialog.cpp \
#    tagdialogbase.ui \
#    tagguesser.cpp \
#    tagguesserconfigdialog.ui \
#    threadweaver.cpp \
#    trackpickerdialog.cpp \
#    trackpickerdialogbase.ui \
#    tracktooltip.cpp \
#    transferdialog.cpp \
#    xmlloader.cpp \
#    xspfplaylist.cpp
#
#libamarok_la_LIBADD = \
#    $(top_builddir)/amarok/src/amarokcore/libamarokcore.la \
#    $(top_builddir)/amarok/src/analyzers/libanalyzers.la \
#    $(top_builddir)/amarok/src/plugin/libplugin.la \
#    $(top_builddir)/amarok/src/statusbar/libstatusbar.la \
#    $(top_builddir)/amarok/src/metadata/libmetadata.la \
#    $(LIB_KUTILS) $(LIB_KFILE) $(LIB_KDEUI) $(LIB_KDECORE) $(LIB_KHTML) $(LIB_KNEWSTUFF) \
#    $(taglib_libs) $(gl_libs) $(LIB_SQLITE) $(LIB_SQLITE_LOCAL) $(EXSCALIBAR_LIBS) \
#    $(LIB_TUNEPIMP) \
#    $(mysql_libs) \
#    $(postgresql_libs)
#
#libamarok_la_LDFLAGS = \
#    $(all_libraries) \
#    $(KDE_RPATH)
#
#METASOURCES = \
#    AUTO
#
#KDE_ICON = \
#    AUTO
#
#
#
#bin_PROGRAMS = amarokapp
#
#amarokapp_SOURCES = \
#    main.cpp
#
#amarokapp_LDADD = \
#    $(top_builddir)/amarok/src/amarokcore/libamarokcore.la \
#    libamarok.la \
#    $(top_builddir)/amarok/src/analyzers/libanalyzers.la \
#    $(top_builddir)/amarok/src/plugin/libplugin.la \
#    $(top_builddir)/amarok/src/statusbar/libstatusbar.la \
#    $(top_builddir)/amarok/src/metadata/libmetadata.la \
#    $(LIB_KUTILS) $(LIB_KFILE) $(LIB_KDEUI) $(LIB_KDECORE) $(LIB_KHTML) $(LIB_KNEWSTUFF) \
#    $(taglib_libs) $(gl_libs) $(LIB_SQLITE) $(LIB_SQLITE_LOCAL) $(EXSCALIBAR_LIBS) \
#    $(LIB_TUNEPIMP) \
#    $(mysql_libs) \
#    $(postgresql_libs)
#
#amarokapp_LDFLAGS = \
#    $(all_libraries) \
#    $(KDE_RPATH)
#
#rcdir = \
#    $(kde_datadir)/amarok
#
#rc_DATA = \
#    amarokui.rc \
#    amarokui_xmms.rc
#
#
#configdir = \
#    $(kde_confdir)
#
#config_DATA = \
#    amarokrc
#
#
#xdg_apps_DATA = \
#    amarok.desktop
#
#kde_servicetypes_DATA = \
#    amarok_plugin.desktop
#
#
#servicemenudir = \
#    $(kde_datadir)/konqueror/servicemenus
#
#servicemenu_DATA = \
#    amarok_append.desktop
#
#
#profiledatadir = \
#    $(kde_datadir)/profiles
#
#profiledata_DATA = \
#    amarok.profile.xml
#
#
#messages: rc.cpp
#	$(EXTRACTRC) `find . -name "*.rc" -o -name "*.ui" -o -name "*.kcfg"` > rc.cpp
#	LIST=`find . -name \*.h -o -name \*.hh -o -name \*.H -o -name \*.hxx -o -name \*.hpp -o -name \*.cpp -o -name \*.cc -o -name \*.cxx -o -name \*.ecpp -o -name \*.C`; \
#	if test -n "$$LIST"; then \
#		$(XGETTEXT) $$LIST -o $(podir)/amarok.pot; \
#	fi
#
#install-exec-hook:
#	@echo ""
#	@echo "=========================="
#	@echo "=  amaroK - INSTALLED  ================================"
#	@echo "=========================="
#	@echo "="
#	@echo "=  Type amarok to start!"
#	@echo "="
#	@echo "=  If you have problems, please consult the README;"
#	@echo "=  if the problems continue join us on #amarok."
#	@echo "="
#	@echo "======================================================="
#
#

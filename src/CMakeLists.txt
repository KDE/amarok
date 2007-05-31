
FIND_PATH( KONQSIDEBAR_INCLUDE_DIR konqsidebarplugin.h
  ${KDE4_INCLUDE_DIR}
  /usr/include
  /usr/local/include
)

MESSAGE(STATUS "reactivate konquesidebar when it will ported")
#if(KONQSIDEBAR_INCLUDE_DIR)
#   add_subdirectory( konquisidebar ) #disabling for now
#else(KONQSIDEBAR_INCLUDE_DIR)
#    MESSAGE(STATUS "You're missing the kdebase header files")
#    MESSAGE(STATUS "Konqueror sidebar will not be compiled.")
#endif(KONQSIDEBAR_INCLUDE_DIR)

ADD_DEFINITIONS(${TAGLIB_CFLAGS})

if(MP4V2_FOUND)
    ADD_DEFINITIONS(-DHAVE_MP4V2)
    include_directories( ${MP4V2_INCLUDE_DIR} )
endif(MP4V2_FOUND)

add_subdirectory( amarokcore )
add_subdirectory( data )
add_subdirectory( images )
add_subdirectory( loader )
add_subdirectory( scripts )
add_subdirectory( themes )
add_subdirectory( vis )
add_subdirectory( metadata )
add_subdirectory( engine )
add_subdirectory( mediadevice )
add_subdirectory( collectionscanner )
add_subdirectory( collection )
#add_subdirectory( device ) #not necessary at the moment, enable again when devicemanager has been ported to Solid

OPTION(USE_SYSTEM_SQLITE "Use system sqlite instead of amarok's own copy" OFF)

if (USE_SYSTEM_SQLITE)
    find_package(Sqlite REQUIRED)
    set(libsqlite_SRCS )
else (USE_SYSTEM_SQLITE)
    # Use our own copy
	set(libsqlite_SRCS sqlite/sqlite3.c)
endif (USE_SYSTEM_SQLITE)


include_directories(
	amarokcore
	analyzers
    configdialog
    configdialog/dialogs
	plugin
	statusbar
	mediadevice
    servicebrowser
    #servicebrowser/magnatunestore
	${CMAKE_CURRENT_SOURCE_DIR}
	meta
	collection
	${CMAKE_CURRENT_BINARY_DIR} #for amarokconfig.h
	${KDE4_INCLUDE_DIR}
	${TAGLIB_INCLUDE_DIR}
	${QT_INCLUDES}
)


set(libamarokcore_SRCS
	amarokcore/amarokdbushandler.cpp
	amarokcore/crashhandler.cpp
)

qt4_add_dbus_adaptor( libamarokcore_SRCS amarokcore/org.kde.amarok.collection.xml amarokcore/amarokdbushandler.h Amarok::DbusCollectionHandler)
qt4_add_dbus_adaptor( libamarokcore_SRCS amarokcore/org.kde.amarok.contextbrowser.xml amarokcore/amarokdbushandler.h Amarok::DbusContextBrowserHandler)
qt4_add_dbus_adaptor( libamarokcore_SRCS amarokcore/org.kde.amarok.mediabrowser.xml amarokcore/amarokdbushandler.h Amarok::DbusMediaBrowserHandler)
qt4_add_dbus_adaptor( libamarokcore_SRCS amarokcore/org.kde.amarok.player.xml amarokcore/amarokdbushandler.h Amarok::DbusPlayerHandler)
qt4_add_dbus_adaptor( libamarokcore_SRCS amarokcore/org.kde.amarok.playlist.xml amarokcore/amarokdbushandler.h Amarok::DbusPlaylistHandler)
qt4_add_dbus_adaptor( libamarokcore_SRCS amarokcore/org.kde.amarok.playlistbrowse.xml amarokcore/amarokdbushandler.h Amarok::DbusPlaylistBrowserHandler)
qt4_add_dbus_adaptor( libamarokcore_SRCS amarokcore/org.kde.amarok.script.xml amarokcore/amarokdbushandler.h Amarok::DbusScriptHandler)


#####################################################################
# PLUGIN
#####################################################################
set(libplugin_SRCS
	plugin/plugin.cpp
    plugin/pluginconfig.cpp
)

#####################################################################
# SERVICEBROWSER
#####################################################################
set(libservicebrowser_SRCS
    servicebrowser/servicebrowser.cpp
    servicebrowser/servicebase.cpp
    servicebrowser/servicemetabase.cpp
    servicebrowser/servicecollection.cpp
    servicebrowser/servicesqlquerymaker.cpp
    servicebrowser/servicesqlcollection.cpp
    servicebrowser/infoparserbase.cpp

)

#####################################################################
# SCRIPTABLESERVICE
#####################################################################
set(libscriptableservice_SRCS
    servicebrowser/scriptableservice/scriptableservicemanager.cpp
    servicebrowser/scriptableservice/scriptableservicecontentitem.cpp
    servicebrowser/scriptableservice/scriptableservice.cpp
    servicebrowser/scriptableservice/scriptableservicecontentmodel.cpp
)

#####################################################################
# JAMENDOSERVICE
#####################################################################
set(libjamendoservice_SRCS
    servicebrowser/jamendo/jamendoservice.cpp
    servicebrowser/jamendo/JamendoMeta.cpp
    servicebrowser/jamendo/jamendodatabasehandler.cpp
    servicebrowser/jamendo/jamendoxmlparser.cpp
    servicebrowser/jamendo/JamendoInfoParser.cpp
)

#####################################################################
# MP3TUNESERVICE
#####################################################################
set(libmp3tunesservice_SRCS
    servicebrowser/mp3tunes/mp3tunesservice.cpp
    servicebrowser/mp3tunes/mp3tunesdatafetcher.cpp
)

#qt4_add_dbus_adaptor( libscriptableservice_SRCS servicebrowser/scriptableservice/org.kde.amarok.ScriptableServiceManager.xml servicebrowser/scriptableservice/scriptableservicemanager.h ScriptableServiceManager)

#####################################################################
# CONFIGDIALOG
#####################################################################
set(libconfigdialog_SRCS
    configdialog/ConfigDialog.cpp
    configdialog/ConfigDialogBase.cpp
    configdialog/dialogs/AppearanceConfig.cpp
    configdialog/dialogs/CollectionConfig.cpp
    configdialog/dialogs/EngineConfig.cpp
    configdialog/dialogs/GeneralConfig.cpp
    configdialog/dialogs/LastfmConfig.cpp
    configdialog/dialogs/MediadeviceConfig.cpp
    configdialog/dialogs/OsdConfig.cpp
    configdialog/dialogs/PlaybackConfig.cpp
)

kde4_add_ui_files(libconfigdialog_SRCS
    configdialog/dialogs/AppearanceConfig.ui
    configdialog/dialogs/CollectionConfig.ui
    configdialog/dialogs/GeneralConfig.ui
    configdialog/dialogs/LastfmConfig.ui
    configdialog/dialogs/OsdConfig.ui
    configdialog/dialogs/PlaybackConfig.ui
)

#####################################################################
# MAGNATUNESTORE
#####################################################################
set(libmagnatunestore_SRCS
	servicebrowser/magnatunestore/magnatuneinfoparser.cpp
	servicebrowser/magnatunestore/magnatunebrowser.cpp
	servicebrowser/magnatunestore/magnatunedownloaddialog.cpp
	servicebrowser/magnatunestore/magnatunepurchasedialog.cpp
	servicebrowser/magnatunestore/magnatunepurchasehandler.cpp
	servicebrowser/magnatunestore/magnatunetypes.cpp
	servicebrowser/magnatunestore/magnatunexmlparser.cpp
	servicebrowser/magnatunestore/magnatunedatabasehandler.cpp
	servicebrowser/magnatunestore/magnatuneredownloaddialog.cpp
	servicebrowser/magnatunestore/magnatuneredownloadhandler.cpp
	servicebrowser/magnatunestore/magnatunedownloadinfo.cpp
	servicebrowser/magnatunestore/magnatunealbumdownloader.cpp
)

kde4_add_ui3_files(libmagnatunestore_SRCS
	servicebrowser/magnatunestore/magnatunedownloaddialogbase.ui
	servicebrowser/magnatunestore/magnatunepurchasedialogbase.ui
	servicebrowser/magnatunestore/magnatuneredownloaddialogbase.ui
)

#####################################################################
# COLLECTIONBROWSER
#####################################################################
set(libcollectionbrowser_SRCS
    collectionbrowser/collectiontreeview.cpp
    collectionbrowser/CollectionTreeItemModelBase.cpp
    collectionbrowser/collectiontreeitemmodel.cpp
    collectionbrowser/singlecollectiontreeitemmodel.cpp
    collectionbrowser/collectionsortfilterproxymodel.cpp
    collectionbrowser/collectiontreeitem.cpp
    collectionbrowser/CollectionWidget.cpp
)

#####################################################################
# CONTEXTVIEW
#####################################################################
set(libcontextview_SRCS
    contextview/contextview.cpp
    contextview/contextbox.cpp
    contextview/albumbox.cpp
    contextview/cloudbox.cpp
    contextview/graphicsitemfader.cpp
    contextview/textfader.cpp
    contextview/introanimation.cpp
)

#####################################################################
# ANALYZERS
#####################################################################
set(libanalyzers_SRCS
    analyzers/analyzerbase.cpp
    analyzers/analyzerfactory.cpp
    analyzers/baranalyzer.cpp
    analyzers/blockanalyzer.cpp
    analyzers/glanalyzer.cpp
    analyzers/glanalyzer2.cpp
    analyzers/glanalyzer3.cpp
    analyzers/sonogram.cpp
#    analyzers/turbine.cpp
#analyzers/boomanalyzer.cpp
)

#####################################################################
# STATUSBAR
#####################################################################
set(libstatusbar_SRCS
    statusbar/statusBarBase.cpp
    statusbar/statusbar.cpp
    statusbar/overlayWidget.cpp
    statusbar/popupMessage.cpp
    statusbar/progressBar.cpp
#    statusbar/squeezedtextlabel.cpp
    statusbar/queueLabel.cpp
    statusbar/selectLabel.cpp
)

#####################################################################
# LAST.FM
#####################################################################
set(lastfm_SRCS
    meta/lastfm/LastFmMeta.cpp
)

#####################################################################
# META
#####################################################################
set(meta_SRCS
    meta/meta.cpp
)

#####################################################################
# COLLECTION
#####################################################################
set(collection_SRCS
    collection/blockingquery.cpp
    collection/collection.cpp
    collection/collectionmanager.cpp
    collection/metaquerybuilder.cpp
    collection/querymaker.cpp
    collection/support/memoryquerymaker.cpp
)

#####################################################################
# LIBAMAROK
#####################################################################
set(amarok_LIB_SRCS
    #${libmagnatunestore_SRCS}
    #${libmp3tunesservice_SRCS}
    #${libscriptableservice_SRCS}
    ${libamarokcore_SRCS}
    ${libanalyzers_SRCS}
    ${libcollectionbrowser_SRCS}
    ${libconfigdialog_SRCS}
    ${libcontextview_SRCS}
    ${libjamendoservice_SRCS}
    ${libplugin_SRCS}
    ${libservicebrowser_SRCS}
    ${libsqlite_SRCS}
    ${libstatusbar_SRCS}
    ${lastfm_SRCS}
    ${meta_SRCS}
    ${collection_SRCS}
    actionclasses.cpp
    analyzerwidget.cpp
    app.cpp
    atomicstring.cpp
    collectiondb.cpp
    columnlist.cpp
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
    playlist/PlaylistModel.cpp
    playlist/PlaylistView.cpp
    playlist/StandardTrackAdvancer.cpp
    playlist/TrackAdvancer.cpp
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
    progressslider.cpp
    querybuilder.cpp
    queuemanager.cpp
    refreshimages.cpp
    scancontroller.cpp
    scriptmanager.cpp
    scrobbler.cpp
    searchwidget.cpp
    sidebar.h
    sidebarwidget.cpp
    sliderwidget.cpp
    smartplaylisteditor.cpp
    socketserver.cpp
    StarManager.cpp
    statistics.cpp
    systray.cpp
    tagdialog.cpp
    tagguesser.cpp
    threadmanager.cpp
    tooltip.cpp
    trackpickerdialog.cpp
    tracktooltip.cpp
    transferdialog.cpp
    volumewidget.cpp
    xmlloader.cpp
    xspfplaylist.cpp
)

kde4_add_kcfg_files(amarok_LIB_SRCS amarokcore/amarokconfig.kcfgc)

kde4_automoc(${amarok_LIB_SRCS})

kde4_add_ui3_files(amarok_LIB_SRCS
   dbsetup.ui
#   firstrunwizard.ui
   newdynamic.ui
   collectionbrowser/OrganizeCollectionDialog.ui
   podcastsettingsbase.ui
   tagdialogbase.ui
#   tagguesserconfigdialog.ui
   trackpickerdialogbase.ui
)

kde4_add_ui_files(amarok_LIB_SRCS
    controlbar.ui
    deletedialogbase.ui
    scriptmanagerbase.ui
    tagguesserconfigdialog.ui
)

kde4_add_library(amarok SHARED ${amarok_LIB_SRCS})

target_link_libraries(amarok
    ${KDE4_KUTILS_LIBS} ${KDE4_KDEUI_LIBS} ${KDE4_KHTML_LIBS} ${KDE4_KNEWSTUFF_LIBS}
    ${TAGLIB_LIBRARIES}
	${KDE4_KFILE_LIBS}
    ${KDE4_KDE3SUPPORT_LIBS} 
	${KDE4_THREADWEAVER_LIBRARIES}
    amarok_taglib
)
if(NOT WIN32)
    target_link_libraries(amarok dl)
endif(NOT WIN32)

if(USE_SYSTEM_SQLITE)
    target_link_libraries(amarok ${SQLITE_LIBRARIES})
endif(USE_SYSTEM_SQLITE)

set_target_properties(amarok PROPERTIES VERSION 1.0.0 SOVERSION 1 )
install(TARGETS amarok DESTINATION ${LIB_INSTALL_DIR} )


#####################################################################
# AMAROKAPP
#####################################################################
set(amarokapp_SRCS main.cpp )

kde4_automoc(${amarokapp_SRCS})

kde4_add_executable(amarokapp ${amarokapp_SRCS})

target_link_libraries(amarokapp  ${KDE4_KDECORE_LIBS} amarok )

install(TARGETS amarokapp DESTINATION ${BIN_INSTALL_DIR})


########### install files ###############

install(FILES amarok.desktop DESTINATION ${XDG_APPS_DIR} )
install(FILES amarok.profile.xml DESTINATION ${DATA_INSTALL_DIR}/profiles )
install(FILES amarokrc DESTINATION ${CONFIG_INSTALL_DIR})
install(FILES amarok_plugin.desktop DESTINATION ${SERVICETYPES_INSTALL_DIR})
install(FILES amarokui.rc DESTINATION ${DATA_INSTALL_DIR}/amarok)
install(FILES amarok_append.desktop DESTINATION ${DATA_INSTALL_DIR}/konqueror/servicemenus)

kde4_install_icons( ${ICON_INSTALL_DIR} )



FIND_PATH( KONQSIDEBAR_INCLUDE_DIR konqsidebarplugin.h
  ${KDE4_INCLUDE_DIR}
  /usr/include
  /usr/local/include
)

MESSAGE(STATUS "reactivate konquesidebar when it is ported")
#if(KONQSIDEBAR_INCLUDE_DIR)
#   add_subdirectory( konquisidebar ) #disabling for now
#else(KONQSIDEBAR_INCLUDE_DIR)
#    MESSAGE(STATUS "You're missing the kdebase header files")
#    MESSAGE(STATUS "Konqueror sidebar will not be compiled.")
#endif(KONQSIDEBAR_INCLUDE_DIR)

ADD_DEFINITIONS(${TAGLIB_CFLAGS} -DQT_STL)

if(MP4V2_FOUND)
    ADD_DEFINITIONS(-DHAVE_MP4V2)
    include_directories( ${MP4V2_INCLUDE_DIR} )
		endif(MP4V2_FOUND)

if(APPLE)
    set(mac_SRCS app_mac.cpp)
endif(APPLE)

add_subdirectory( amarokcore )
add_subdirectory( data )
add_subdirectory( images )
add_subdirectory( scripts )
add_subdirectory( themes )
add_subdirectory( vis )
add_subdirectory( metadata )
add_subdirectory( engine )
add_subdirectory( mediadevice )
add_subdirectory( collectionscanner )
add_subdirectory( collection )
add_subdirectory( strigi )
add_subdirectory( context )
# add_subdirectory( playlistmanager )
add_subdirectory( servicebrowser )

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
    context
    context/widgets
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
    podcasts
    widgets
    playlistbrowser
    playlistmanager
    ${CMAKE_CURRENT_BINARY_DIR} #for amarokconfig.h
    ${KDE4_INCLUDE_DIR}
    ${TAGLIB_INCLUDE_DIR}
    ${QT_INCLUDES}
)

if ( KDEMULTIMEDIA_FOUND )
    include_directories( ${KDEMULTIMEDIA_INCLUDE_DIR} )
endif ( KDEMULTIMEDIA_FOUND )


set(libamarokcore_SRCS
    amarokcore/amarokdbushandler.cpp
    amarokcore/crashhandler.cpp
)

qt4_add_dbus_adaptor( libamarokcore_SRCS amarokcore/org.kde.amarok.collection.xml amarokcore/amarokdbushandler.h Amarok::DbusCollectionHandler)
#qt4_add_dbus_adaptor( libamarokcore_SRCS amarokcore/org.kde.amarok.mediabrowser.xml amarokcore/amarokdbushandler.h Amarok::DbusMediaBrowserHandler)
qt4_add_dbus_adaptor( libamarokcore_SRCS amarokcore/org.kde.amarok.player.xml amarokcore/amarokdbushandler.h Amarok::DbusPlayerHandler)
qt4_add_dbus_adaptor( libamarokcore_SRCS amarokcore/org.kde.amarok.playlist.xml amarokcore/amarokdbushandler.h Amarok::DbusPlaylistHandler)
qt4_add_dbus_adaptor( libamarokcore_SRCS amarokcore/org.kde.amarok.playlistbrowser.xml amarokcore/amarokdbushandler.h Amarok::DbusPlaylistBrowserHandler)
qt4_add_dbus_adaptor( libamarokcore_SRCS amarokcore/org.kde.amarok.script.xml amarokcore/amarokdbushandler.h Amarok::DbusScriptHandler)
qt4_add_dbus_adaptor( libamarokcore_SRCS amarokcore/org.kde.amarok.context.xml amarokcore/amarokdbushandler.h Amarok::DbusContextHandler)

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
    servicebrowser/DynamicServiceQueryMaker.cpp
    servicebrowser/infoparserbase.cpp
    servicebrowser/ServiceAlbumCoverDownloader.cpp
    servicebrowser/servicebase.cpp
    servicebrowser/servicebrowser.cpp
    servicebrowser/servicecollection.cpp
    servicebrowser/ServiceCollectionTreeView.cpp
    servicebrowser/ServiceCustomActionsCapability.cpp
    servicebrowser/ServiceDynamicCollection.cpp
    servicebrowser/ServiceInfoProxy.cpp
    servicebrowser/ServiceListDelegate.cpp
    servicebrowser/ServiceListModel.cpp
    servicebrowser/servicemetabase.cpp
    servicebrowser/ServicePluginManager.cpp
    servicebrowser/ServiceSourceInfoCapability.cpp
    servicebrowser/servicesqlcollection.cpp
    servicebrowser/servicesqlquerymaker.cpp
    servicebrowser/ServiceSqlRegistry.cpp
)

#####################################################################
# SCRIPTABLESERVICE
#####################################################################
set(libscriptableservice_SRCS
    servicebrowser/scriptableservice/DynamicScriptableQueryMaker.cpp
    servicebrowser/scriptableservice/DynamicScriptableServiceCollection.cpp
    servicebrowser/scriptableservice/DynamicScriptableServiceMeta.cpp
    servicebrowser/scriptableservice/ScriptableServiceCollection.cpp
    servicebrowser/scriptableservice/scriptableservice.cpp
    servicebrowser/scriptableservice/ScriptableServiceInfoParser.cpp
    servicebrowser/scriptableservice/scriptableservicemanager.cpp
)


qt4_add_dbus_adaptor( libscriptableservice_SRCS servicebrowser/scriptableservice/org.kde.amarok.ScriptableServiceManager.xml servicebrowser/scriptableservice/scriptableservicemanager.h ScriptableServiceManager)

#####################################################################
# CONFIGDIALOG
#####################################################################
set(libconfigdialog_SRCS
    configdialog/ConfigDialog.cpp
    configdialog/ConfigDialogBase.cpp
    configdialog/dialogs/CollectionConfig.cpp
    configdialog/dialogs/GeneralConfig.cpp
    configdialog/dialogs/MediadeviceConfig.cpp
    configdialog/dialogs/OsdConfig.cpp
    configdialog/dialogs/PlaybackConfig.cpp
    configdialog/dialogs/ServiceConfig.cpp
)

kde4_add_ui_files(libconfigdialog_SRCS
    configdialog/dialogs/CollectionConfig.ui
    configdialog/dialogs/GeneralConfig.ui
    configdialog/dialogs/OsdConfig.ui
    configdialog/dialogs/PlaybackConfig.ui
    EditCoverSearchDialog.ui
)

kde4_add_ui_files(libconfigdialog_SRCS
    servicebrowser/magnatunestore/magnatunepurchasedialogbase.ui
)

#####################################################################
# COLLECTIONBROWSER
#####################################################################
set(libcollectionbrowser_SRCS
    collectionbrowser/CollectionTreeView.cpp
    collectionbrowser/CollectionTreeItemModelBase.cpp
    collectionbrowser/CollectionTreeItemModel.cpp
    collectionbrowser/SingleCollectionTreeItemModel.cpp
    collectionbrowser/CollectionSortFilterProxyModel.cpp
    collectionbrowser/CollectionTreeItem.cpp
    collectionbrowser/CollectionWidget.cpp
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
    #statusbar/AmarokStatusBar.cpp
    statusbar/StatusBarBase.cpp
    statusbar/ContextStatusBar.cpp
    statusbar/overlayWidget.cpp
    statusbar/popupMessage.cpp
    statusbar/progressBar.cpp
    statusbar/StatusBarMessageLabel.cpp
    statusbar/selectLabel.cpp
#   statusbar/queueLabel.cpp
)

#####################################################################
# META
#####################################################################
set(meta_SRCS
    meta/Capability.cpp
    meta/EditCapability.cpp
    meta/LastFmCapability.cpp
    meta/CustomActionsCapability.cpp
    meta/OrganiseCapability.cpp
    meta/Meta.cpp
    meta/proxy/MetaProxy.cpp
    meta/file/File.cpp
    meta/stream/Stream.cpp
    meta/PlaylistFileSupport.cpp
    meta/MetaUtility.cpp
    meta/XSPFPlaylist.cpp
    meta/M3UPlaylist.cpp
    meta/PLSPlaylist.cpp
    meta/EditablePlaylistCapability.cpp
    meta/DynamicPlaylist.cpp
    meta/MultiPlayableCapability.cpp
    meta/SourceInfoCapability.cpp
    meta/CurrentTrackActionsCapability.cpp
)

#####################################################################
# COLLECTION
#####################################################################
set(collection_SRCS
    collection/BlockingQuery.cpp
    collection/Collection.cpp
    collection/CollectionManager.cpp
    collection/CollectionLocation.cpp
    collection/MetaQueryBuilder.cpp
    collection/QueryMaker.cpp
    collection/support/MemoryMatcher.cpp
    collection/support/MemoryQueryMaker.cpp

)

#####################################################################
# CONTEXT
#####################################################################
#

set( libcontextview_SRCS
    context/Applet.cpp
    context/Containment.cpp
    context/ContextView.cpp
    context/ContextObserver.cpp
    context/ContextScene.cpp
    context/CoverBling.cpp
    context/DataEngineManager.cpp
    context/LyricsManager.cpp
    context/Svg.cpp
    context/widgets/TextWidget.cpp
    context/layouts/ContextLayout.cpp
    context/layouts/VerticalLayout.cpp
)

#####################################################################
# PODCASTS
#####################################################################
set(libpodcasts_SRCS
    podcasts/PodcastCollection.cpp
    podcasts/PodcastReader.cpp
)

#####################################################################
# PLAYLISTBROWSER
#####################################################################
set(libplaylistbrowser_SRCS
    playlistbrowser/PlaylistBrowser.cpp
    playlistbrowser/PodcastModel.cpp
    playlistbrowser/PodcastCategory.cpp
)

#####################################################################
# QUEUEMANAGER
#####################################################################
set(libqueuemanager_SRCS
    queuemanager/QueueManager.cpp
    queuemanager/QueueModel.cpp
)

#####################################################################
# PLAYLISTMANAGER
#####################################################################
set(libplaylistmanager_SRCS
    playlistmanager/PlaylistManager.cpp
    playlistmanager/PlaylistFileProvider.cpp
)

#####################################################################
# PLAYLIST
#####################################################################
set(libplaylist_SRCS
    playlist/PlaylistHeader.cpp
    playlist/PlaylistItem.cpp
    playlist/PlaylistModel.cpp
    playlist/PlaylistWidget.cpp
    playlist/RepeatTrackNavigator.cpp
    playlist/StandardTrackNavigator.cpp
    playlist/TrackNavigator.cpp
    playlist/UndoCommands.cpp
    playlist/PlaylistDropVis.cpp
    playlist/PlaylistGraphicsItem.cpp
    playlist/PlaylistGraphicsScene.cpp
    playlist/PlaylistGraphicsView.cpp
    playlist/PlaylistTextItem.cpp
    playlist/PlaylistAlbumGroup.cpp
    playlist/PlaylistClassicView.cpp
)

#####################################################################
# AUDIO CD SUPPORT
#####################################################################

if(KDEMULTIMEDIA_FOUND)
    set(audiocdsupport_SRCS
        meta/audiocd/AudioCdTrackProvider.cpp
        meta/audiocd/AudioCdTrackProvider_p.cpp
       )
endif(KDEMULTIMEDIA_FOUND)

#####################################################################
# MEDIADEVICE
#####################################################################
set(mediadevice_SRCS
    mediadevice/CopyToDeviceAction.cpp
)

#####################################################################
# LIBAMAROK
#####################################################################
set(amaroklib_LIB_SRCS
    ${libscriptableservice_SRCS}
    ${libamarokcore_SRCS}
    ${libanalyzers_SRCS}
    ${libcontextview_SRCS}
    ${libcollectionbrowser_SRCS}
    ${libconfigdialog_SRCS}
    ${libplaylist_SRCS}
    ${libplugin_SRCS}
    ${libpodcasts_SRCS}
    ${libservicebrowser_SRCS}
    ${libsqlite_SRCS}
    ${libstatusbar_SRCS}
    ${meta_SRCS}
    ${collection_SRCS}
    ${mac_SRCS}
    ${libplaylistbrowser_SRCS}
    ${libqueuemanager_SRCS}
    ${libplaylistmanager_SRCS}
    ${mediadevice_SRCS}
    actionclasses.cpp
    AmarokMimeData.cpp
    AmarokProcess.cpp
    app.cpp
    atomicstring.cpp
    CoverFetcher.cpp
    CoverManager.cpp
    cuefile.cpp
    dbsetup.ui.h
    deletedialog.cpp
    deviceconfiguredialog.cpp
    directorylist.cpp
#    editfilterdialog.cpp
    enginebase.cpp
    enginecontroller.cpp
    engineobserver.cpp
    equalizergraph.cpp
    equalizerpresetmanager.cpp
    equalizersetup.cpp
    expression.cpp
    fht.cpp
    filebrowser/FileBrowser.cpp
    filebrowser/kbookmarkhandler.cpp
    filebrowser/MyDirLister.cpp
    filebrowser/MyDirOperator.cpp
    hintlineedit.cpp
    k3bexporter.cpp
    ktrm.cpp
    MainWindow.cpp
    mediabrowser.cpp
    MediaItem.cpp
    MediaDevice.cpp
    MediaDeviceCache.cpp
    MediaDevicePluginManager.cpp
    medium.cpp
#    moodbar.cpp
    mountpointmanager.cpp
    osd.cpp
    PixmapViewer.cpp
    PlaylistHandler.cpp
    pluginmanager.cpp
    podcastsettings.cpp
    prettypopupmenu.cpp
    refreshimages.cpp
    scriptmanager.cpp
    Sidebar.h
    SimilarArtistsAction.cpp
#    SmartPlaylistEditor.cpp
    socketserver.cpp
    StarManager.cpp
#    Statistics.cpp
    systray.cpp
    tagdialog.cpp
    collectionbrowser/OrganizeCollectionDialog.cpp
    TagGuesser.cpp
    threadmanager.cpp
    tooltip.cpp
    trackpickerdialog.cpp
#    tracktooltip.cpp
    transferdialog.cpp
    widgets/analyzerwidget.cpp
    widgets/progressslider.cpp
    widgets/searchwidget.cpp
    widgets/SidebarWidget.cpp
    widgets/sliderwidget.cpp
    widgets/volumewidget.cpp
    widgets/MainToolbar.cpp
    SvgTinter.cpp
    xspfplaylist.cpp
)


set( amaroklib_DEPENDS "amarokplasma" )

kde4_add_kcfg_files(amaroklib_LIB_SRCS amarokcore/amarokconfig.kcfgc)

kde4_add_ui3_files(amaroklib_LIB_SRCS
   dbsetup.ui
#   newdynamic.ui
#   tagdialogbase.ui
#   tagguesserconfigdialog.ui
)

kde4_add_ui_files(amaroklib_LIB_SRCS
    deletedialogbase.ui
    scriptmanagerbase.ui
    tagdialogbase.ui
    tagguesserconfigdialog.ui
    playlistbrowser/PodcastCategoryBase.ui
    trackpickerdialogbase.ui
    podcastsettingsbase.ui
   collectionbrowser/OrganizeCollectionDialogBase.ui
)

kde4_add_library(amaroklib SHARED ${amaroklib_LIB_SRCS})

target_link_libraries(amaroklib
    ${KDE4_KUTILS_LIBS} ${KDE4_KDEUI_LIBS} ${KDE4_KHTML_LIBS} ${KDE4_KNEWSTUFF_LIBS}
    ${TAGLIB_LIBRARIES}
    ${KDE4_KFILE_LIBS}
    ${KDE4_KDE3SUPPORT_LIBS}
    ${KDE4_THREADWEAVER_LIBRARIES}
    ${QT_QTOPENGL_LIBRARY}
    ${OPENGL_gl_LIBRARY}
    ${OPENGL_glu_LIBRARY}
    amarok_taglib
    amarokplasma
)

if(KDEMULTIMEDIA_FOUND)
    target_link_libraries(amaroklib
        ${KCDDB_LIBRARY}
        ${KCOMPACTDISC_LIBRARY}
    )
endif(KDEMULTIMEDIA_FOUND)
if(NOT WIN32)
    target_link_libraries(amaroklib dl)
endif(NOT WIN32)

if(USE_SYSTEM_SQLITE)
    target_link_libraries(amaroklib ${SQLITE_LIBRARIES})
endif(USE_SYSTEM_SQLITE)

set_target_properties(amaroklib PROPERTIES VERSION 1.0.0 SOVERSION 1 )
install(TARGETS amaroklib DESTINATION ${LIB_INSTALL_DIR} )


#####################################################################
# AMAROK
#####################################################################

set(amarok_SRCS main.cpp )

if(Q_WS_MAC)
    kde4_add_executable(Amarok ${amarok_SRCS})
    target_link_libraries(Amarok ${KDE4_KDECORE_LIBS} amaroklib )
    install(TARGETS Amarok DESTINATION ${BIN_INSTALL_DIR})

else(Q_WS_MAC)
    set(amarok_icons
        ${CMAKE_CURRENT_SOURCE_DIR}/hi16-app-amarok.png
        ${CMAKE_CURRENT_SOURCE_DIR}/hi32-app-amarok.png
        ${CMAKE_CURRENT_SOURCE_DIR}/hi48-app-amarok.png
        ${CMAKE_CURRENT_SOURCE_DIR}/hi64-app-amarok.png
        ${CMAKE_CURRENT_SOURCE_DIR}/hi128-app-amarok.png
    )
    kde4_add_app_icon(amarok_SRCS ${amarok_icons})

    kde4_add_executable(amarok ${amarok_SRCS})
    target_link_libraries(amarok ${KDE4_KDECORE_LIBS} amaroklib )
    install(TARGETS amarok DESTINATION ${BIN_INSTALL_DIR})
endif(Q_WS_MAC)


########### install files ###############

install(FILES amarok.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} )
install(FILES amarok.profile.xml DESTINATION ${DATA_INSTALL_DIR}/profiles )
install(FILES amarokrc DESTINATION ${CONFIG_INSTALL_DIR})
install(FILES amarok_plugin.desktop DESTINATION ${SERVICETYPES_INSTALL_DIR})
install(FILES amarok_append.desktop DESTINATION
${SERVICES_INSTALL_DIR}/ServiceMenus)

install(FILES context/servicetypes/amarok_context_applet.desktop
              context/servicetypes/amarok_data_engine.desktop
              DESTINATION ${SERVICETYPES_INSTALL_DIR} )

kde4_install_icons( ${ICON_INSTALL_DIR} )


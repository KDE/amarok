ADD_DEFINITIONS(${TAGLIB_CFLAGS} -DQT_STL)

if(MP4V2_FOUND)
    ADD_DEFINITIONS(-DHAVE_MP4V2)
    include_directories( ${MP4V2_INCLUDE_DIR} )
        endif(MP4V2_FOUND)

if(APPLE)
    set(mac_SRCS app_mac.cpp)
endif(APPLE)

add_subdirectory( data )
add_subdirectory( images )
add_subdirectory( scripts )
add_subdirectory( themes )
add_subdirectory( vis )
add_subdirectory( metadata )
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
    analyzers
    collection
    configdialog
    configdialog/dialogs
    context
    context/widgets
    dbus
    dialogs
    dynamic
    mediadevice
    meta
    playlistbrowser
    playlistmanager
    plugin
    podcasts
    servicebrowser
    statusbar
    widgets
    ${CMAKE_CURRENT_BINARY_DIR} #for amarokconfig.h
    ${KDE4_INCLUDE_DIR}
    ${TAGLIB_INCLUDE_DIR}
    ${QT_INCLUDES}
    ${QT_QTSCRIPT_INCLUDE_DIR}
)

if ( KDEMULTIMEDIA_FOUND )
    include_directories( ${KDEMULTIMEDIA_INCLUDE_DIR} )
endif ( KDEMULTIMEDIA_FOUND )



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
    servicebrowser/InfoParserBase.cpp
    servicebrowser/ServiceAlbumCoverDownloader.cpp
    servicebrowser/ServiceBase.cpp
    servicebrowser/ServiceBrowser.cpp
    servicebrowser/ServiceCollection.cpp
    servicebrowser/ServiceCollectionLocation.cpp
    servicebrowser/ServiceCollectionTreeView.cpp
    servicebrowser/ServiceCurrentTrackActionsCapability.cpp
    servicebrowser/ServiceCustomActionsCapability.cpp
    servicebrowser/ServiceInfoProxy.cpp
    servicebrowser/ServiceListDelegate.cpp
    servicebrowser/ServiceListModel.cpp
    servicebrowser/ServiceMetaBase.cpp
    servicebrowser/ServicePluginManager.cpp
    servicebrowser/ServiceSourceInfoCapability.cpp
    servicebrowser/ServiceSqlCollection.cpp
    servicebrowser/ServiceSqlQueryMaker.cpp
    servicebrowser/ServiceSqlRegistry.cpp
    servicebrowser/ShowInServiceAction.cpp
)

qt4_add_dbus_adaptor( libservicebrowser_SRCS servicebrowser/org.kde.amarok.ServicePluginManager.xml servicebrowser/ServicePluginManager.h ServicePluginManager)

#####################################################################
# SCRIPTABLESERVICE
#####################################################################
set(libscriptableservice_SRCS
    servicebrowser/scriptableservice/ScriptableService.cpp
    servicebrowser/scriptableservice/ScriptableServiceCollection.cpp
    servicebrowser/scriptableservice/ScriptableServiceInfoParser.cpp
    servicebrowser/scriptableservice/ScriptableServiceManager.cpp
    servicebrowser/scriptableservice/ScriptableServiceMeta.cpp
    servicebrowser/scriptableservice/ScriptableServiceQueryMaker.cpp
)


qt4_add_dbus_adaptor( libscriptableservice_SRCS servicebrowser/scriptableservice/org.kde.amarok.ScriptableServiceManager.xml servicebrowser/scriptableservice/ScriptableServiceManager.h ScriptableServiceManager)

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
)

kde4_add_ui_files(libconfigdialog_SRCS
    servicebrowser/magnatunestore/MagnatunePurchaseDialogBase.ui
)

#####################################################################
# COLLECTIONBROWSER
#####################################################################
set(libcollectionbrowser_SRCS
    collectionbrowser/CollectionSortFilterProxyModel.cpp
    collectionbrowser/CollectionTreeItem.cpp
    collectionbrowser/CollectionTreeItemModel.cpp
    collectionbrowser/CollectionTreeItemModelBase.cpp
    collectionbrowser/CollectionTreeView.cpp
    collectionbrowser/CollectionWidget.cpp
    collectionbrowser/SingleCollectionTreeItemModel.cpp
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
    statusbar/StatusBar.cpp
    statusbar/StatusBarBase.cpp
    statusbar/StatusBarMessageLabel.cpp
    statusbar/overlayWidget.cpp
    statusbar/popupMessage.cpp
    statusbar/progressBar.cpp
    statusbar/selectLabel.cpp
#   statusbar/queueLabel.cpp
)

#####################################################################
# META
#####################################################################
set(meta_SRCS
    meta/Capability.cpp
    meta/CurrentTrackActionsCapability.cpp
    meta/CustomActionsCapability.cpp
    meta/EditCapability.cpp
    meta/EditablePlaylistCapability.cpp
    meta/LastFmCapability.cpp
    meta/M3UPlaylist.cpp
    meta/Meta.cpp
    meta/MetaUtility.cpp
    meta/MultiPlayableCapability.cpp
    meta/OrganiseCapability.cpp
    meta/PLSPlaylist.cpp
    meta/SqlPlaylist.cpp
    meta/PlaylistFileSupport.cpp
    meta/SourceInfoCapability.cpp
    meta/StreamInfoCapability.cpp
    meta/XSPFPlaylist.cpp
    meta/file/File.cpp
    meta/proxy/MetaProxy.cpp
    meta/stream/Stream.cpp
)

#####################################################################
# COLLECTION
#####################################################################
set(collection_SRCS
    collection/BlockingQuery.cpp
    collection/Collection.cpp
    collection/CollectionManager.cpp
    collection/CollectionLocation.cpp
    collection/MetaQueryMaker.cpp
    collection/QueryMaker.cpp
    collection/support/MemoryFilter.cpp
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
    context/ContextObserver.cpp
    context/ContextScene.cpp
    context/ContextView.cpp
    context/CoverBling.cpp
    context/DataEngineManager.cpp
    context/LyricsManager.cpp
    context/layouts/ContextLayout.cpp
    context/layouts/VerticalLayout.cpp
)

#####################################################################
# PODCASTS
#####################################################################
set(libpodcasts_SRCS
    podcasts/PodcastReader.cpp
    podcasts/sql/SqlPodcastMeta.cpp
    podcasts/sql/SqlPodcastProvider.cpp
)

#####################################################################
# PLAYLISTBROWSER
#####################################################################
set(libplaylistbrowser_SRCS
    playlistbrowser/DynamicModel.cpp
    playlistbrowser/DynamicCategory.cpp
    playlistbrowser/PlaylistBrowser.cpp
    playlistbrowser/UserPlaylistModel.cpp
    playlistbrowser/PlaylistCategory.cpp
    playlistbrowser/SqlPlaylistGroup.cpp
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
    playlist/PlaylistAlbumGroup.cpp
    playlist/PlaylistClassicView.cpp
    playlist/PlaylistDropVis.cpp
    playlist/PlaylistGraphicsItem.cpp
    playlist/PlaylistGraphicsScene.cpp
    playlist/PlaylistGraphicsView.cpp
    playlist/PlaylistHeader.cpp
    playlist/PlaylistItem.cpp
    playlist/PlaylistModel.cpp
    playlist/PlaylistRowList.cpp
    playlist/PlaylistTextItem.cpp
    playlist/PlaylistViewCommon.cpp
    playlist/PlaylistWidget.cpp
    playlist/RandomTrackNavigator.cpp
    playlist/RepeatPlaylistNavigator.cpp
    playlist/RepeatTrackNavigator.cpp
    playlist/StandardTrackNavigator.cpp
    playlist/TrackNavigator.cpp
    playlist/DynamicTrackNavigator.cpp
    playlist/UndoCommands.cpp
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
# DBUS
#####################################################################
set(dbus_SRCS
    dbus/RootDBusHandler.cpp
    dbus/PlayerDBusHandler.cpp
    dbus/TracklistDBusHandler.cpp
)


#####################################################################
# SCRIPTING INTERFACE
#####################################################################
set (scriptengine_SRCS
     scriptengine/ScriptImporter.cpp
     scriptengine/AmarokScript.cpp
     scriptengine/AmarokEngineScript.cpp
     scriptengine/AmarokOSDScript.cpp
     scriptengine/AmarokPlaylistScript.cpp
     scriptengine/AmarokStatusbarScript.cpp
     scriptengine/AmarokTrackInfoScript.cpp
     scriptengine/AmarokWindowScript.cpp
)

#####################################################################
# DYNAMIC
#####################################################################
set(libdynamic_SRCS
    dynamic/DynamicPlaylist.cpp
    dynamic/RandomPlaylist.cpp
)

#####################################################################
# DYNAMIC
#####################################################################
set(libdynamic_SRCS
    dynamic/DynamicPlaylist.cpp
    dynamic/RandomPlaylist.cpp
)

#####################################################################
# LIBAMAROK
#####################################################################
set(amaroklib_LIB_SRCS
    ${libscriptableservice_SRCS}
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
    ${libdynamic_SRCS}
    ${meta_SRCS}
    ${collection_SRCS}
    ${mac_SRCS}
    ${libplaylistbrowser_SRCS}
    ${libqueuemanager_SRCS}
    ${libplaylistmanager_SRCS}
    ${mediadevice_SRCS}
    ${dbus_SRCS}
    ${scriptengine_SRCS}
    ActionClasses.cpp
    AmarokMimeData.cpp
    AmarokProcess.cpp
    App.cpp
    EngineController.cpp
    EngineObserver.cpp
    Expression.cpp
    MainWindow.cpp
    MediaDevice.cpp
    MediaDeviceCache.cpp
    MediaDevicePluginManager.cpp
    MediaItem.cpp
    Osd.cpp
    PluginManager.cpp
    Sidebar.h
    StarManager.cpp
    SvgHandler.cpp
    SvgTinter.cpp
    covermanager/CoverFetcher.cpp
    covermanager/CoverFetchingActions.cpp
    covermanager/CoverManager.cpp
    CrashHandler.cpp
    cuefile.cpp
    dbsetup.ui.h
    dialogs/CollectionSetup.cpp
    dialogs/EditFilterDialog.cpp
    dialogs/PixmapViewer.cpp
    dialogs/ScriptManager.cpp
    dialogs/TagDialog.cpp
    dialogs/TagGuesser.cpp
    dialogs/deletedialog.cpp
    dialogs/deviceconfiguredialog.cpp
    dialogs/trackpickerdialog.cpp
    dialogs/transferdialog.cpp
    equalizergraph.cpp
    equalizerpresetmanager.cpp
    equalizersetup.cpp
    fht.cpp
    filebrowser/FileBrowser.cpp
    filebrowser/MyDirLister.cpp
    filebrowser/MyDirOperator.cpp
    filebrowser/kbookmarkhandler.cpp
    k3bexporter.cpp
    ktrm.cpp
    mediabrowser.cpp
    medium.cpp
    mountpointmanager.cpp
    podcastsettings.cpp
    prettypopupmenu.cpp
    refreshimages.cpp
    servicebrowser/lastfm/SimilarArtistsAction.cpp
    socketserver.cpp
    systray.cpp
    widgets/AnalyzerWidget.cpp
    widgets/MainToolbar.cpp
    widgets/ProgressSlider.cpp
    widgets/SearchWidget.cpp
    widgets/SidebarWidget.cpp
    widgets/SliderWidget.cpp
    widgets/TrackTooltip.cpp
    widgets/VolumeWidget.cpp
    widgets/hintlineedit.cpp
    widgets/Splitter.cpp
    WidgetBackgroundPainter.cpp
#    SmartPlaylistEditor.cpp
#    Statistics.cpp
#    moodbar.cpp
)

qt4_add_dbus_adaptor( amaroklib_LIB_SRCS dbus/org.freedesktop.MediaPlayer.root.xml RootDBusHandler.h Amarok::RootDBusHandler RootAdaptor RootAdaptor)
qt4_add_dbus_adaptor( amaroklib_LIB_SRCS dbus/org.freedesktop.MediaPlayer.player.xml PlayerDBusHandler.h Amarok::PlayerDBusHandler PlayerAdaptor PlayerAdaptor)
qt4_add_dbus_adaptor( amaroklib_LIB_SRCS dbus/org.freedesktop.MediaPlayer.tracklist.xml TracklistDBusHandler.h Amarok::TracklistDBusHandler TracklistAdaptor TracklistAdaptor)

set( amaroklib_DEPENDS "amarokplasma" "amarokpud" )

kde4_add_kcfg_files(amaroklib_LIB_SRCS amarokconfig.kcfgc)

kde4_add_ui3_files(amaroklib_LIB_SRCS
   dbsetup.ui
)

kde4_add_ui_files(amaroklib_LIB_SRCS
    dialogs/EditCoverSearchDialog.ui
    dialogs/deletedialogbase.ui
    dialogs/ScriptManagerBase.ui
    dialogs/tagdialogbase.ui
    dialogs/tagguesserconfigdialog.ui
    dialogs/trackpickerdialogbase.ui
    playlistbrowser/PodcastCategoryBase.ui
    podcastsettingsbase.ui
)

kde4_add_library(amaroklib SHARED ${amaroklib_LIB_SRCS})

target_link_libraries(amaroklib
    ${KDE4_KUTILS_LIBS} ${KDE4_KDEUI_LIBS} ${KDE4_KHTML_LIBS} ${KDE4_KNEWSTUFF_LIBS}
    ${TAGLIB_LIBRARIES}
    ${KDE4_KFILE_LIBS}
    ${KDE4_KDE3SUPPORT_LIBS}
    ${KDE4_THREADWEAVER_LIBRARIES}
    ${KDE4_KNEWSTUFF2_LIBS}
    ${QT_QTOPENGL_LIBRARY}
    ${OPENGL_gl_LIBRARY}
    ${OPENGL_glu_LIBRARY}
    ${QT_QTSCRIPT_LIBRARY}
    amarok_taglib
    amarokplasma
    amarokpud
)

if(APPLE)
    SET_TARGET_PROPERTIES(amaroklib PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)

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

if(CMAKE_COMPILER_IS_GNUCXX AND CMAKE_SYSTEM_NAME MATCHES Linux)
    set ( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=iso9899:1999" )
endif(CMAKE_COMPILER_IS_GNUCXX AND CMAKE_SYSTEM_NAME MATCHES Linux)

set_target_properties(amaroklib PROPERTIES VERSION 1.0.0 SOVERSION 1 )
install(TARGETS amaroklib ${INSTALL_TARGETS_DEFAULT_ARGS} )


#####################################################################
# AMAROK
#####################################################################

set(amarok_SRCS main.cpp )

kde4_add_app_icon(amarok_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/images/hi*-app-amarok.png)
if(Q_WS_MAC)
    kde4_add_executable(Amarok ${amarok_SRCS})
    target_link_libraries(Amarok ${KDE4_KDECORE_LIBS} amaroklib )
    SET_TARGET_PROPERTIES(Amarok PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
    install(TARGETS Amarok ${INSTALL_TARGETS_DEFAULT_ARGS})
else(Q_WS_MAC)
    kde4_add_executable(amarok ${amarok_SRCS})
    target_link_libraries(amarok ${KDE4_KDECORE_LIBS} amaroklib )
    install(TARGETS amarok ${INSTALL_TARGETS_DEFAULT_ARGS})
endif(Q_WS_MAC)


########### install files ###############

install(FILES amarok.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} )
install(FILES amarok_plugin.desktop DESTINATION ${SERVICETYPES_INSTALL_DIR})
install(FILES amarok_append.desktop DESTINATION
${SERVICES_INSTALL_DIR}/ServiceMenus)
install(FILES amarok.knsrc DESTINATION ${CONFIG_INSTALL_DIR})

install(FILES context/servicetypes/amarok_context_applet.desktop
              context/servicetypes/amarok_data_engine.desktop
              DESTINATION ${SERVICETYPES_INSTALL_DIR} )

install(FILES  amarok.kcfg DESTINATION ${KCFG_INSTALL_DIR} )
install(FILES dbus/org.freedesktop.MediaPlayer.root.xml
              dbus/org.freedesktop.MediaPlayer.player.xml
              dbus/org.freedesktop.MediaPlayer.tracklist.xml
              DESTINATION ${DBUS_INTERFACES_INSTALL_DIR})

kde4_install_icons( ${ICON_INSTALL_DIR} )


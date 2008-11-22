ADD_DEFINITIONS(${TAGLIB_CFLAGS})

if(MP4V2_FOUND)
    ADD_DEFINITIONS(-DHAVE_MP4V2)
    include_directories( ${MP4V2_INCLUDE_DIR} )
        endif(MP4V2_FOUND)

if(APPLE)
    set(mac_SRCS app_mac.cpp)
endif(APPLE)

# add_subdirectory( playlistmanager )
add_subdirectory( browsers/collectionbrowser )
add_subdirectory( browsers/servicebrowser )
add_subdirectory( collection )
add_subdirectory( collectionscanner )
add_subdirectory( context )
add_subdirectory( images )
add_subdirectory( metadata )
add_subdirectory( services )
add_subdirectory( scripts )
add_subdirectory( strigi )
add_subdirectory( themes )


include_directories(
#    analyzers
    browsers/playlistbrowser
    browsers/servicebrowser
    browsers/servicebrowser/scriptableservice
    charset-detector/include
    charset-detector/src
    charset-detector/src/tables/
    collection
    configdialog
    configdialog/dialogs
    context
    context/widgets
    dbus
    dialogs
    dynamic
    meta
    playlistmanager
    plugin
    podcasts
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
# SERVICEFRAMEWORK
#####################################################################
set(libserviceframework_SRCS
    services/DynamicServiceQueryMaker.cpp
    services/InfoParserBase.cpp
    services/ServiceAlbumCoverDownloader.cpp
    services/ServiceBase.cpp
    services/ServiceCollection.cpp
    services/ServiceCollectionLocation.cpp
    services/ServiceCollectionTreeView.cpp
    services/ServiceCurrentTrackActionsCapability.cpp
    services/ServiceCustomActionsCapability.cpp
    services/ServiceInfoProxy.cpp
    services/ServiceMetaBase.cpp
    services/ServicePluginManager.cpp
    services/ServiceSourceInfoCapability.cpp
    services/ServiceSqlCollection.cpp
    services/ServiceSqlQueryMaker.cpp
    services/ServiceSqlRegistry.cpp
    services/ShowInServiceAction.cpp
)


#####################################################################
# SERVICEBROWSER
#####################################################################
    set(libservicebrowser_SRCS
    browsers/servicebrowser/ServiceBrowser.cpp
    browsers/servicebrowser/ServiceListDelegate.cpp
    browsers/servicebrowser/ServiceListModel.cpp
    browsers/servicebrowser/ServiceListSortFilterProxyModel.cpp
)
        

#####################################################################
# SCRIPTABLESERVICE
#####################################################################
set(libscriptableservice_SRCS
    services/scriptable/ScriptableService.cpp
    services/scriptable/ScriptableServiceCollection.cpp
    services/scriptable/ScriptableServiceInfoParser.cpp
    services/scriptable/ScriptableServiceManager.cpp
    services/scriptable/ScriptableServiceMeta.cpp
    services/scriptable/ScriptableServiceQueryMaker.cpp
)

#####################################################################
# CONFIGDIALOG
#####################################################################
set(libconfigdialog_SRCS
    configdialog/ConfigDialog.cpp
    configdialog/ConfigDialogBase.cpp
    configdialog/dialogs/CollectionConfig.cpp
    configdialog/dialogs/GeneralConfig.cpp
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

#####################################################################
# COLLECTIONBROWSER
#####################################################################
set(libcollectionbrowser_SRCS
    browsers/collectionbrowser/CollectionSortFilterProxyModel.cpp
    browsers/collectionbrowser/CollectionTreeItem.cpp
    browsers/collectionbrowser/CollectionTreeItemModel.cpp
    browsers/collectionbrowser/CollectionTreeItemModelBase.cpp
    browsers/collectionbrowser/CollectionTreeView.cpp
    browsers/collectionbrowser/CollectionWidget.cpp
    browsers/collectionbrowser/SingleCollectionTreeItemModel.cpp
)

#####################################################################
# ANALYZERS
#####################################################################
#set(libanalyzers_SRCS
#    analyzers/analyzerbase.cpp
#    analyzers/analyzerfactory.cpp
#    analyzers/baranalyzer.cpp
#    analyzers/blockanalyzer.cpp
#    analyzers/glanalyzer.cpp
#    analyzers/glanalyzer2.cpp
#    analyzers/glanalyzer3.cpp
#    analyzers/sonogram.cpp
#    analyzers/turbine.cpp
#    analyzers/boomanalyzer.cpp
#)

#####################################################################
# STATUSBAR
#####################################################################
set(libstatusbar_SRCS
    statusbar/StatusBar.cpp
    statusbar/ProgressBar.cpp
    statusbar/KJobProgressBar.cpp
    statusbar/CompoundProgressBar.cpp
    statusbar/PopupWidget.cpp
    statusbar/LongMessageWidget.cpp
)

#####################################################################
# META
#####################################################################
set(meta_SRCS
    meta/Capability.cpp
    meta/CollectionCapability.cpp
    meta/CurrentTrackActionsCapability.cpp
    meta/CustomActionsCapability.cpp
    meta/EditCapability.cpp
    meta/EditablePlaylistCapability.cpp
    meta/ImportCapability.cpp
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
    meta/UpdateCapability.cpp
    meta/XSPFPlaylist.cpp
    meta/file/File.cpp
    meta/proxy/MetaProxy.cpp
    meta/stream/Stream.cpp
)

#####################################################################
# COLLECTION
#####################################################################
set(collection_SRCS
    collection/Collection.cpp
    collection/CollectionManager.cpp
    collection/CollectionLocation.cpp
    collection/MetaQueryMaker.cpp
    collection/QueryMaker.cpp
    collection/support/MemoryFilter.cpp
    collection/support/MemoryMatcher.cpp
    collection/support/MemoryQueryMaker.cpp
    collection/support/XmlQueryReader.cpp
    collection/support/XmlQueryWriter.cpp
    collection/support/FileCollectionLocation.cpp
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
    context/DataEngineManager.cpp
    context/LyricsManager.cpp
    context/layouts/ContextLayout.cpp
    context/layouts/VerticalLayout.cpp
    context/widgets/RatingWidget.cpp
    context/widgets/ToolBoxIcon.cpp
    context/widgets/ContainmentArrow.cpp
    context/widgets/ContainmentSelectionLayer.cpp
    context/widgets/ToolBoxMenu.cpp
    context/widgets/TrackWidget.cpp
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
    browsers/playlistbrowser/DynamicModel.cpp
    browsers/playlistbrowser/DynamicCategory.cpp
    browsers/playlistbrowser/DynamicBiasDelegate.cpp
    browsers/playlistbrowser/DynamicBiasModel.cpp
    browsers/playlistbrowser/DynamicBiasWidgets.cpp
    browsers/playlistbrowser/PlaylistBrowser.cpp
    browsers/playlistbrowser/UserPlaylistModel.cpp
    browsers/playlistbrowser/UserPlaylistTreeView.cpp
    browsers/playlistbrowser/PlaylistCategory.cpp
    browsers/playlistbrowser/SqlPlaylistGroup.cpp
    browsers/playlistbrowser/PodcastModel.cpp
    browsers/playlistbrowser/PodcastCategory.cpp
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
    playlist/GroupingProxy.cpp
    playlist/PlaylistActions.cpp
    playlist/PlaylistController.cpp
    playlist/PlaylistHeader.cpp
    playlist/PlaylistItem.cpp
    playlist/PlaylistModel.cpp
    playlist/PlaylistWidget.cpp
    playlist/UndoCommands.cpp
    playlist/navigators/DynamicTrackNavigator.cpp
    playlist/navigators/RandomAlbumNavigator.cpp
    playlist/navigators/RandomTrackNavigator.cpp
    playlist/navigators/RepeatAlbumNavigator.cpp
    playlist/navigators/RepeatTrackNavigator.cpp
    playlist/navigators/StandardTrackNavigator.cpp
    playlist/navigators/TrackNavigator.cpp
    playlist/view/PlaylistViewCommon.cpp
    playlist/view/classic/PlaylistClassicView.cpp
    playlist/view/listview/PrettyItemDelegate.cpp
    playlist/view/listview/PrettyListView.cpp
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
set(scriptengine_SRCS
    scriptengine/AmarokScript.cpp
    scriptengine/AmarokCollectionScript.cpp
    scriptengine/AmarokScriptConfig.cpp
    scriptengine/AmarokInfoScript.cpp
    scriptengine/AmarokNetworkScript.cpp
    scriptengine/AmarokScriptableServiceScript.cpp
    scriptengine/AmarokServicePluginManagerScript.cpp
    scriptengine/AmarokEngineScript.cpp
    scriptengine/AmarokOSDScript.cpp
    scriptengine/AmarokPlaylistScript.cpp
    scriptengine/AmarokStatusbarScript.cpp
    scriptengine/AmarokWindowScript.cpp
    scriptengine/AmarokLyricsScript.cpp
    scriptengine/MetaTypeExporter.cpp
    scriptengine/ScriptImporter.cpp
)

#####################################################################
# DYNAMIC
#####################################################################
set(libdynamic_SRCS
    dynamic/BiasedPlaylist.cpp
    dynamic/DynamicPlaylist.cpp
    dynamic/Bias.cpp
    dynamic/BiasSolver.cpp
    dynamic/TrackSet.cpp
    dynamic/gsl/gauss.c
)

#####################################################################
# Character Encoding Detector, imported from Mozilla
#####################################################################
set(libchardet_SRCS
    charset-detector/src/impl.cpp
    charset-detector/src/CharDistribution.cpp
    charset-detector/src/JpCntx.cpp
    charset-detector/src/LangBulgarianModel.cpp
    charset-detector/src/LangCyrillicModel.cpp
    charset-detector/src/LangGreekModel.cpp
    charset-detector/src/LangHebrewModel.cpp
    charset-detector/src/LangHungarianModel.cpp
    charset-detector/src/LangThaiModel.cpp
    charset-detector/src/nsBig5Prober.cpp
    charset-detector/src/nsCharSetProber.cpp
    charset-detector/src/nsEscCharsetProber.cpp
    charset-detector/src/nsEscSM.cpp
    charset-detector/src/nsEUCJPProber.cpp
    charset-detector/src/nsEUCKRProber.cpp
    charset-detector/src/nsEUCTWProber.cpp
    charset-detector/src/nsGB2312Prober.cpp
    charset-detector/src/nsHebrewProber.cpp
    charset-detector/src/nsLatin1Prober.cpp
    charset-detector/src/nsMBCSGroupProber.cpp
    charset-detector/src/nsMBCSSM.cpp
    charset-detector/src/nsSBCharSetProber.cpp
    charset-detector/src/nsSBCSGroupProber.cpp
    charset-detector/src/nsSJISProber.cpp
    charset-detector/src/nsUniversalDetector.cpp
    charset-detector/src/nsUTF8Prober.cpp
)

#####################################################################
# LIBAMAROK
#####################################################################
set(amaroklib_LIB_SRCS
    ${libscriptableservice_SRCS}
    ${libanalyzers_SRCS}
    ${libchardet_SRCS}
    ${libcontextview_SRCS}
    ${libcollectionbrowser_SRCS}
    ${libconfigdialog_SRCS}
    ${libplaylist_SRCS}
    ${libplugin_SRCS}
    ${libpodcasts_SRCS}
    ${libserviceframework_SRCS}
    ${libservicebrowser_SRCS}
    ${libdynamic_SRCS}
    ${meta_SRCS}
    ${collection_SRCS}
    ${mac_SRCS}
    ${libplaylistbrowser_SRCS}
    ${libqueuemanager_SRCS}
    ${libplaylistmanager_SRCS}
    ${dbus_SRCS}
    ${scriptengine_SRCS}
    ${libstatusbar_SRCS}
    ActionClasses.cpp
    AmarokMimeData.cpp
    AmarokProcess.cpp
    App.cpp
    DirectoryLoader.cpp
    EngineController.cpp
    EngineObserver.cpp
    Expression.cpp
    MainWindow.cpp
    MediaDeviceCache.cpp
    MediaDeviceMonitor.cpp
    PluginManager.cpp
    Sidebar.h
    StarManager.cpp
    SvgHandler.cpp
    SvgTinter.cpp
    covermanager/CoverFetcher.cpp
    covermanager/CoverFetchingActions.cpp
    covermanager/CoverManager.cpp
    databaseimporter/DatabaseImporter.cpp
    databaseimporter/amarok14/FastForwardImporter.cpp
    databaseimporter/amarok14/FastForwardImporterConfig.cpp
    databaseimporter/amarok14/FastForwardWorker.cpp
    databaseimporter/itunes/ITunesImporterConfig.cpp
    databaseimporter/itunes/ITunesImporter.cpp
    databaseimporter/itunes/ITunesImporterWorker.cpp
    dialogs/CollectionSetup.cpp
    dialogs/DatabaseImporterDialog.cpp
    dialogs/EditFilterDialog.cpp
    dialogs/FilenameLayoutDialog.cpp
    dialogs/PixmapViewer.cpp
    dialogs/PodcastSettingsDialog.cpp
    dialogs/ScriptSelector.cpp
    dialogs/ScriptManager.cpp
    dialogs/TagDialog.cpp
    dialogs/TagGuesser.cpp
    browsers/filebrowser/FileBrowser.cpp
    browsers/filebrowser/MyDirLister.cpp
    browsers/filebrowser/MyDirOperator.cpp
    browsers/filebrowser/kbookmarkhandler.cpp
    firstruntutorial/FirstRunTutorial.cpp
    medium.cpp
    MountPointManager.cpp
    PaletteHandler.cpp
    PopupDropperFactory.cpp
    refreshimages.cpp
    services/lastfm/SimilarArtistsAction.cpp
    Systray.cpp
    widgets/hintlineedit.cpp
    widgets/kdatecombo.cpp
    widgets/kratingpainter.cpp
    widgets/kratingwidget.cpp
    widgets/FilenameLayoutWidget.cpp
    widgets/FlowLayout.cpp
    widgets/MainControlsButton.cpp
    widgets/MainControlsWidget.cpp
    widgets/MainToolbar.cpp
    widgets/Osd.cpp
    widgets/PrettyTreeView.cpp
    widgets/ProgressSlider.cpp
    widgets/SearchWidget.cpp
    widgets/SidebarWidget.cpp
    widgets/SliderWidget.cpp
    widgets/TokenListWidget.cpp
    widgets/Token.cpp
    widgets/TrackTooltip.cpp
    widgets/VolumeWidget.cpp
    widgets/Splitter.cpp
    GlobalCollectionActions.cpp
)

qt4_add_dbus_adaptor( amaroklib_LIB_SRCS dbus/org.freedesktop.MediaPlayer.root.xml RootDBusHandler.h Amarok::RootDBusHandler RootAdaptor RootAdaptor)
qt4_add_dbus_adaptor( amaroklib_LIB_SRCS dbus/org.freedesktop.MediaPlayer.player.xml PlayerDBusHandler.h Amarok::PlayerDBusHandler PlayerAdaptor PlayerAdaptor)
qt4_add_dbus_adaptor( amaroklib_LIB_SRCS dbus/org.freedesktop.MediaPlayer.tracklist.xml TracklistDBusHandler.h Amarok::TracklistDBusHandler TracklistAdaptor TracklistAdaptor)

set( amaroklib_DEPENDS "amarokplasma" "amarokpud" )

kde4_add_kcfg_files(amaroklib_LIB_SRCS amarokconfig.kcfgc)

kde4_add_ui_files(amaroklib_LIB_SRCS
    dialogs/EditCoverSearchDialog.ui
    #dialogs/deletedialogbase.ui
    dialogs/ScriptManagerBase.ui
    dialogs/TagDialogBase.ui
    dialogs/FilenameLayoutDialog.ui
    dialogs/PodcastSettingsBase.ui
    browsers/playlistbrowser/PodcastCategoryBase.ui
)

kde4_add_library(amaroklib SHARED ${amaroklib_LIB_SRCS})

target_link_libraries(amaroklib
    ${KDE4_KUTILS_LIBS}
    ${KDE4_KDEUI_LIBS}
    ${KDE4_KHTML_LIBS}
    ${KDE4_KNEWSTUFF_LIBS}
    ${KDE4_KFILE_LIBS}
    ${KDE4_THREADWEAVER_LIBRARIES}
    ${KDE4_KNEWSTUFF2_LIBS}
    ${KDE4_PHONON_LIBRARY}
    ${KDE4_SOLID_LIBRARY}
    ${TAGLIB_LIBRARIES}
    ${QT_QTSCRIPT_LIBRARY}
    ${QT_QTSQL_LIBRARY}
    ${QT_QTWEBKIT_LIBRARY}
    ${CMAKE_THREAD_LIBS_INIT}
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
if(LIBDL_FOUND)
    target_link_libraries(amaroklib dl)
endif(LIBDL_FOUND)

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
install(FILES amarok_codecinstall.desktop DESTINATION ${SERVICETYPES_INSTALL_DIR})
install(FILES amarok_append.desktop DESTINATION ${SERVICES_INSTALL_DIR}/ServiceMenus)
install(FILES amarok.knsrc DESTINATION ${CONFIG_INSTALL_DIR})

install(FILES amaroklastfm.protocol DESTINATION ${SERVICES_INSTALL_DIR})
#install(FILES amarokitpc.protocol DESTINATION ${SERVICES_INSTALL_DIR})
#install(FILES amarokpcast.protocol DESTINATION ${SERVICES_INSTALL_DIR})

install(FILES context/servicetypes/amarok_context_applet.desktop
              context/servicetypes/amarok_data_engine.desktop
              DESTINATION ${SERVICETYPES_INSTALL_DIR} )

install(FILES  amarok.kcfg DESTINATION ${KCFG_INSTALL_DIR} )
install(FILES dbus/org.freedesktop.MediaPlayer.root.xml
              dbus/org.freedesktop.MediaPlayer.player.xml
              dbus/org.freedesktop.MediaPlayer.tracklist.xml
              DESTINATION ${DBUS_INTERFACES_INSTALL_DIR})

kde4_install_icons( ${ICON_INSTALL_DIR} )

############# AFT Tagger ##############

set(amarok_afttagger_SRCS
    afttagger/SafeFileSaver.cpp
    afttagger/afttagger_main.cpp
)

kde4_add_executable(amarok_afttagger ${amarok_afttagger_SRCS} )

target_link_libraries(amarok_afttagger
    ${KDE4_KDECORE_LIBS}
    ${QT_QTCORE_LIBS}
    ${TAGLIB_LIBRARIES}
)

install(TARGETS amarok_afttagger ${INSTALL_TARGETS_DEFAULT_ARGS})




ADD_DEFINITIONS(${TAGLIB_CFLAGS} ${TAGLIB-EXTRAS_CFLAGS})

if(APPLE)
    set(mac_SRCS
        app_mac.cpp
        mac/GrowlInterface.cpp
    )

    include_directories ( services/lastfm/ )
    set( MAC_FILES_DIR  ${CMAKE_SOURCE_DIR}/src/mac )
endif(APPLE)

# add_subdirectory( playlistmanager )
add_subdirectory( collection )
#dont use our own libplasma anymore, but still pick up our applets/engines/containments
add_subdirectory( context )
add_subdirectory( images )
add_subdirectory( services )
add_subdirectory( scripts )
add_subdirectory( themes )
add_subdirectory( aboutdialog/libattica-ocsclient )

if( STRIGI_FOUND )
    add_subdirectory( strigi )
endif( STRIGI_FOUND )

include_directories(
    aboutdialog
    aboutdialog/libattica-ocsclient
    amarokurls
    browsers/
    browsers/playlistbrowser
    browsers/servicebrowser
    browsers/servicebrowser/scriptableservice
    collection
    collection/support
    collection/mediadevicecollection
    collection/mediadevicecollection/handler  # for Handler
    collection/mediadevicecollection/support  # for MediaDeviceInfo
    collection/mediadevicecollection/playlist # for MediaDevicePlaylist etc.
    configdialog
    configdialog/dialogs
    context
    context/widgets
    context/widgets/appletexplorer/
    dbus
    dialogs
    dynamic
    meta
    meta/capabilities
    playlistmanager
    plugin
    podcasts
    statusbar
    synchronization
    timecode
    widgets
    ${KDE4_INCLUDE_DIR}
    ${TAGLIB-EXTRAS_INCLUDE_DIR}
    ${TAGLIB-EXTRAS_INCLUDES}
    ${MYSQL_INCLUDE_DIR}
    ${QT_INCLUDES}
    ${QT_QTSCRIPT_INCLUDE_DIR}
    ${QCA2_INCLUDE_DIR}
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
# PROXYCOLLECTION
#####################################################################
set(libproxycollection_SRCS
    collection/proxycollection/ProxyCollection.cpp
    collection/proxycollection/ProxyCollectionMeta.cpp
    collection/proxycollection/ProxyCollectionQueryMaker.cpp
)

#####################################################################
# MEDIADEVICEFRAMEWORK
#####################################################################
set(libmediadeviceframework_SRCS
  collection/mediadevicecollection/MediaDeviceCollection.cpp
  collection/mediadevicecollection/MediaDeviceCollectionLocation.cpp
  collection/mediadevicecollection/MediaDeviceMeta.cpp
  collection/mediadevicecollection/MediaDeviceCollectionCapability.cpp
  collection/mediadevicecollection/MediaDeviceDecoratorCapability.cpp
  collection/mediadevicecollection/handler/MediaDeviceHandler.cpp
  collection/mediadevicecollection/handler/MediaDeviceHandlerCapability.cpp
  collection/mediadevicecollection/handler/capabilities/ArtworkCapability.cpp
  collection/mediadevicecollection/handler/capabilities/CustomReadCapability.cpp
  collection/mediadevicecollection/handler/capabilities/PlaylistCapability.cpp
  collection/mediadevicecollection/handler/capabilities/ReadCapability.cpp
  collection/mediadevicecollection/handler/capabilities/ReadCapabilityBase.cpp
  collection/mediadevicecollection/handler/capabilities/WriteCapability.cpp
  collection/mediadevicecollection/handler/capabilities/WriteCapabilityBase.cpp
  collection/mediadevicecollection/playlist/MediaDevicePlaylist.cpp
  collection/mediadevicecollection/playlist/MediaDeviceUserPlaylistProvider.cpp
  collection/mediadevicecollection/support/ConnectionAssistant.cpp
  collection/mediadevicecollection/support/MediaDeviceInfo.cpp
)

#####################################################################
# SERVICEFRAMEWORK
#####################################################################
set(libserviceframework_SRCS
    services/DynamicServiceQueryMaker.cpp
    services/InfoParserBase.cpp
    services/ServiceAlbumCoverDownloader.cpp
    services/ServiceBase.cpp
    services/ServiceBookmarkThisCapability.cpp
    services/ServiceCollection.cpp
    services/ServiceCollectionLocation.cpp
    services/ServiceCollectionTreeView.cpp
    services/ServiceCurrentTrackActionsCapability.cpp
    services/ServiceCustomActionsCapability.cpp
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
)

#####################################################################
# AMAROKURL
#####################################################################
set(libamarokurl_SRCS
    amarokurls/AmarokUrl.cpp
    amarokurls/AmarokUrlAction.cpp
    amarokurls/AmarokUrlHandler.cpp
    amarokurls/BookmarkCurrentButton.cpp
    amarokurls/ContextUrlGenerator.cpp
    amarokurls/ContextUrlRunner.cpp
    amarokurls/NavigationUrlRunner.cpp
    amarokurls/NavigationUrlGenerator.cpp
    amarokurls/PlayUrlRunner.cpp
    amarokurls/PlayUrlGenerator.cpp
    amarokurls/BookmarkManager.cpp
    amarokurls/BookmarkManagerWidget.cpp
    amarokurls/BookmarkGroup.cpp
    amarokurls/BookmarkModel.cpp
    amarokurls/BookmarkTreeView.cpp
    amarokurls/BookmarkMetaActions.cpp
    timecode/TimecodeObserver.cpp
    timecode/TimecodeMeta.cpp
    timecode/TimecodeTrackProvider.cpp
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
    configdialog/dialogs/NotificationsConfig.cpp
    configdialog/dialogs/PlaybackConfig.cpp
    configdialog/dialogs/ServiceConfig.cpp
    configdialog/dialogs/DatabaseConfig.cpp
)

kde4_add_ui_files(libconfigdialog_SRCS
    configdialog/dialogs/CollectionConfig.ui
    configdialog/dialogs/EqualizerConfig.ui
    configdialog/dialogs/GeneralConfig.ui
    configdialog/dialogs/NotificationsConfig.ui
    configdialog/dialogs/PlaybackConfig.ui
    configdialog/dialogs/DatabaseConfig.ui
)

set(libbrowserframework_SRCS
    browsers/BrowserBreadcrumbItem.cpp
    browsers/BrowserBreadcrumbWidget.cpp
    browsers/BrowserCategory.cpp
    browsers/BrowserCategoryList.cpp
    browsers/BrowserCategoryListDelegate.cpp
    browsers/BrowserCategoryListModel.cpp
    browsers/BrowserCategoryListSortFilterProxyModel.cpp
    browsers/BrowserWidget.cpp
    browsers/CollectionSortFilterProxyModel.cpp
    browsers/CollectionTreeItem.cpp
    browsers/CollectionTreeItemModel.cpp
    browsers/CollectionTreeItemModelBase.cpp
    browsers/CollectionTreeView.cpp
    browsers/InfoProxy.cpp
    browsers/SingleCollectionTreeItemModel.cpp
)

#####################################################################
# COLLECTIONBROWSER
#####################################################################
set(libcollectionbrowser_SRCS
    browsers/collectionbrowser/CollectionTreeItemDelegate.cpp
    browsers/collectionbrowser/CollectionBrowserTreeView.cpp
    browsers/collectionbrowser/CollectionWidget.cpp
)

#####################################################################
# SYNCHRONIZATION
#####################################################################
set(libsynchronization_SRCS
    synchronization/MasterSlaveSynchronizationJob.cpp
    synchronization/OneWaySynchronizationJob.cpp
    synchronization/SynchronizationBaseJob.cpp
    synchronization/UnionJob.cpp
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
    meta/M3UPlaylist.cpp
    meta/Meta.cpp
    meta/MetaUtility.cpp
    meta/TagLibUtils.cpp
    ../shared/MetaReplayGain.cpp
    meta/Playlist.cpp
    meta/PLSPlaylist.cpp
    meta/StatisticsProvider.cpp
    meta/PlaylistFileSupport.cpp
    meta/PrivateMetaRegistry.cpp
    meta/XSPFPlaylist.cpp
    meta/capabilities/BookmarkThisCapability.cpp
    meta/capabilities/BoundedPlaybackCapability.cpp
    meta/capabilities/CollectionCapability.cpp
    meta/capabilities/CurrentTrackActionsCapability.cpp
    meta/capabilities/CustomActionsCapability.cpp
    meta/capabilities/DecoratorCapability.cpp
    meta/capabilities/EditCapability.cpp
    meta/capabilities/EditablePlaylistCapability.cpp
    meta/capabilities/LastFmCapability.cpp
    meta/capabilities/MultiPlayableCapability.cpp
    meta/capabilities/MultiSourceCapability.cpp
    meta/capabilities/OrganiseCapability.cpp
    meta/capabilities/ReadLabelCapability.cpp
    meta/capabilities/SourceInfoCapability.cpp
    meta/capabilities/StatisticsCapability.cpp
    meta/capabilities/StreamInfoCapability.cpp
    meta/capabilities/TimecodeLoadCapability.cpp
    meta/capabilities/TimecodeWriteCapability.cpp
    meta/capabilities/UpdateCapability.cpp
    meta/capabilities/WriteLabelCapability.cpp
    meta/file/File.cpp
    meta/multi/MultiTrack.cpp
    meta/multi/MultiSourceCapabilityImpl.cpp
    meta/cue/CueFileSupport.cpp
    meta/proxy/MetaProxy.cpp
    meta/stream/Stream.cpp
    meta/support/PermanentUrlStatisticsProvider.cpp
    meta/support/TagStatisticsProvider.cpp
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
    collection/TrackForUrlWorker.cpp
    collection/support/MemoryCustomValue.cpp
    collection/support/MemoryFilter.cpp
    collection/support/MemoryMatcher.cpp
    collection/support/MemoryQueryMaker.cpp
    collection/support/MemoryQueryMakerHelper.cpp
    collection/support/PlaylistCollection.cpp
    collection/support/PlaylistCollectionLocation.cpp
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
    context/LyricsManager.cpp
    context/ToolbarView.cpp
    context/layouts/ContextLayout.cpp
    context/layouts/VerticalLayout.cpp
    context/toolbar/AppletItemOverlay.cpp
    context/toolbar/AppletToolbar.cpp
    context/toolbar/AppletToolbarAddItem.cpp
    context/toolbar/AppletToolbarAppletItem.cpp
    context/toolbar/AppletToolbarBase.cpp
    context/toolbar/AppletToolbarConfigItem.cpp
    context/widgets/AmarokWebView.cpp
    context/widgets/RatingWidget.cpp
    context/widgets/TextScrollingWidget.cpp
    context/widgets/DropPixmapItem.cpp
    context/widgets/ToolBoxIcon.cpp
    context/widgets/ContainmentArrow.cpp
    context/widgets/ContainmentSelectionLayer.cpp
    context/widgets/appletexplorer/AppletExplorer.cpp
    context/widgets/appletexplorer/AppletIcon.cpp
    context/widgets/appletexplorer/AppletItemModel.cpp
    context/widgets/appletexplorer/AppletsList.cpp
    context/widgets/TrackWidget.cpp
)

#####################################################################
# PODCASTS
#####################################################################
set(libpodcasts_SRCS
    podcasts/PodcastReader.cpp
    podcasts/PodcastMeta.cpp
    podcasts/PodcastImageFetcher.cpp
    podcasts/PodcastProvider.cpp
    podcasts/sql/SqlPodcastMeta.cpp
    podcasts/sql/SqlPodcastProvider.cpp
    podcasts/sql/PodcastSettingsDialog.cpp
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
    browsers/playlistbrowser/QtGroupingProxy.cpp
    browsers/playlistbrowser/PlaylistBrowser.cpp
    browsers/playlistbrowser/UserPlaylistModel.cpp
    browsers/playlistbrowser/UserPlaylistTreeView.cpp
    browsers/playlistbrowser/PlaylistCategory.cpp
    browsers/playlistbrowser/PlaylistsInGroupsProxy.cpp
    browsers/playlistbrowser/PodcastModel.cpp
    browsers/playlistbrowser/PodcastCategory.cpp
)

#####################################################################
# PLAYLISTMANAGER
#####################################################################
set(libplaylistmanager_SRCS
    playlistmanager/PlaylistManager.cpp
    playlistmanager/PlaylistProvider.cpp
    playlistmanager/UserPlaylistProvider.cpp
    playlistmanager/file/PlaylistFileProvider.cpp
    playlistmanager/sql/SqlUserPlaylistProvider.cpp
    playlistmanager/sql/SqlPlaylist.cpp
    playlistmanager/sql/SqlPlaylistGroup.cpp
)

#####################################################################
# PLAYLIST
#####################################################################
set(libplaylist_SRCS
    playlist/PlaylistActions.cpp
    playlist/PlaylistBreadcrumbItem.cpp
    playlist/PlaylistBreadcrumbItemSortButton.cpp
    playlist/PlaylistBreadcrumbLevel.cpp
    playlist/PlaylistController.cpp
    playlist/PlaylistHeader.cpp
    playlist/PlaylistItem.cpp
    playlist/PlaylistModel.cpp
    playlist/PlaylistModelStack.cpp
    playlist/PlaylistSortWidget.cpp
    playlist/PlaylistViewUrlGenerator.cpp
    playlist/PlaylistViewUrlRunner.cpp
    playlist/PlaylistWidget.cpp
    playlist/ProgressiveSearchWidget.cpp
    playlist/UndoCommands.cpp
    playlist/layouts/LayoutEditDialog.cpp
    playlist/layouts/LayoutEditWidget.cpp
    playlist/layouts/LayoutConfigAction.cpp
    playlist/layouts/LayoutItemConfig.cpp
    playlist/layouts/LayoutManager.cpp
    playlist/layouts/PlaylistLayoutEditDialog.cpp
    playlist/navigators/DynamicTrackNavigator.cpp
    playlist/navigators/FavoredRandomTrackNavigator.cpp
    playlist/navigators/NavigatorConfigAction.cpp
    playlist/navigators/RandomAlbumNavigator.cpp
    playlist/navigators/RandomTrackNavigator.cpp
    playlist/navigators/RepeatAlbumNavigator.cpp
    playlist/navigators/RepeatTrackNavigator.cpp
    playlist/navigators/StandardTrackNavigator.cpp
    playlist/navigators/TrackNavigator.cpp
    playlist/view/PlaylistViewCommon.cpp
    playlist/view/listview/InlineEditorWidget.cpp
    playlist/view/listview/PrettyItemDelegate.cpp
    playlist/view/listview/PrettyListView.cpp
    playlist/view/listview/SourceSelectionPopup.cpp
    playlist/proxymodels/FilterProxy.cpp
    playlist/proxymodels/GroupingProxy.cpp
    playlist/proxymodels/ProxyBase.cpp
    playlist/proxymodels/SortAlgorithms.cpp
    playlist/proxymodels/SortProxy.cpp
    playlist/proxymodels/SortScheme.cpp
    playlist/proxymodels/SearchProxy.cpp
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
    dbus/CollectionDBusHandler.cpp
    dbus/DBusQueryHelper.cpp
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
    dynamic/CustomBias.cpp
    dynamic/CustomBiasEntry.cpp
    dynamic/CustomBiasEntryWidget.cpp
    dynamic/BiasSolver.cpp
    dynamic/TrackSet.cpp
    dynamic/gsl/gauss.c

    # biases
    dynamic/biases/EchoNest.cpp
)

#####################################################################
# RESOLVERS
#####################################################################
set(taglib_filetype_resolver_SRCS
    ../shared/taglib_filetype_resolvers/asffiletyperesolver.cpp
    ../shared/taglib_filetype_resolvers/mp4filetyperesolver.cpp
    ../shared/taglib_filetype_resolvers/wavfiletyperesolver.cpp
    ../shared/taglib_filetype_resolvers/mimefiletyperesolver.cpp
)


#####################################################################
# LIBAMAROK
#####################################################################
set(amaroklib_LIB_SRCS
    ${libscriptableservice_SRCS}
    ${libanalyzers_SRCS}
    ${libbrowserframework_SRCS}
    ${libcontextview_SRCS}
    ${libcollectionbrowser_SRCS}
    ${libconfigdialog_SRCS}
    ${libplaylist_SRCS}
    ${libplugin_SRCS}
    ${libproxycollection_SRCS}
    ${libpodcasts_SRCS}
    ${libmediadeviceframework_SRCS}
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
    ${libamarokurl_SRCS}
    ${taglib_filetype_resolver_SRCS}
    ${libsynchronization_SRCS}
    aboutdialog/AnimatedBarWidget.cpp
    aboutdialog/AnimatedWidget.cpp
    aboutdialog/ExtendedAboutDialog.cpp
    aboutdialog/FramedLabel.cpp
    aboutdialog/OcsData.cpp
    aboutdialog/OcsPersonItem.cpp
    aboutdialog/OcsPersonListWidget.cpp
    ActionClasses.cpp
    AmarokMimeData.cpp
    AmarokProcess.cpp
    App.cpp
    CaseConverter.cpp
    DirectoryLoader.cpp
    EngineController.cpp
    EngineObserver.cpp
    Expression.cpp
    KNotificationBackend.cpp
    MainWindow.cpp
    MediaDeviceCache.cpp
    MediaDeviceMonitor.cpp
    PluginManager.cpp
    SmartPointerList.cpp
    StarManager.cpp
    SvgHandler.cpp
    SvgTinter.cpp
    covermanager/CoverFetcher.cpp
    covermanager/CoverFetchingActions.cpp
    covermanager/CoverFetchQueue.cpp
    covermanager/CoverFetchUnit.cpp
    covermanager/CoverFoundDialog.cpp
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
    dialogs/OrganizeCollectionDialog.cpp
    dialogs/ScriptSelector.cpp
    dialogs/ScriptManager.cpp
    dialogs/ScriptUpdater.cpp
    dialogs/TagDialog.cpp
    dialogs/TagGuesser.cpp
    dialogs/LabelListModel.cpp
    browsers/filebrowser/FileBrowser.cpp
    browsers/filebrowser/MyDirOperator.cpp
    browsers/filebrowser/kbookmarkhandler.cpp
    firstruntutorial/FirstRunTutorial.cpp
    firstruntutorial/FirstRunTutorialPage.cpp
    MountPointManager.cpp
    OpmlParser.cpp
    PaletteHandler.cpp
    PopupDropperFactory.cpp
    toolbar/CurrentTrackToolbar.cpp
    toolbar/MainToolbar.cpp
    toolbar/SlimToolbar.cpp
    toolbar/VolumePopupButton.cpp
    TrayIcon.cpp
    widgets/AmarokDockWidget.cpp
    widgets/BreadcrumbItemButton.cpp
    widgets/CoverLabel.cpp
    widgets/hintlineedit.cpp
    widgets/kdatecombo.cpp
    widgets/kratingpainter.cpp
    widgets/kratingwidget.cpp
    widgets/TokenDropTarget.cpp
    widgets/EditDeleteComboBoxView.cpp
    widgets/EditDeleteDelegate.cpp
    widgets/ElidingButton.cpp
    widgets/ExpandingControlsWidget.cpp
    widgets/FlowLayout.cpp
    widgets/HorizontalDivider.cpp
    widgets/LineEdit.cpp
    widgets/MainControlsButton.cpp
    widgets/MainControlsWidget.cpp
    widgets/Osd.cpp
    widgets/PixmapViewer.cpp
    widgets/PrettyTreeView.cpp
    widgets/ProgressWidget.cpp
    widgets/SearchWidget.cpp
    widgets/SliderWidget.cpp
    widgets/TokenPool.cpp
    widgets/Token.cpp
    widgets/TokenWithLayout.cpp
    widgets/VolumeWidget.cpp
    widgets/Splitter.cpp
    widgets/BookmarkTriangle.cpp
    widgets/BookmarkPopup.cpp
    GlobalCollectionActions.cpp
    GlobalCurrentTrackActions.cpp
    moodbar/MoodbarManager.cpp
)

if( BUILD_MYSQLE_COLLECTION )
else( BUILD_MYSQLE_COLLECTION )
        set(amaroklib_LIB_SRCS
            ${amaroklib_LIB_SRCS}
            MySqlServerTester.cpp
           )
endif( BUILD_MYSQLE_COLLECTION )

if(CMAKE_BUILD_TYPE MATCHES debugfull)
    set(amaroklib_LIB_SRCS
        ${amaroklib_LIB_SRCS}
        ../tests/meta/TestM3UPlaylist.cpp
        ../tests/meta/TestMetaTrack.cpp
        ../tests/meta/TestPLSPlaylist.cpp
        ../tests/meta/TestPlaylistFileSupport.cpp
        ../tests/meta/TestXSPFPlaylist.cpp
        ../tests/meta/cue/TestMetaCueCueFileItem.cpp
        #../tests/meta/cue/TestMetaCueTrack.cpp
        ../tests/meta/file/TestMetaFileTrack.cpp
        ../tests/meta/multi/TestMetaMultiTrack.cpp
        ../tests/playlistmanager/file/TestPlaylistFileProvider.cpp
        ../tests/playlistmanager/sql/TestSqlUserPlaylistProvider.cpp
        ../tests/timecode/TestTimecodeTrackProvider.cpp
        ../tests/TestAmarok.cpp
        ../tests/TestBase.cpp
        ../tests/TestCaseConverter.cpp
        ../tests/TestDirectoryLoader.cpp
    )
    include_directories(
        ../tests/
        ../tests/meta/
        ../tests/meta/cue/
        ../tests/meta/file/
        ../tests/meta/multi/
        ../tests/playlistmanager/
        ../tests/playlistmanager/file/
        ../tests/playlistmanager/sql/
        ../tests/timecode/
    )
endif(CMAKE_BUILD_TYPE MATCHES debugfull)

if( LIBLASTFM_FOUND )
    set(amaroklib_LIB_SRCS
        ${amaroklib_LIB_SRCS}
        LastfmReadLabelCapability.cpp
    )
    include_directories( ${LIBLASTFM_INCLUDE_DIR}/..  ${LIBLASTFM_INCLUDE_DIR})
    set( EXTRA_LIBS ${LIBLASTFM_LIBRARY} )
endif( LIBLASTFM_FOUND )

qt4_add_dbus_adaptor( amaroklib_LIB_SRCS dbus/org.freedesktop.MediaPlayer.root.xml RootDBusHandler.h Amarok::RootDBusHandler RootAdaptor RootAdaptor)
qt4_add_dbus_adaptor( amaroklib_LIB_SRCS dbus/org.freedesktop.MediaPlayer.player.xml PlayerDBusHandler.h Amarok::PlayerDBusHandler PlayerAdaptor PlayerAdaptor)
qt4_add_dbus_adaptor( amaroklib_LIB_SRCS dbus/org.freedesktop.MediaPlayer.tracklist.xml TracklistDBusHandler.h Amarok::TracklistDBusHandler TracklistAdaptor TracklistAdaptor)
qt4_add_dbus_adaptor( amaroklib_LIB_SRCS dbus/org.kde.amarok.Collection.xml CollectionDBusHandler.h CollectionDBusHandler CollectionAdaptor CollectionAdaptor)

set( amaroklib_DEPENDS "amarokpud" )

kde4_add_kcfg_files(amaroklib_LIB_SRCS amarokconfig.kcfgc)

kde4_add_ui_files(amaroklib_LIB_SRCS
    aboutdialog/OcsPersonItem.ui
    dialogs/EditCoverSearchDialog.ui
    dialogs/EditFilterDialog.ui
    dialogs/ScriptManagerBase.ui
    dialogs/TagDialogBase.ui
    dialogs/FilenameLayoutDialog.ui
    dialogs/OrganizeCollectionDialogBase.ui
    playlist/layouts/PlaylistLayoutEditDialog.ui
    podcasts/sql/PodcastSettingsBase.ui
    browsers/playlistbrowser/PodcastCategoryBase.ui
)

kde4_add_library(amaroklib SHARED ${amaroklib_LIB_SRCS})

target_link_libraries(amaroklib
    ${KDE4_KDEUI_LIBS}
    ${KDE4_KFILE_LIBS}
    ${KDE4_KHTML_LIBS}
    ${KDE4_KNEWSTUFF2_LIBS}
    ${KDE4_KNEWSTUFF_LIBS}
    ${KDE4_KNOTIFYCONFIG_LIBS}
    ${KDE4_KUTILS_LIBS}
    ${KDE4_PHONON_LIBRARY}
    ${KDE4_PLASMA_LIBS}
    ${KDE4_SOLID_LIBRARY}
    ${KDE4_THREADWEAVER_LIBRARIES}
    ${TAGLIB_LIBRARIES}
    ${TAGLIB-EXTRAS_LIBRARIES}
    ${QT_QTSCRIPT_LIBRARY}
    ${QT_QTSQL_LIBRARY}
    ${QT_QTWEBKIT_LIBRARY}
    ${QCA2_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${EXTRA_LIBS}
    amarokpud
    amarokocsclient
)

if (KDE_VERSION VERSION_GREATER "4.3.80")
    target_link_libraries(amaroklib ${KDE4_KNEWSTUFF3_LIBS})
endif (KDE_VERSION VERSION_GREATER "4.3.80")

if(CMAKE_BUILD_TYPE MATCHES debugfull)
    target_link_libraries(amaroklib ${QT_QTTEST_LIBRARY})
endif(CMAKE_BUILD_TYPE MATCHES debugfull)

if( BUILD_MYSQLE_COLLECTION )
else( BUILD_MYSQLE_COLLECTION )
    target_link_libraries(amaroklib ${MYSQL_LIBRARIES} ${ZLIB_LIBRARIES})
endif( BUILD_MYSQLE_COLLECTION )

if(WIN32)
    target_link_libraries(amaroklib ${QT_QTWEBKIT_LIBRARY})
endif(WIN32)
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
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}")

kde4_add_app_icon(amarok_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/images/hi*-app-amarok.png)

if(Q_WS_MAC)
    kde4_add_executable(Amarok ${amarok_SRCS})
    target_link_libraries(Amarok ${KDE4_KDECORE_LIBS} amaroklib )
    SET_TARGET_PROPERTIES(Amarok PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
    set_target_properties(Amarok PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${MAC_FILES_DIR}/Info.plist.template)
    set_target_properties(Amarok PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.amarok")
    set_target_properties(Amarok PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "Amarok 2")
    #the following line was copied from KDE4Macros.cmake. kde4_add_app_icon fails to actually copy the .icns file
    SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_BINARY_DIR}/amarok.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    install(TARGETS Amarok ${INSTALL_TARGETS_DEFAULT_ARGS})
else(Q_WS_MAC)
    kde4_add_executable(amarok ${amarok_SRCS})
    target_link_libraries(amarok ${KDE4_KDEUI_LIBS} amaroklib ${TAGLIB_LIBRARIES} ${TAGLIB-EXTRAS_LIBRARIES} )

    install(TARGETS amarok ${INSTALL_TARGETS_DEFAULT_ARGS})
endif(Q_WS_MAC)

########### install files ###############

install(FILES amarok.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} )
install(FILES amarok_plugin.desktop DESTINATION ${SERVICETYPES_INSTALL_DIR})
install(FILES amarok_codecinstall.desktop DESTINATION ${SERVICETYPES_INSTALL_DIR})
install(FILES amarok_append.desktop DESTINATION ${SERVICES_INSTALL_DIR}/ServiceMenus)
install(FILES amarok-play-audiocd.desktop DESTINATION ${DATA_INSTALL_DIR}/solid/actions)
install(FILES amarok.knsrc DESTINATION ${CONFIG_INSTALL_DIR})
install(FILES context/amarokapplets.knsrc DESTINATION ${CONFIG_INSTALL_DIR})

#install(FILES amarokitpc.protocol DESTINATION ${SERVICES_INSTALL_DIR})
#install(FILES amarokpcast.protocol DESTINATION ${SERVICES_INSTALL_DIR})

install(FILES context/servicetypes/amarok_context_applet.desktop
              context/servicetypes/amarok_data_engine.desktop
              DESTINATION ${SERVICETYPES_INSTALL_DIR} )

install(FILES  amarokconfig.kcfg DESTINATION ${KCFG_INSTALL_DIR} )
install(FILES dbus/org.freedesktop.MediaPlayer.root.xml
              dbus/org.freedesktop.MediaPlayer.player.xml
              dbus/org.freedesktop.MediaPlayer.tracklist.xml
              dbus/org.kde.amarok.Collection.xml
              DESTINATION ${DBUS_INTERFACES_INSTALL_DIR})


install(FILES data/DefaultPlaylistLayouts.xml
              services/InfoParserLoading.html
              browsers/hover_info_template.html
              DESTINATION ${DATA_INSTALL_DIR}/amarok/data)

install( FILES data/amarok_homerc DESTINATION ${CONFIG_INSTALL_DIR} )
install( FILES data/amarok.notifyrc DESTINATION ${DATA_INSTALL_DIR}/amarok )

kde4_install_icons( ${ICON_INSTALL_DIR} )

############ amarok protocol handler ###########

install( FILES amarokurls/amarok.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )



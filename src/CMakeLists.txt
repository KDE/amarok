# Improves speed of string concatenation
add_definitions(-DQT_USE_FAST_CONCATENATION)
add_definitions(-DQT_USE_FAST_OPERATOR_PLUS)
if(NOT MSVC)
    add_definitions(-DQT_STRICT_ITERATORS)
endif(NOT MSVC)

if(APPLE)
    set(mac_SRCS
        app_mac.cpp
        mac/GrowlInterface.cpp
    )

    include_directories ( services/lastfm/ )
    set( MAC_FILES_DIR  ${CMAKE_SOURCE_DIR}/src/mac )
endif(APPLE)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

add_subdirectory( core )
add_subdirectory( core-impl/collections )
#don't use our own libplasma anymore, but still pick up our applets/engines/containments
add_subdirectory( context )
add_subdirectory( services )
add_subdirectory( scripts )
add_subdirectory( aboutdialog/libattica-ocsclient )
add_subdirectory( transcoding )
add_subdirectory( kconf_update )
add_subdirectory( importers )

if ( KDEMULTIMEDIA_FOUND )
    include_directories( ${KDEMULTIMEDIA_INCLUDE_DIR} )
endif ( KDEMULTIMEDIA_FOUND )

#####################################################################
# PROXYCOLLECTION
#####################################################################
set(aggregatecollection_SRCS
    core-impl/collections/aggregate/AggregateCollection.cpp
    core-impl/collections/aggregate/AggregateMeta.cpp
    core-impl/collections/aggregate/AggregateQueryMaker.cpp
)

#####################################################################
# MEDIADEVICEFRAMEWORK
#####################################################################
set(libmediadeviceframework_SRCS
  core-impl/collections/mediadevicecollection/MediaDeviceCollection.cpp
  core-impl/collections/mediadevicecollection/MediaDeviceCollectionLocation.cpp
  core-impl/collections/mediadevicecollection/MediaDeviceMeta.cpp
  core-impl/collections/mediadevicecollection/MediaDeviceTrackEditor.cpp
  core-impl/collections/mediadevicecollection/handler/MediaDeviceHandler.cpp
  core-impl/collections/mediadevicecollection/handler/MediaDeviceHandlerCapability.cpp
  core-impl/collections/mediadevicecollection/handler/capabilities/ArtworkCapability.cpp
  core-impl/collections/mediadevicecollection/handler/capabilities/PlaylistCapability.cpp
  core-impl/collections/mediadevicecollection/handler/capabilities/PodcastCapability.cpp
  core-impl/collections/mediadevicecollection/handler/capabilities/ReadCapability.cpp
  core-impl/collections/mediadevicecollection/handler/capabilities/WriteCapability.cpp
  core-impl/collections/mediadevicecollection/playlist/MediaDevicePlaylist.cpp
  core-impl/collections/mediadevicecollection/playlist/MediaDeviceUserPlaylistProvider.cpp
  core-impl/collections/mediadevicecollection/podcast/MediaDevicePodcastProvider.cpp
  core-impl/collections/mediadevicecollection/support/ConnectionAssistant.cpp
  core-impl/collections/mediadevicecollection/support/MediaDeviceInfo.cpp
)

#####################################################################
# SERVICEFRAMEWORK
#####################################################################
set(libserviceframework_SRCS
    services/DynamicServiceQueryMaker.cpp
    services/InfoParserBase.cpp
    services/ServiceAlbumCoverDownloader.cpp
    services/ServiceBase.cpp
    services/ServiceCapabilities.cpp
    services/ServiceCollection.cpp
    services/ServiceCollectionLocation.cpp
    services/ServiceCollectionTreeView.cpp
    services/ServiceMetaBase.cpp
    services/ServicePluginManager.cpp
    services/ServiceSqlCollection.cpp
    services/ServiceSqlQueryMaker.cpp
    services/ServiceSqlRegistry.cpp
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
    )

#####################################################################
# SCRIPTABLESERVICE
#####################################################################
set(libscriptableservice_SRCS
    services/scriptable/ScriptableService.cpp
    services/scriptable/ScriptableServiceCollection.cpp
    services/scriptable/ScriptableServiceCollectionTreeModel.cpp
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
    configdialog/dialogs/ExcludedLabelsDialog.cpp
    configdialog/dialogs/GeneralConfig.cpp
    configdialog/dialogs/MetadataConfig.cpp
    configdialog/dialogs/NotificationsConfig.cpp
    configdialog/dialogs/PlaybackConfig.cpp
    configdialog/dialogs/PluginsConfig.cpp
    configdialog/dialogs/ScriptsConfig.cpp
    configdialog/dialogs/ScriptSelector.cpp
    configdialog/dialogs/DatabaseConfig.cpp
)

kde4_add_ui_files(libconfigdialog_SRCS
    configdialog/dialogs/CollectionConfig.ui
    configdialog/dialogs/GeneralConfig.ui
    configdialog/dialogs/MetadataConfig.ui
    configdialog/dialogs/ExcludedLabelsDialog.ui
    configdialog/dialogs/NotificationsConfig.ui
    configdialog/dialogs/PlaybackConfig.ui
    configdialog/dialogs/DatabaseConfig.ui
    configdialog/dialogs/ScriptsConfig.ui
)

set(libbrowserframework_SRCS
    browsers/BrowserBreadcrumbItem.cpp
    browsers/BrowserBreadcrumbWidget.cpp
    browsers/BrowserCategory.cpp
    browsers/BrowserCategoryList.cpp
    browsers/BrowserCategoryListModel.cpp
    browsers/BrowserCategoryListSortFilterProxyModel.cpp
    browsers/BrowserDock.cpp
    browsers/BrowserMessageArea.cpp
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
# STATUSBAR
#####################################################################
set(libstatusbar_SRCS
    statusbar/ProgressBar.cpp
    statusbar/KJobProgressBar.cpp
    statusbar/NetworkProgressBar.cpp
    statusbar/CompoundProgressBar.cpp
    statusbar/PopupWidget.cpp
    statusbar/LongMessageWidget.cpp
)


#####################################################################
# META
#####################################################################
set(libmetaimpl_SRCS
    core-impl/playlists/providers/user/UserPlaylistProvider.cpp
    core-impl/playlists/types/file/asx/ASXPlaylist.cpp
    core-impl/playlists/types/file/m3u/M3UPlaylist.cpp
    core-impl/playlists/types/file/pls/PLSPlaylist.cpp
    core-impl/playlists/types/file/PlaylistFileLoaderJob.cpp
    core-impl/playlists/types/file/PlaylistFileSupport.cpp
    core-impl/playlists/types/file/xspf/XSPFPlaylist.cpp
    core-impl/capabilities/AlbumActionsCapability.cpp
    core-impl/capabilities/timecode/TimecodeBoundedPlaybackCapability.cpp
    core-impl/capabilities/timecode/TimecodeLoadCapability.cpp
    core-impl/capabilities/timecode/TimecodeWriteCapability.cpp
    core-impl/capabilities/multisource/MultiSourceCapabilityImpl.cpp
    core-impl/meta/file/File.cpp
    core-impl/meta/file/FileTrackProvider.cpp
    core-impl/meta/multi/MultiTrack.cpp
    core-impl/meta/cue/CueFileSupport.cpp
    core-impl/meta/proxy/MetaProxy.cpp
    core-impl/meta/proxy/MetaProxyWorker.cpp
    core-impl/meta/stream/Stream.cpp
    core-impl/playlists/types/file/PlaylistFile.cpp
    core-impl/support/PersistentStatisticsStore.cpp
    core-impl/support/TagStatisticsStore.cpp
    core-impl/support/UrlStatisticsStore.cpp
)

#####################################################################
# COLLECTION
#####################################################################
set(collection_SRCS
    core-impl/collections/support/jobs/WriteTagsJob.cpp
    core-impl/collections/support/ArtistHelper.cpp
    core-impl/collections/support/CollectionManager.cpp
    core-impl/collections/support/CollectionLocationDelegateImpl.cpp
    core-impl/collections/support/MemoryCustomValue.cpp
    core-impl/collections/support/MemoryFilter.cpp
    core-impl/collections/support/MemoryMatcher.cpp
    core-impl/collections/support/MemoryMeta.cpp
    core-impl/collections/support/MemoryQueryMaker.cpp
    core-impl/collections/support/MemoryQueryMakerInternal.cpp
    core-impl/collections/support/MemoryQueryMakerHelper.cpp
    core-impl/collections/support/TrashCollectionLocation.cpp
    core-impl/collections/support/XmlQueryReader.cpp
    core-impl/collections/support/FileCollectionLocation.cpp
    core-impl/collections/support/Expression.cpp
    core-impl/collections/support/TextualQueryFilter.cpp
)

#####################################################################
# SCANNER
#####################################################################
set( scanner_SRCS
    scanner/GenericScanManager.cpp
    scanner/GenericScannerJob.cpp
    scanner/AbstractDirectoryWatcher.cpp
    scanner/AbstractScanResultProcessor.cpp
)

#####################################################################
# CONTEXT
#####################################################################
set( libcontextview_SRCS
    context/Applet.cpp
    context/Containment.cpp
    context/ContextObserver.cpp
    context/ContextScene.cpp
    context/ContextDock.cpp
    context/ContextView.cpp
    context/LyricsManager.cpp
    context/ToolbarView.cpp
    context/toolbar/AppletItemOverlay.cpp
    context/toolbar/AppletToolbar.cpp
    context/toolbar/AppletToolbarAddItem.cpp
    context/toolbar/AppletToolbarAppletItem.cpp
    context/toolbar/AppletToolbarBase.cpp
    context/toolbar/AppletToolbarConfigItem.cpp
    context/widgets/AppletHeader.cpp
    context/widgets/RatingWidget.cpp
    context/widgets/TextScrollingWidget.cpp
    context/widgets/DropPixmapItem.cpp
    context/widgets/ToolBoxIcon.cpp
    # context/widgets/ContainmentArrow.cpp
    # context/widgets/ContainmentSelectionLayer.cpp
    context/widgets/appletexplorer/AppletExplorer.cpp
    context/widgets/appletexplorer/AppletIcon.cpp
    context/widgets/RecentlyPlayedListWidget.cpp
)

#####################################################################
# PODCASTS
#####################################################################
set(libpodcasts_SRCS
    core-impl/podcasts/sql/SqlPodcastMeta.cpp
    core-impl/podcasts/sql/SqlPodcastProvider.cpp
    core-impl/podcasts/sql/PodcastSettingsDialog.cpp
    core-impl/podcasts/sql/PodcastFilenameLayoutConfigDialog.cpp
)

#####################################################################
# PLAYLISTBROWSER
#####################################################################
set(libplaylistbrowser_SRCS
    browsers/playlistbrowser/APGCategory.cpp
    browsers/playlistbrowser/DynamicCategory.cpp
    browsers/playlistbrowser/DynamicBiasDelegate.cpp
    browsers/playlistbrowser/DynamicBiasWidgets.cpp
    browsers/playlistbrowser/DynamicView.cpp
    browsers/playlistbrowser/PlaylistBrowserFilterProxy.cpp
    browsers/playlistbrowser/PlaylistBrowserModel.cpp
    browsers/playlistbrowser/PlaylistBrowserCategory.cpp
    browsers/playlistbrowser/QtGroupingProxy.cpp
    browsers/playlistbrowser/PlaylistBrowser.cpp
    browsers/playlistbrowser/PlaylistBrowserView.cpp
    browsers/playlistbrowser/UserPlaylistCategory.cpp
    browsers/playlistbrowser/PlaylistsInFoldersProxy.cpp
    browsers/playlistbrowser/PlaylistsByProviderProxy.cpp
    browsers/playlistbrowser/PodcastModel.cpp
    browsers/playlistbrowser/PodcastCategory.cpp
    browsers/playlistbrowser/UserPlaylistModel.cpp
)

#####################################################################
# PLAYLISTMANAGER
#####################################################################
set(libplaylistmanager_SRCS
    playlistmanager/PlaylistManager.cpp
    playlistmanager/file/PlaylistFileProvider.cpp
    playlistmanager/file/KConfigSyncRelStore.cpp
    playlistmanager/sql/SqlUserPlaylistProvider.cpp
    playlistmanager/sql/SqlPlaylist.cpp
    playlistmanager/sql/SqlPlaylistGroup.cpp
    playlistmanager/SyncedPlaylist.cpp
    playlistmanager/SyncedPodcast.cpp
    playlistmanager/SyncRelationStorage.cpp
)

#####################################################################
# PLAYLIST
#####################################################################
set(libplaylist_SRCS
    playlist/PlaylistActions.cpp
    playlist/PlaylistBreadcrumbItem.cpp
    playlist/PlaylistBreadcrumbItemSortButton.cpp
    playlist/PlaylistBreadcrumbLevel.cpp
    playlist/PlaylistDefines.cpp
    playlist/PlaylistController.cpp
    playlist/PlaylistInfoWidget.cpp
    playlist/PlaylistItem.cpp
    playlist/PlaylistModel.cpp
    playlist/PlaylistModelStack.cpp
    playlist/PlaylistRestorer.cpp
    playlist/PlaylistQueueEditor.cpp
    playlist/PlaylistSortWidget.cpp
    playlist/PlaylistViewUrlGenerator.cpp
    playlist/PlaylistViewUrlRunner.cpp
    playlist/PlaylistDock.cpp
    playlist/PlaylistToolBar.cpp
    playlist/ProgressiveSearchWidget.cpp
    playlist/UndoCommands.cpp
    playlist/layouts/LayoutEditDialog.cpp
    playlist/layouts/LayoutEditWidget.cpp
    playlist/layouts/LayoutConfigAction.cpp
    playlist/layouts/LayoutItemConfig.cpp
    playlist/layouts/LayoutManager.cpp
    playlist/layouts/PlaylistLayoutEditDialog.cpp
    playlist/navigators/AlbumNavigator.cpp
    playlist/navigators/DynamicTrackNavigator.cpp
    playlist/navigators/FavoredRandomTrackNavigator.cpp
    playlist/navigators/NavigatorConfigAction.cpp
    playlist/navigators/NonlinearTrackNavigator.cpp
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
    playlist/proxymodels/GroupingProxy.cpp
    playlist/proxymodels/ProxyBase.cpp
    playlist/proxymodels/SortAlgorithms.cpp
    playlist/proxymodels/SortFilterProxy.cpp
    playlist/proxymodels/SortScheme.cpp
    playlist/proxymodels/SearchProxy.cpp
)

kde4_add_ui_files(libplaylist_SRCS
    playlist/PlaylistQueueEditor.ui
)

#####################################################################
# DYNAMIC
#####################################################################
set(libdynamic_SRCS
    dynamic/TrackSet.cpp
    dynamic/BiasFactory.cpp
    dynamic/BiasedPlaylist.cpp
    dynamic/BiasSolver.cpp
    dynamic/DynamicPlaylist.cpp
    dynamic/DynamicModel.cpp

    # biases
    dynamic/Bias.cpp
    dynamic/biases/AlbumPlayBias.cpp
    dynamic/biases/EchoNestBias.cpp
    dynamic/biases/IfElseBias.cpp
    dynamic/biases/PartBias.cpp
    dynamic/biases/QuizPlayBias.cpp
    dynamic/biases/TagMatchBias.cpp
    dynamic/biases/SearchQueryBias.cpp
)

#####################################################################
# DBUS
#####################################################################
set(dbus_SRCS
    dbus/mpris1/RootHandler.cpp
    dbus/mpris1/PlayerHandler.cpp
    dbus/mpris1/TrackListHandler.cpp
    dbus/mpris2/DBusAbstractAdaptor.cpp
    dbus/mpris2/Mpris2.cpp
    dbus/mpris2/MediaPlayer2.cpp
    dbus/mpris2/MediaPlayer2Player.cpp
    dbus/mpris2/MediaPlayer2AmarokExtensions.cpp
    dbus/mpris2/DBusAmarokApp.cpp
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
    scriptengine/AmarokKNotifyScript.cpp
    scriptengine/AmarokOSDScript.cpp
    scriptengine/AmarokPlaylistScript.cpp
    scriptengine/AmarokStatusbarScript.cpp
    scriptengine/AmarokWindowScript.cpp
    scriptengine/AmarokLyricsScript.cpp
    scriptengine/MetaTypeExporter.cpp
    scriptengine/ScriptImporter.cpp
)

#####################################################################
# PLAYLIST GENERATOR
#####################################################################
set(apg_SRCS
    playlistgenerator/Constraint.cpp
    playlistgenerator/ConstraintGroup.cpp
    playlistgenerator/ConstraintFactory.cpp
    playlistgenerator/ConstraintNode.cpp
    playlistgenerator/ConstraintSolver.cpp
    playlistgenerator/Preset.cpp
    playlistgenerator/PresetEditDialog.cpp
    playlistgenerator/PresetModel.cpp
    playlistgenerator/TreeController.cpp
    playlistgenerator/TreeModel.cpp
    playlistgenerator/constraints/Checkpoint.cpp
    playlistgenerator/constraints/Matching.cpp
    playlistgenerator/constraints/PlaylistDuration.cpp
    playlistgenerator/constraints/PlaylistFileSize.cpp
    playlistgenerator/constraints/PlaylistLength.cpp
    playlistgenerator/constraints/PreventDuplicates.cpp
    playlistgenerator/constraints/TagMatch.cpp
    playlistgenerator/constraints/TagMatchSupport.cpp
    playlistgenerator/constraints/TrackSpreader.cpp
)

kde4_add_ui_files(apg_SRCS
    playlistgenerator/ConstraintGroupEditWidget.ui
    playlistgenerator/PresetEditDialog.ui
    playlistgenerator/constraints/CheckpointEditWidget.ui
    playlistgenerator/constraints/PlaylistDurationEditWidget.ui
    playlistgenerator/constraints/PlaylistFileSizeEditWidget.ui
    playlistgenerator/constraints/PlaylistLengthEditWidget.ui
    playlistgenerator/constraints/PreventDuplicatesEditWidget.ui
    playlistgenerator/constraints/TagMatchEditWidget.ui
)

#####################################################################
# NETWORK ACCESS
#####################################################################
set(network_access_SRCS
    network/NetworkAccessManagerProxy.cpp
)

if( CMAKE_BUILD_TYPE_TOLOWER MATCHES debug )
    set(network_access_SRCS
        ${network_access_SRCS}
        network/NetworkAccessViewer.cpp
        )

    kde4_add_ui_files(network_access_SRCS
        network/NetworkRequests.ui
        )
endif( CMAKE_BUILD_TYPE_TOLOWER MATCHES debug )

#####################################################################
# STATISTICS SYNCHRONIZATION
#####################################################################
set( statsyncing_SRCS
    statsyncing/Config.cpp
    statsyncing/Controller.cpp
    statsyncing/Options.cpp
    statsyncing/Process.cpp
    statsyncing/Provider.cpp
    statsyncing/ScrobblingService.cpp
    statsyncing/SimpleTrack.cpp
    statsyncing/SimpleWritableTrack.cpp
    statsyncing/Track.cpp
    statsyncing/TrackTuple.cpp
    statsyncing/collection/CollectionProvider.cpp
    statsyncing/collection/CollectionTrack.cpp
    statsyncing/jobs/MatchTracksJob.cpp
    statsyncing/jobs/SynchronizeTracksJob.cpp
    statsyncing/models/CommonModel.cpp
    statsyncing/models/MatchedTracksModel.cpp
    statsyncing/models/ProvidersModel.cpp
    statsyncing/models/SingleTracksModel.cpp
    statsyncing/ui/ChooseProvidersPage.cpp
    statsyncing/ui/CreateProviderDialog.cpp
    statsyncing/ui/ConfigureProviderDialog.cpp
    statsyncing/ui/MatchedTracksPage.cpp
    statsyncing/ui/TrackDelegate.cpp
)

kde4_add_ui_files( statsyncing_SRCS
    statsyncing/ui/ChooseProvidersPage.ui
    statsyncing/ui/MatchedTracksPage.ui
)

#####################################################################
# STATISTICS IMPORTERS
#####################################################################
set( importers_SRCS
    importers/ImporterManager.cpp
    importers/ImporterProvider.cpp
    importers/ImporterSqlProvider.cpp
    importers/ImporterSqlTrack.cpp
    importers/SimpleImporterConfigWidget.cpp
)

#####################################################################
# LIBAMAROK
#####################################################################
set(amaroklib_LIB_SRCS
    ${libscriptableservice_SRCS}
    ${libbrowserframework_SRCS}
    ${libcontextview_SRCS}
    ${libcollectionbrowser_SRCS}
    ${libconfigdialog_SRCS}
    ${libplaylist_SRCS}
    ${libtooltip_SRCS}
    ${aggregatecollection_SRCS}
    ${libpodcasts_SRCS}
    ${libmediadeviceframework_SRCS}
    ${libserviceframework_SRCS}
    ${libservicebrowser_SRCS}
    ${libdynamic_SRCS}
    ${libmetaimpl_SRCS}
    ${core_interfaces_SRCS}
    ${apg_SRCS}
    ${collection_SRCS}
    ${scanner_SRCS}
    ${mac_SRCS}
    ${network_access_SRCS}
    ${libplaylistbrowser_SRCS}
    ${libqueuemanager_SRCS}
    ${libplaylistmanager_SRCS}
    ${dbus_SRCS}
    ${scriptengine_SRCS}
    ${libstatusbar_SRCS}
    ${libamarokurl_SRCS}
    ${libsynchronization_SRCS}
    ${statsyncing_SRCS}
    ${importers_SRCS}
    core-impl/logger/ProxyLogger.cpp
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
    EngineController.cpp
    KNotificationBackend.cpp
    MainWindow.cpp
    MediaDeviceCache.cpp
    MediaDeviceMonitor.cpp
    PluginManager.cpp
    QStringx.cpp
    ScriptManager.cpp
    ScriptUpdater.cpp
    SvgHandler.cpp
    SvgTinter.cpp
    TrayIcon.cpp
    core-impl/meta/timecode/TimecodeObserver.cpp
    core-impl/meta/timecode/TimecodeMeta.cpp
    core-impl/meta/timecode/TimecodeTrackProvider.cpp
    core-impl/support/TrackLoader.cpp
    covermanager/CoverCache.cpp
    covermanager/CoverFetcher.cpp
    covermanager/CoverFetchingActions.cpp
    covermanager/CoverFetchQueue.cpp
    covermanager/CoverFetchUnit.cpp
    covermanager/CoverFoundDialog.cpp
    covermanager/CoverManager.cpp
    covermanager/CoverViewDialog.cpp
    databaseimporter/DatabaseImporter.cpp
    databaseimporter/amarok14/FastForwardImporter.cpp
    databaseimporter/amarok14/FastForwardImporterConfig.cpp
    databaseimporter/amarok14/FastForwardWorker.cpp
    databaseimporter/itunes/ITunesImporterConfig.cpp
    databaseimporter/itunes/ITunesImporter.cpp
    databaseimporter/itunes/ITunesImporterWorker.cpp
    databaseimporter/sqlbatch/SqlBatchImporter.cpp
    databaseimporter/sqlbatch/SqlBatchImporterConfig.cpp
    dialogs/CollectionSetup.cpp
    dialogs/DatabaseImporterDialog.cpp
    dialogs/DiagnosticDialog.cpp
    dialogs/EditFilterDialog.cpp
    dialogs/EqualizerDialog.cpp
    dialogs/MusicBrainzTagger.cpp
    dialogs/OrganizeCollectionDialog.cpp
    dialogs/TrackOrganizer.cpp
    dialogs/TagDialog.cpp
    dialogs/TagGuesser.cpp
    dialogs/TagGuesserDialog.cpp
    dialogs/LabelListModel.cpp
    equalizer/EqualizerPresets.cpp
    browsers/filebrowser/DirPlaylistTrackFilterProxyModel.cpp
    browsers/filebrowser/FileBrowser.cpp
    browsers/filebrowser/FileView.cpp
    musicbrainz/MusicBrainzFinder.cpp
    musicbrainz/MusicBrainzTagsItem.cpp
    musicbrainz/MusicBrainzTagsModel.cpp
    musicbrainz/MusicBrainzTagsModelDelegate.cpp
    musicbrainz/MusicBrainzTagsView.cpp
    musicbrainz/MusicBrainzXmlParser.cpp
    OpmlOutline.cpp
    OpmlParser.cpp
    OpmlWriter.cpp
    PaletteHandler.cpp
    PopupDropperFactory.cpp
    playback/DelayedDoers.cpp
    playback/Fadeouter.cpp
    playback/PowerManager.cpp
    statemanagement/ApplicationController.cpp
    statemanagement/DefaultApplicationController.cpp
    toolbar/CurrentTrackToolbar.cpp
    toolbar/SlimToolbar.cpp
    toolbar/VolumePopupButton.cpp
    toolbar/MainToolbar.cpp
    widgets/AlbumBreadcrumbWidget.cpp
    widgets/AmarokDockWidget.cpp
    widgets/AnimatedLabelStack.cpp
    widgets/BreadcrumbItemButton.cpp
    widgets/ClearSpinBox.cpp
    widgets/CoverLabel.cpp
    widgets/HintLineEdit.cpp
    widgets/kdatecombo.cpp
    widgets/TokenDropTarget.cpp
    widgets/EditDeleteComboBoxView.cpp
    widgets/EditDeleteDelegate.cpp
    widgets/ElidingButton.cpp
    widgets/FilenameLayoutWidget.cpp
    widgets/FlowLayout.cpp
    widgets/HorizontalDivider.cpp
    widgets/IconButton.cpp
    widgets/ComboBox.cpp
    widgets/LineEdit.cpp
    widgets/Osd.cpp
    widgets/TimeLabel.cpp
    widgets/PixmapViewer.cpp
    widgets/PlayPauseButton.cpp
    widgets/PrettyTreeView.cpp
    widgets/PrettyTreeDelegate.cpp
    widgets/ProgressWidget.cpp
    widgets/SearchWidget.cpp
    widgets/SliderWidget.cpp
    widgets/StarManager.cpp
    widgets/TokenPool.cpp
    widgets/Token.cpp
    widgets/TokenWithLayout.cpp
    widgets/VolumeDial.cpp
    widgets/TrackActionButton.cpp
    widgets/BookmarkTriangle.cpp
    widgets/BookmarkPopup.cpp
    widgets/TrackSelectWidget.cpp
    widgets/MetaQueryWidget.cpp
    GlobalCollectionActions.cpp
    GlobalCurrentTrackActions.cpp
    moodbar/MoodbarManager.cpp
)

if( NOT BUILD_MYSQLE_COLLECTION )
    set(amaroklib_LIB_SRCS
        ${amaroklib_LIB_SRCS}
        MySqlServerTester.cpp
    )
    add_definitions(${MYSQL_CFLAGS})
    include_directories(${MYSQL_INCLUDE_DIR})
endif( NOT BUILD_MYSQLE_COLLECTION )

if( LIBMYGPO_QT_FOUND )
    set( EXTRA_LIBS ${LIBMYGPO_QT_LIBRARIES} )
    include_directories(     ${LIBMYGPO_QT_INCLUDE_DIRS} ${LIBMYGPO_QT_INCLUDE_DIRS}/../ )
endif( LIBMYGPO_QT_FOUND )

if( LIBLASTFM_FOUND )
    set(amaroklib_LIB_SRCS
        ${amaroklib_LIB_SRCS}
        LastfmReadLabelCapability.cpp
    )
    include_directories( ${LIBLASTFM_INCLUDE_DIR}/..  ${LIBLASTFM_INCLUDE_DIR})
    set( EXTRA_LIBS ${LIBLASTFM_LIBRARY} )
endif( LIBLASTFM_FOUND )

if( LIBOFA_FOUND )
    add_definitions(
        ${AVCODEC_DEFINITIONS}
        ${AVFORMAT_DEFINITIONS}
        ${AVUTIL_DEFINITIONS}
    )
    include_directories(
        ${AVCODEC_INCLUDE_DIRS}
        ${AVFORMAT_INCLUDE_DIRS}
        ${AVUTIL_INCLUDE_DIRS}
    )
    set( EXTRA_LIBS
        ${EXTRA_LIBS}
        ${LIBOFA_LIBRARY}
        ${AVFORMAT_LIBRARIES}
        ${AVCODEC_LIBRARIES}
        ${AVUTIL_LIBRARIES}
    )
    set( amaroklib_LIB_SRCS
        ${amaroklib_LIB_SRCS}
        musicbrainz/MusicDNSAudioDecoder.cpp
        musicbrainz/MusicDNSFinder.cpp
        musicbrainz/MusicDNSXmlParser.cpp
    )
endif( LIBOFA_FOUND )

qt4_add_dbus_adaptor(
   AMAROK_MPRIS1_ADAPTOR_SRCS
   dbus/mpris1/org.freedesktop.MediaPlayer.root.xml
   dbus/mpris1/RootHandler.h
   Mpris1::RootHandler
   Mpris1RootAdaptor
   Mpris1RootAdaptor )
qt4_add_dbus_adaptor(
   AMAROK_MPRIS1_ADAPTOR_SRCS
   dbus/mpris1/org.freedesktop.MediaPlayer.player.xml
   dbus/mpris1/PlayerHandler.h
   Mpris1::PlayerHandler
   Mpris1PlayerAdaptor
   Mpris1PlayerAdaptor )
qt4_add_dbus_adaptor(
   AMAROK_MPRIS1_ADAPTOR_SRCS
   dbus/mpris1/org.freedesktop.MediaPlayer.tracklist.xml
   dbus/mpris1/TrackListHandler.h
   Mpris1::TrackListHandler
   Mpris1TrackListAdaptor
   Mpris1TrackListAdaptor )
qt4_add_dbus_adaptor(
   AMAROK_MPRIS1_ADAPTOR_SRCS
   dbus/org.kde.amarok.App.xml
   dbus/mpris1/RootHandler.h
   Mpris1::RootHandler
   Mpris1AmarokAppAdaptor
   Mpris1AmarokAppAdaptor )
qt4_add_dbus_adaptor(
   AMAROK_MPRIS1_ADAPTOR_SRCS
   dbus/mpris1/org.kde.amarok.Mpris1Extensions.Player.xml
   dbus/mpris1/PlayerHandler.h
   Mpris1::PlayerHandler
   Mpris1AmarokPlayerAdaptor
   Mpris1AmarokPlayerAdaptor )

qt4_add_dbus_adaptor(
   AMAROK_MPRIS1_ADAPTOR_SRCS
   dbus/org.kde.amarok.Collection.xml
   dbus/CollectionDBusHandler.h
   CollectionDBusHandler
   CollectionAdaptor
   CollectionAdaptor )

# suppress deprecated methods warnings
if(NOT WIN32)
    set_source_files_properties(${AMAROK_MPRIS1_ADAPTOR_SRCS}
                                PROPERTIES
                                COMPILE_FLAGS -Wno-deprecated-declarations)
endif(NOT WIN32)

set( amaroklib_DEPENDS "amarokpud" )
set( amaroklib_DEPENDS "amarokcore" )
set( amaroklib_DEPENDS "amarok-transcoding" ) # depends on generated ui_*.h file

kde4_add_kcfg_files(amaroklib_LIB_SRCS amarokconfig.kcfgc)
add_custom_target(amarokconfig_h DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/amarokconfig.h)

kde4_add_ui_files(amaroklib_LIB_SRCS
    aboutdialog/OcsPersonItem.ui
    dialogs/EditFilterDialog.ui
    dialogs/EqualizerDialog.ui
    dialogs/MusicBrainzTagger.ui
    dialogs/TagDialogBase.ui
    dialogs/TagGuessOptions.ui
    dialogs/OrganizeCollectionOptions.ui
    dialogs/OrganizeCollectionDialogBase.ui
    playlist/layouts/PlaylistLayoutEditDialog.ui
    core-impl/podcasts/sql/PodcastSettingsBase.ui
    core-impl/podcasts/sql/SqlPodcastProviderSettingsWidget.ui
    core-impl/podcasts/sql/PodcastFilenameLayoutConfigWidget.ui
    browsers/playlistbrowser/PodcastCategoryBase.ui
)

kde4_add_library(amaroklib SHARED ${amaroklib_LIB_SRCS} ${AMAROK_MPRIS1_ADAPTOR_SRCS})

target_link_libraries(amaroklib
    ${KDE4_KDEUI_LIBS}
    ${KDE4_KFILE_LIBS}
    ${KDE4_KNEWSTUFF3_LIBS}
    ${KDE4_KNOTIFYCONFIG_LIBS}
    ${KDE4_KUTILS_LIBS}
    ${KDE4_PHONON_LIBRARY}
    ${KDE4_PLASMA_LIBS}
    ${KDE4_SOLID_LIBRARY}
    ${KDE4_THREADWEAVER_LIBRARIES}
    ${QT_QTSCRIPT_LIBRARY}
    ${QT_QTSQL_LIBRARY}
    ${CMAKE_THREAD_LIBS_INIT}
    ${EXTRA_LIBS}
    amarokpud
    amarokcore
    amarokocsclient
    amarok-transcoding
    amarokshared
)

include_directories(${TAGLIB_INCLUDES})
add_definitions(${TAGLIB_CFLAGS})
target_link_libraries(amaroklib ${TAGLIB_LIBRARIES})
if( TAGLIB-EXTRAS_FOUND )
    include_directories(${TAGLIB-EXTRAS_INCLUDES})
    add_definitions(${TAGLIB-EXTRAS_CFLAGS})
    target_link_libraries(amaroklib ${TAGLIB-EXTRAS_LIBRARIES})
endif( TAGLIB-EXTRAS_FOUND )

if( QCA2_FOUND )
    include_directories( ${QCA2_INCLUDE_DIR} )
    target_link_libraries(amaroklib ${QCA2_LIBRARIES})
endif( QCA2_FOUND )


if(KDE4_BUILD_TESTS)
    target_link_libraries(amaroklib ${QT_QTTEST_LIBRARY})
endif()

if( NOT BUILD_MYSQLE_COLLECTION )
    target_link_libraries(amaroklib ${MYSQL_LIBRARIES} ${ZLIB_LIBRARIES})
endif( NOT BUILD_MYSQLE_COLLECTION )

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

set( amarok_SRCS main.cpp )

kde4_add_app_icon(amarok_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/../images/hi*-app-amarok.png)

find_package(X11)

if(Q_WS_MAC)
    kde4_add_executable(Amarok ${amarok_SRCS})
    target_link_libraries(Amarok ${KDE4_KDECORE_LIBS} amarokcore amaroklib )
    SET_TARGET_PROPERTIES(Amarok PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
    set_target_properties(Amarok PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${MAC_FILES_DIR}/Info.plist.template)
    set_target_properties(Amarok PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.amarok")
    set_target_properties(Amarok PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "Amarok 2")
    #the following line was copied from KDE4Macros.cmake. kde4_add_app_icon fails to actually copy the .icns file
    SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_BINARY_DIR}/amarok.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    install(TARGETS Amarok ${INSTALL_TARGETS_DEFAULT_ARGS})
elseif(X11_FOUND)
    kde4_add_executable(amarok ${amarok_SRCS})
    target_link_libraries(amarok ${KDE4_KDEUI_LIBS} amarokcore amaroklib ${X11_LIBRARIES})
    install(TARGETS amarok ${INSTALL_TARGETS_DEFAULT_ARGS})
else(Q_WS_MAC)
    kde4_add_executable(amarok ${amarok_SRCS})
    target_link_libraries(amarok ${KDE4_KDEUI_LIBS} amarokcore amaroklib)
    install(TARGETS amarok ${INSTALL_TARGETS_DEFAULT_ARGS})
endif(Q_WS_MAC)

########### install files ###############

install(PROGRAMS amarok.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} )
install(PROGRAMS amarok_containers.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} )

install(FILES amarok_plugin.desktop DESTINATION ${SERVICETYPES_INSTALL_DIR})
install(FILES amarok_codecinstall.desktop DESTINATION ${SERVICETYPES_INSTALL_DIR})
install(FILES amarok_append.desktop DESTINATION ${SERVICES_INSTALL_DIR}/ServiceMenus)
install(FILES amarok-play-audiocd.desktop DESTINATION ${DATA_INSTALL_DIR}/solid/actions)

install(FILES amarok.knsrc DESTINATION ${CONFIG_INSTALL_DIR})
install(FILES context/amarokapplets.knsrc DESTINATION ${CONFIG_INSTALL_DIR})

# protocol handlers
install(FILES amarokurls/amarok.protocol  DESTINATION  ${SERVICES_INSTALL_DIR})
install(FILES amarokitpc.protocol DESTINATION ${SERVICES_INSTALL_DIR})
#install(FILES amarokpcast.protocol DESTINATION ${SERVICES_INSTALL_DIR})

install(FILES context/servicetypes/amarok_context_applet.desktop
              context/servicetypes/amarok_data_engine.desktop
              DESTINATION ${SERVICETYPES_INSTALL_DIR} )

install(FILES amarokconfig.kcfg DESTINATION ${KCFG_INSTALL_DIR} )
install(FILES dbus/mpris1/org.freedesktop.MediaPlayer.root.xml
              dbus/mpris1/org.freedesktop.MediaPlayer.player.xml
              dbus/mpris1/org.freedesktop.MediaPlayer.tracklist.xml
              dbus/org.kde.amarok.App.xml
              dbus/org.kde.amarok.Collection.xml
              dbus/mpris1/org.kde.amarok.Mpris1Extensions.Player.xml
              dbus/mpris2/org.kde.amarok.Mpris2Extensions.Player.xml
              DESTINATION ${DBUS_INTERFACES_INSTALL_DIR})

install(FILES services/InfoParserLoading.html
              browsers/hover_info_template.html
              DESTINATION ${DATA_INSTALL_DIR}/amarok/data)

kde4_install_icons( ${ICON_INSTALL_DIR} )

# Improves speed of string concatenation
add_definitions(-DQT_USE_FAST_CONCATENATION)
add_definitions(-DQT_USE_FAST_OPERATOR_PLUS)

if(APPLE)
    set(mac_SRCS
        app_mac.cpp
        mac/GrowlInterface.cpp
    )
    # Notification Center Appeared in 10.8, or Darwin 12
    if( CMAKE_SYSTEM_VERSION VERSION_GREATER "11.9.9")
        list(APPEND mac_SRCS mac/MacSystemNotify.mm)
        add_definitions(-DHAVE_NOTIFICATION_CENTER)
    endif()

    include_directories ( services/lastfm/ )
    set( MAC_FILES_DIR  ${CMAKE_SOURCE_DIR}/src/mac )
endif()

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/core-impl/collections/db/sql/
)

add_subdirectory( core )
add_subdirectory( core-impl/collections )
add_subdirectory( core-impl/storage/sql )
add_subdirectory( context )
add_subdirectory( services )
add_subdirectory( scripting/scripts )
add_subdirectory( transcoding )
add_subdirectory( importers )

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

ki18n_wrap_ui(libconfigdialog_SRCS
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
# STORAGE
#####################################################################
set(storage_SRCS
    core-impl/storage/StorageManager.cpp
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
    context/AmarokContextPackageStructure.cpp
    context/AppletLoader.cpp
    context/AppletModel.cpp
    context/ContextDock.cpp
    context/ContextView.cpp
)

#####################################################################
# LYRICS
#####################################################################
set( liblyrics_SRCS
    lyrics/LyricsManager.cpp
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
    browsers/playlistbrowser/DynamicBiasDialog.cpp
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

ki18n_wrap_ui(libplaylist_SRCS
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
    scripting/scriptengine/AmarokBookmarkScript.cpp
    scripting/scriptengine/AmarokCollectionScript.cpp
    scripting/scriptengine/AmarokCollectionViewScript.cpp
    scripting/scriptengine/AmarokEngineScript.cpp
    scripting/scriptengine/AmarokEqualizerScript.cpp
    scripting/scriptengine/AmarokInfoScript.cpp
    scripting/scriptengine/AmarokKNotifyScript.cpp
    scripting/scriptengine/AmarokLyricsScript.cpp
    scripting/scriptengine/AmarokNetworkScript.cpp
    scripting/scriptengine/AmarokOSDScript.cpp
    scripting/scriptengine/AmarokPlaylistManagerScript.cpp
    scripting/scriptengine/AmarokPlaylistScript.cpp
    scripting/scriptengine/AmarokScript.cpp
    scripting/scriptengine/AmarokScriptConfig.cpp
    scripting/scriptengine/AmarokScriptableServiceScript.cpp
    scripting/scriptengine/AmarokServicePluginManagerScript.cpp
    scripting/scriptengine/AmarokStatusbarScript.cpp
    scripting/scriptengine/AmarokStreamItemScript.cpp
    scripting/scriptengine/AmarokWindowScript.cpp
    scripting/scriptengine/AmarokScriptXml.cpp
    scripting/scriptengine/ScriptImporter.cpp
    scripting/scriptengine/ScriptingDefines.cpp
    scripting/scriptengine/exporters/CollectionTypeExporter.cpp
    scripting/scriptengine/exporters/MetaTypeExporter.cpp
    scripting/scriptengine/exporters/PlaylistExporter.cpp
    scripting/scriptengine/exporters/PlaylistProviderExporter.cpp
    scripting/scriptengine/exporters/QueryMakerExporter.cpp
    scripting/scriptengine/exporters/ScriptableBiasExporter.cpp
)

set(qtbindings_SRCS
    scripting/scriptengine/qtbindings/CoreByteArray.cpp
    scripting/scriptengine/qtbindings/CoreCoreApplication.cpp
    scripting/scriptengine/qtbindings/CoreDir.cpp
    scripting/scriptengine/qtbindings/CoreFile.cpp
    scripting/scriptengine/qtbindings/CoreFileInfo.cpp
    scripting/scriptengine/qtbindings/CoreIODevice.cpp
    scripting/scriptengine/qtbindings/CoreLocale.cpp
    scripting/scriptengine/qtbindings/CoreResource.cpp
    scripting/scriptengine/qtbindings/CoreTextCodec.cpp
    scripting/scriptengine/qtbindings/CoreTextStream.cpp
    scripting/scriptengine/qtbindings/CoreTranslator.cpp
    scripting/scriptengine/qtbindings/CoreUrl.cpp
    scripting/scriptengine/qtbindings/GuiCheckBox.cpp
    scripting/scriptengine/qtbindings/GuiDialogButtonBox.cpp
    scripting/scriptengine/qtbindings/GuiLabel.cpp
    scripting/scriptengine/qtbindings/SqlSqlQuery.cpp
)
if( Qt${QT_MAJOR_VERSION}UiTools_FOUND )
    # Append below all files from QtBindings::UiTools namespace
    list(APPEND qtbindings_SRCS scripting/scriptengine/qtbindings/UiToolsUiLoader.cpp)
endif()

set(scriptconsole_SRCS
    scripting/scriptconsole/CompletionModel.cpp
    scripting/scriptconsole/ScriptConsole.cpp
    scripting/scriptconsole/ScriptEditorDocument.cpp
    scripting/scriptconsole/ScriptConsoleItem.cpp
)

if (TARGET Python3::Interpreter)
    execute_process(COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/src/scripting/scriptengine/PHAACG2.py
                                                  ${CMAKE_SOURCE_DIR}/src/scripting/scriptengine
                                                  ${CMAKE_BINARY_DIR}/scriptconsole)
    install(FILES ${CMAKE_BINARY_DIR}/scriptconsole/AutoComplete.txt
            DESTINATION ${KDE_INSTALL_DATADIR}/amarok/scriptconsole)
endif()

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

ki18n_wrap_ui(apg_SRCS
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

    ki18n_wrap_ui(network_access_SRCS
        network/NetworkRequests.ui
        )
endif()

#####################################################################
# STATISTICS SYNCHRONIZATION
#####################################################################
set( statsyncing_SRCS
    statsyncing/Config.cpp
    statsyncing/Controller.cpp
    statsyncing/Options.cpp
    statsyncing/Process.cpp
    statsyncing/Provider.cpp
    statsyncing/ProviderFactory.cpp
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

ki18n_wrap_ui( statsyncing_SRCS
    statsyncing/ui/ChooseProvidersPage.ui
    statsyncing/ui/MatchedTracksPage.ui
)

#####################################################################
# STATISTICS IMPORTERS
#####################################################################
set( importers_SRCS
    importers/ImporterManager.cpp
    importers/ImporterProvider.cpp
    importers/ImporterSqlConnection.cpp
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
    ${liblyrics_SRCS}
    ${libplaylist_SRCS}
    ${aggregatecollection_SRCS}
    ${libpodcasts_SRCS}
    ${libmediadeviceframework_SRCS}
    ${libserviceframework_SRCS}
    ${libservicebrowser_SRCS}
    ${libdynamic_SRCS}
    ${libmetaimpl_SRCS}
    ${apg_SRCS}
    ${collection_SRCS}
    ${storage_SRCS}
    ${scanner_SRCS}
    ${mac_SRCS}
    ${network_access_SRCS}
    ${libplaylistbrowser_SRCS}
    ${libplaylistmanager_SRCS}
    ${dbus_SRCS}
    ${qtbindings_SRCS}
    ${scriptengine_SRCS}
    ${scriptconsole_SRCS}
    ${libstatusbar_SRCS}
    ${libamarokurl_SRCS}
    ${libsynchronization_SRCS}
    ${statsyncing_SRCS}
    ${importers_SRCS}
    aboutdialog/ExtendedAboutDialog.cpp
    aboutdialog/FramedLabel.cpp
    aboutdialog/OcsData.cpp
    aboutdialog/OcsPersonItem.cpp
    aboutdialog/OcsPersonListWidget.cpp
    ActionClasses.cpp
    AmarokMimeData.cpp
    AmarokProcess.cpp
    App.cpp
    EngineController.cpp
    KNotificationBackend.cpp
    MainWindow.cpp
    MediaDeviceCache.cpp
    MediaDeviceMonitor.cpp
    PluginManager.cpp
    QStringx.cpp
    scripting/scriptmanager/ScriptManager.cpp
    scripting/scriptmanager/ScriptItem.cpp
    SvgHandler.cpp
    SvgTinter.cpp
    TrayIcon.cpp
    core-impl/logger/DebugLogger.cpp
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
    databaseimporter/SqlBatchImporter.cpp
    databaseimporter/SqlBatchImporterConfig.cpp
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
    playback/EqualizerController.cpp
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
    widgets/BoxWidget.cpp
    widgets/BreadcrumbItemButton.cpp
    widgets/ClearSpinBox.cpp
    widgets/CoverLabel.cpp
    widgets/kdatecombo.cpp
    widgets/TokenDropTarget.cpp
    widgets/ElidingButton.cpp
    widgets/FilenameLayoutWidget.cpp
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

if( LIBLASTFM_FOUND )
    set(amaroklib_LIB_SRCS
        ${amaroklib_LIB_SRCS}
        LastfmReadLabelCapability.cpp
    )
    include_directories(SYSTEM ${LIBLASTFM_INCLUDE_DIR}/..  ${LIBLASTFM_INCLUDE_DIR})
    set( EXTRA_LIBS ${LIBLASTFM_LIBRARY} )
endif()

if( LIBOFA_FOUND AND AVCODEC_FOUND AND AVFORMAT_FOUND AND AVUTIL_FOUND )
    add_definitions(
        ${AVCODEC_DEFINITIONS}
        ${AVFORMAT_DEFINITIONS}
        ${AVUTIL_DEFINITIONS}
    )
    include_directories(SYSTEM
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
endif()

qt_add_dbus_adaptor(
   amaroklib_LIB_SRCS
   dbus/org.kde.amarok.Collection.xml
   dbus/CollectionDBusHandler.h
   CollectionDBusHandler
   CollectionAdaptor
   CollectionAdaptor )

set( amaroklib_DEPENDS "amarokpud" )
set( amaroklib_DEPENDS "amarokcore" )
set( amaroklib_DEPENDS "amarok-transcoding" ) # depends on generated ui_*.h file

kconfig_add_kcfg_files(amaroklib_LIB_SRCS amarokconfig.kcfgc)
add_custom_target(amarokconfig_h DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/amarokconfig.h)

ki18n_wrap_ui(amaroklib_LIB_SRCS
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

add_library(amaroklib SHARED ${amaroklib_LIB_SRCS})

if(BUILD_WITH_QT6)
    target_link_libraries(amaroklib
        Phonon::phonon4qt${QT_MAJOR_VERSION}
        KF${KF_MAJOR_VERSION}::Archive
        KF${KF_MAJOR_VERSION}::CoreAddons
        KF${KF_MAJOR_VERSION}::ColorScheme
        KF${KF_MAJOR_VERSION}::GlobalAccel
        KF${KF_MAJOR_VERSION}::GuiAddons
        KF${KF_MAJOR_VERSION}::I18n
        KF${KF_MAJOR_VERSION}::IconThemes
        KF${KF_MAJOR_VERSION}::KCMUtils
        KF${KF_MAJOR_VERSION}::KIOCore
        KF${KF_MAJOR_VERSION}::KIOFileWidgets
        KF${KF_MAJOR_VERSION}::KIOWidgets
        KF${KF_MAJOR_VERSION}::Notifications
        KF${KF_MAJOR_VERSION}::Package
        KF${KF_MAJOR_VERSION}::StatusNotifierItem
        KF${KF_MAJOR_VERSION}::TextEditor
        KF${KF_MAJOR_VERSION}::ThreadWeaver
        KF${KF_MAJOR_VERSION}::WidgetsAddons
        KF${KF_MAJOR_VERSION}::WindowSystem
        Qt::Core5Compat
        Qt::Gui
        Qt::Qml
        Qt::Quick
        Qt::QuickWidgets
        Qt::Sql
        Qt::Svg
        Qt::SvgWidgets
        ${CMAKE_DL_LIBS}
        Threads::Threads
        ${EXTRA_LIBS}
        amarokpud
        amarokcore
        amarok-transcoding
        amarokshared
    )
else()
    target_link_libraries(amaroklib
        Phonon::phonon4qt${QT_MAJOR_VERSION}
        KF${KF_MAJOR_VERSION}::Archive
        KF${KF_MAJOR_VERSION}::CoreAddons
        KF${KF_MAJOR_VERSION}::GlobalAccel
        KF${KF_MAJOR_VERSION}::GuiAddons
        KF${KF_MAJOR_VERSION}::I18n
        KF${KF_MAJOR_VERSION}::IconThemes
        KF${KF_MAJOR_VERSION}::KCMUtils
        KF${KF_MAJOR_VERSION}::KIOCore
        KF${KF_MAJOR_VERSION}::KIOFileWidgets
        KF${KF_MAJOR_VERSION}::KIOWidgets
        KF${KF_MAJOR_VERSION}::Notifications
        KF${KF_MAJOR_VERSION}::Package
        KF${KF_MAJOR_VERSION}::TextEditor
        KF${KF_MAJOR_VERSION}::ThreadWeaver
        KF${KF_MAJOR_VERSION}::WidgetsAddons
        KF${KF_MAJOR_VERSION}::WindowSystem
        Qt::Gui
        Qt::Qml
        Qt::Quick
        Qt::QuickWidgets
        Qt::Sql
        Qt::Svg
        ${CMAKE_DL_LIBS}
        Threads::Threads
        ${EXTRA_LIBS}
        amarokpud
        amarokcore
        amarok-transcoding
        amarokshared
    )
endif()

if( Qt5WebEngine_FOUND )
    target_link_libraries( amaroklib Qt::WebEngine )
    add_definitions( -DWITH_QT_WEBENGINE )
endif()

if( Qt6WebEngineWidgets_FOUND )
    target_link_libraries( amaroklib Qt::WebEngineWidgets )
    add_definitions( -DWITH_QT_WEBENGINE )
endif()

if( Qt${QT_MAJOR_VERSION}UiTools_FOUND )
    target_link_libraries( amaroklib Qt::UiTools )
    add_definitions( -DWITH_QT_UITOOLS )
endif()

include_directories(SYSTEM ${TAGLIB_INCLUDES})
add_definitions(${TAGLIB_CFLAGS})
target_link_libraries(amaroklib ${TAGLIB_LIBRARIES})
if( TAGLIB-EXTRAS_FOUND )
    include_directories(SYSTEM ${TAGLIB-EXTRAS_INCLUDES})
    add_definitions(${TAGLIB-EXTRAS_CFLAGS})
    target_link_libraries(amaroklib ${TAGLIB-EXTRAS_LIBRARIES})
endif()

if(WIN32)
    target_link_libraries(amaroklib Qt::WebKitWidgets)
endif()
if(APPLE)
    target_link_libraries(amaroklib "/System/Library/Frameworks/Foundation.framework")
    set_target_properties(amaroklib PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif()

set_target_properties(amaroklib PROPERTIES VERSION 1.0.0 SOVERSION 1 )
install(TARGETS amaroklib ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )


#####################################################################
# AMAROK
#####################################################################

set( amarok_SRCS main.cpp )

file(GLOB ICONS_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/../images/*-apps-amarok.png)
ecm_add_app_icon(amarok_SRCS ICONS ${ICONS_SRCS})

add_executable(amarok ${amarok_SRCS})
if(APPLE)
    set_target_properties(amarok PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
    set(MACOSX_BUNDLE_BUNDLE_NAME "Amarok 2")
    set(MACOSX_BUNDLE_BUNDLE_VERSION "2.8.0-git")
    set(MACOSX_BUNDLE_COPYRIGHT "Amarok Team")
    set_target_properties(amarok PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${MAC_FILES_DIR}/Info.plist.template)
    set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/amarok.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
endif()

target_link_libraries(amarok
    KF${KF_MAJOR_VERSION}::CoreAddons
    KF${KF_MAJOR_VERSION}::Crash
    KF${KF_MAJOR_VERSION}::DBusAddons
    KF${KF_MAJOR_VERSION}::I18n

    amarokcore
    amaroklib
)

install(TARGETS amarok ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

########### install files ###############

install(PROGRAMS org.kde.amarok.desktop DESTINATION ${KDE_INSTALL_APPDIR} )
install(PROGRAMS org.kde.amarok_containers.desktop DESTINATION ${KDE_INSTALL_APPDIR} )
install(FILES org.kde.amarok.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})

if(NOT BUILD_WITH_QT6)
  install(FILES amarok-plugin.desktop DESTINATION ${KDE_INSTALL_KSERVICETYPES5DIR})
  install(FILES amarok-contextapplet.desktop DESTINATION ${KDE_INSTALL_KSERVICETYPES5DIR})
  install(FILES amarok_codecinstall.desktop DESTINATION ${KDE_INSTALL_KSERVICETYPES5DIR})
endif()
install(FILES amarok_append.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR}/ServiceMenus)
install(FILES amarok-play-audiocd.desktop DESTINATION ${KDE_INSTALL_DATADIR}/solid/actions)

#install(FILES amarok.knsrc DESTINATION ${KDE_INSTALL_KNSRCDIR})

# protocol handlers

if(NOT BUILD_WITH_QT6)
  install(FILES amarokurls/amarok.protocol  DESTINATION  ${KDE_INSTALL_KSERVICES5DIR})
  install(FILES amarokitpc.protocol DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
endif()
#install(FILES amarokpcast.protocol DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

install(FILES amarokconfig.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR} )
install(FILES dbus/org.kde.amarok.App.xml
              dbus/org.kde.amarok.Collection.xml
              dbus/mpris2/org.kde.amarok.Mpris2Extensions.Player.xml
              DESTINATION ${KDE_INSTALL_DBUSINTERFACEDIR})
configure_file(dbus/org.kde.amarok.service.in ${CMAKE_CURRENT_BINARY_DIR}/dbus/org.kde.amarok.service @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dbus/org.kde.amarok.service
              DESTINATION ${KDE_INSTALL_DBUSSERVICEDIR})

install(FILES services/InfoParserLoading.html
              browsers/hover_info_template.html
              DESTINATION ${KDE_INSTALL_DATADIR}/amarok/data)

ecm_install_icons(ICONS
    DESTINATION ${KDE_INSTALL_ICONDIR}
    THEME hicolor
)

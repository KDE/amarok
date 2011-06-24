# Improves speed of string concatenation
add_definitions(-DQT_USE_FAST_CONCATENATION)
add_definitions(-DQT_USE_FAST_OPERATOR_PLUS)
add_definitions(-DQT_STRICT_ITERATORS)

if(APPLE)
    set(mac_SRCS
        app_mac.cpp
        mac/GrowlInterface.cpp
    )

    include_directories ( services/lastfm/ )
    set( MAC_FILES_DIR  ${CMAKE_SOURCE_DIR}/src/mac )
endif(APPLE)

add_subdirectory( core )
add_subdirectory( core-impl/collections )
#dont use our own libplasma anymore, but still pick up our applets/engines/containments
add_subdirectory( context )
add_subdirectory( images )
add_subdirectory( services )
if( QTSCRIPTQTBINDINGS_FOUND )
    add_subdirectory( scripts )
endif( QTSCRIPTQTBINDINGS_FOUND )
add_subdirectory( themes )
add_subdirectory( aboutdialog/libattica-ocsclient )
add_subdirectory( transcoding )
add_subdirectory( kconf_update )

# if( STRIGI_FOUND )
    # add_subdirectory( strigi )
# endif( STRIGI_FOUND )

include_directories(
    ${CMAKE_SOURCE_DIR}
    aboutdialog
    aboutdialog/libattica-ocsclient
    amarokurls
    browsers/
    browsers/playlistbrowser
    browsers/servicebrowser
    browsers/servicebrowser/scriptableservice
    core-impl/collections
    core-impl/collections/support
    core-impl/collections/mediadevicecollection
    core-impl/collections/mediadevicecollection/handler  # for Handler
    core-impl/collections/mediadevicecollection/support  # for MediaDeviceInfo
    core-impl/collections/mediadevicecollection/playlist # for MediaDevicePlaylist etc.
    core-impl/collections/mediadevicecollection/podcast
    configdialog
    configdialog/dialogs
    context
    context/widgets
    context/widgets/appletexplorer/
    dbus
    dialogs
    dynamic
    likeback
    musicbrainz
    network
    playlistmanager
    statusbar
    synchronization
    timecode
    transcoding
    widgets
    ${KDE4_INCLUDE_DIR}
    ${QT_INCLUDES}
    ${QT_QTSCRIPT_INCLUDE_DIR}
)

if ( KDEMULTIMEDIA_FOUND )
    include_directories( ${KDEMULTIMEDIA_INCLUDE_DIR} )
endif ( KDEMULTIMEDIA_FOUND )

#####################################################################
# LIKEBACK
#####################################################################
set(likeback_SRCS
    likeback/LikeBack.cpp
    likeback/LikeBackBar.cpp
    likeback/LikeBackDialog.cpp
)

#####################################################################
# PROXYCOLLECTION
#####################################################################
set(libproxycollection_SRCS
    core-impl/collections/proxycollection/ProxyCollection.cpp
    core-impl/collections/proxycollection/ProxyCollectionMeta.cpp
    core-impl/collections/proxycollection/ProxyCollectionQueryMaker.cpp
)

#####################################################################
# MEDIADEVICEFRAMEWORK
#####################################################################
set(libmediadeviceframework_SRCS
  core-impl/collections/mediadevicecollection/MediaDeviceCollection.cpp
  core-impl/collections/mediadevicecollection/MediaDeviceCollectionLocation.cpp
  core-impl/collections/mediadevicecollection/MediaDeviceMeta.cpp
  core-impl/collections/mediadevicecollection/handler/MediaDeviceHandler.cpp
  core-impl/collections/mediadevicecollection/handler/MediaDeviceHandlerCapability.cpp
  core-impl/collections/mediadevicecollection/handler/capabilities/ArtworkCapability.cpp
  core-impl/collections/mediadevicecollection/handler/capabilities/CustomReadCapability.cpp
  core-impl/collections/mediadevicecollection/handler/capabilities/PlaylistCapability.cpp
  core-impl/collections/mediadevicecollection/handler/capabilities/PodcastCapability.cpp
  core-impl/collections/mediadevicecollection/handler/capabilities/ReadCapability.cpp
  core-impl/collections/mediadevicecollection/handler/capabilities/ReadCapabilityBase.cpp
  core-impl/collections/mediadevicecollection/handler/capabilities/WriteCapability.cpp
  core-impl/collections/mediadevicecollection/handler/capabilities/WriteCapabilityBase.cpp
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
    configdialog/dialogs/PluginsConfig.cpp
    configdialog/dialogs/ScriptsConfig.cpp
    configdialog/dialogs/ScriptSelector.cpp
    configdialog/dialogs/DatabaseConfig.cpp
)

kde4_add_ui_files(libconfigdialog_SRCS
    configdialog/dialogs/CollectionConfig.ui
    configdialog/dialogs/GeneralConfig.ui
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
    browsers/BrowserCategoryListDelegate.cpp
    browsers/BrowserCategoryListModel.cpp
    browsers/BrowserCategoryListSortFilterProxyModel.cpp
    browsers/BrowserDock.cpp
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
    statusbar/NetworkProgressBar.cpp
    statusbar/CompoundProgressBar.cpp
    statusbar/PopupWidget.cpp
    statusbar/LongMessageWidget.cpp
)


#####################################################################
# META
#####################################################################
set(libmetaimpl_SRCS
    core-impl/playlists/types/file/m3u/M3UPlaylist.cpp
    ../shared/MetaTagLib.cpp
    ../shared/TagsFromFileNameGuesser.cpp
    core-impl/playlists/types/file/pls/PLSPlaylist.cpp
    core-impl/playlists/providers/user/UserPlaylistProvider.cpp
    core-impl/playlists/types/file/PlaylistFileSupport.cpp
    core-impl/playlists/types/file/xspf/XSPFPlaylist.cpp
    core-impl/capabilities/timecode/TimecodeBoundedPlaybackCapability.cpp
    core-impl/capabilities/timecode/TimecodeEditCapability.cpp
    core-impl/capabilities/timecode/TimecodeLoadCapability.cpp
    core-impl/capabilities/timecode/TimecodeWriteCapability.cpp
    core-impl/capabilities/multisource/MultiSourceCapabilityImpl.cpp
    core-impl/meta/file/File.cpp
    core-impl/meta/multi/MultiTrack.cpp
    core-impl/meta/cue/CueFileSupport.cpp
    core-impl/meta/proxy/MetaProxy.cpp
    core-impl/meta/stream/Stream.cpp
    core-impl/statistics/providers/url/PermanentUrlStatisticsProvider.cpp
    core-impl/statistics/providers/tag/TagStatisticsProvider.cpp
)

if( TAGLIB_FOUND )
    set(libmetaimpl_SRCS
        ${libmetaimpl_SRCS}
        ../shared/MetaReplayGain.cpp
        ../shared/FileTypeResolver.cpp
        ../shared/tag_helpers/APETagHelper.cpp
        ../shared/tag_helpers/ASFTagHelper.cpp
        ../shared/tag_helpers/ID3v2TagHelper.cpp
        ../shared/tag_helpers/MP4TagHelper.cpp
        ../shared/tag_helpers/StringHelper.cpp
        ../shared/tag_helpers/TagHelper.cpp
        ../shared/tag_helpers/VorbisCommentTagHelper.cpp
    )
endif( TAGLIB_FOUND )

#####################################################################
# COLLECTION
#####################################################################
set(collection_SRCS
    core-impl/collections/support/CollectionManager.cpp
    core-impl/collections/support/CollectionLocationDelegateImpl.cpp
    core-impl/collections/support/MemoryCustomValue.cpp
    core-impl/collections/support/MemoryFilter.cpp
    core-impl/collections/support/MemoryMatcher.cpp
    core-impl/collections/support/MemoryQueryMaker.cpp
    core-impl/collections/support/MemoryQueryMakerInternal.cpp
    core-impl/collections/support/MemoryQueryMakerHelper.cpp
    core-impl/collections/support/PlaylistCollection.cpp
    core-impl/collections/support/PlaylistCollectionLocation.cpp
    core-impl/collections/support/TrashCollectionLocation.cpp
    core-impl/collections/support/XmlQueryReader.cpp
    core-impl/collections/support/XmlQueryWriter.cpp
    core-impl/collections/support/FileCollectionLocation.cpp
    core-impl/collections/support/Expression.cpp
    core-impl/collections/support/TextualQueryFilter.cpp
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
    browsers/playlistbrowser/PlaylistTreeItemDelegate.cpp
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
    playlist/PlaylistColumnNames.cpp
    playlist/PlaylistController.cpp
    playlist/PlaylistInfoWidget.cpp
    playlist/PlaylistItem.cpp
    playlist/PlaylistModel.cpp
    playlist/PlaylistModelStack.cpp
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
# AUDIO CD SUPPORT
#####################################################################

if(KDEMULTIMEDIA_FOUND)
    set(audiocdsupport_SRCS
        core-impl/meta/cdda/AudioCdTrackProvider.cpp
        core-impl/meta/cdda/AudioCdTrackProvider_p.cpp
       )
endif(KDEMULTIMEDIA_FOUND)

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
    dbus/mpris2/Mpris2DBusHandler.cpp
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
    playlistgenerator/ConstraintTester.cpp
    playlistgenerator/ConstraintTestWrapper.cpp
    playlistgenerator/Preset.cpp
    playlistgenerator/PresetEditDialog.cpp
    playlistgenerator/PresetModel.cpp
    playlistgenerator/TreeController.cpp
    playlistgenerator/TreeModel.cpp
    playlistgenerator/constraints/Checkpoint.cpp
    playlistgenerator/constraints/CheckpointSupport.cpp
    playlistgenerator/constraints/Matching.cpp
    playlistgenerator/constraints/PlaylistDuration.cpp
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
    ${libtooltip_SRCS}
    ${libproxycollection_SRCS}
    ${libpodcasts_SRCS}
    ${libmediadeviceframework_SRCS}
    ${libserviceframework_SRCS}
    ${libservicebrowser_SRCS}
    ${libdynamic_SRCS}
    ${likeback_SRCS}
    ${libmetaimpl_SRCS}
    ${core_interfaces_SRCS}
    ${apg_SRCS}
    ${collection_SRCS}
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
    DirectoryLoader.cpp
    EngineController.cpp
    KNotificationBackend.cpp
    MainWindow.cpp
    MediaDeviceCache.cpp
    MediaDeviceMonitor.cpp
    MountPointManager.cpp
    PluginManager.cpp
    ScriptManager.cpp
    ScriptUpdater.cpp
    SvgHandler.cpp
    SvgTinter.cpp
    TrayIcon.cpp
    core-impl/meta/timecode/TimecodeObserver.cpp
    core-impl/meta/timecode/TimecodeMeta.cpp
    core-impl/meta/timecode/TimecodeTrackProvider.cpp
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
    dialogs/EditFilterDialog.cpp
    dialogs/EqualizerDialog.cpp
    dialogs/FilenameLayoutDialog.cpp
    dialogs/MusicBrainzTagger.cpp
    dialogs/OrganizeCollectionDialog.cpp
    dialogs/TrackOrganizer.cpp
    dialogs/TagDialog.cpp
    dialogs/TagGuesser.cpp
    dialogs/LabelListModel.cpp
    equalizer/EqualizerPresets.cpp
    browsers/filebrowser/MimeTypeFilterProxyModel.cpp
    browsers/filebrowser/FileBrowser.cpp
    browsers/filebrowser/FileView.cpp
    firstruntutorial/FirstRunTutorial.cpp
    firstruntutorial/FirstRunTutorialPage.cpp
    musicbrainz/MusicBrainzFinder.cpp
    musicbrainz/MusicBrainzTags.cpp
    musicbrainz/MusicBrainzXmlParser.cpp
    OpmlOutline.cpp
    OpmlParser.cpp
    OpmlWriter.cpp
    PaletteHandler.cpp
    PopupDropperFactory.cpp
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
    widgets/CoverLabel.cpp
    widgets/HintLineEdit.cpp
    widgets/kdatecombo.cpp
    widgets/kratingpainter.cpp
    widgets/kratingwidget.cpp
    widgets/TokenDropTarget.cpp
    widgets/EditDeleteComboBoxView.cpp
    widgets/EditDeleteDelegate.cpp
    widgets/ElidingButton.cpp
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
    widgets/ProgressWidget.cpp
    widgets/SearchWidget.cpp
    widgets/SliderWidget.cpp
    widgets/StarManager.cpp
    widgets/TokenPool.cpp
    widgets/Token.cpp
    widgets/TokenWithLayout.cpp
    widgets/VolumeDial.cpp
    widgets/Splitter.cpp
    widgets/TrackActionButton.cpp
    widgets/BookmarkTriangle.cpp
    widgets/BookmarkPopup.cpp
    widgets/TrackSelectWidget.cpp
    widgets/MetaQueryWidget.cpp
    GlobalCollectionActions.cpp
    GlobalCurrentTrackActions.cpp
    moodbar/MoodbarManager.cpp
    ../shared/FileType.cpp
)

if( NOT BUILD_MYSQLE_COLLECTION )
    set(amaroklib_LIB_SRCS
        ${amaroklib_LIB_SRCS}
        MySqlServerTester.cpp
    )
    add_definitions(${MYSQL_CFLAGS})
    include_directories(${MYSQL_INCLUDE_DIR})
endif( NOT BUILD_MYSQLE_COLLECTION )

if(KDE4_BUILD_TESTS)
    set(amaroklib_LIB_SRCS
        ${amaroklib_LIB_SRCS}
        ../tests/TestBase.cpp
        ../tests/TestDirectoryLoader.cpp
    )
    include_directories(
        ../tests/
        ../tests/context/
        ../tests/context/engines/
        ../tests/context/engines/upcomingevents/
        ../tests/context/engines/similarartists/
        ../tests/meta/
        ../tests/meta/cue/
        ../tests/meta/file/
        ../tests/meta/multi/
        ../tests/playlistmanager/
        ../tests/playlistmanager/file/
        ../tests/playlistmanager/sql/
        ../tests/timecode/
    )
endif()

if( LIBLASTFM_FOUND )
    set(amaroklib_LIB_SRCS
        ${amaroklib_LIB_SRCS}
        LastfmReadLabelCapability.cpp
    )
    include_directories( ${LIBLASTFM_INCLUDE_DIR}/..  ${LIBLASTFM_INCLUDE_DIR})
    set( EXTRA_LIBS ${LIBLASTFM_LIBRARY} )
endif( LIBLASTFM_FOUND )

if( LIBOFA_FOUND )
    include_directories(
        ${LIBAVCODEC_INCLUDE_DIR}
        ${LIBAVFORMAT_INCLUDE_DIR}
    )
    set( EXTRA_LIBS
        ${EXTRA_LIBS}
        ${LIBOFA_LIBRARY}
        ${LIBAVFORMAT_LIBRARY}
        ${LIBAVCODEC_LIBRARY}
    )
    set( amaroklib_LIB_SRCS
        ${amaroklib_LIB_SRCS}
        musicbrainz/MusicDNSAudioDecoder.cpp
        musicbrainz/MusicDNSFinder.cpp
        musicbrainz/MusicDNSXmlParser.cpp
    )
endif( LIBOFA_FOUND )

qt4_add_dbus_adaptor(
   amaroklib_LIB_SRCS
   dbus/mpris1/org.freedesktop.MediaPlayer.root.xml
   dbus/mpris1/RootHandler.h
   Mpris1::RootHandler
   Mpris1RootAdaptor
   Mpris1RootAdaptor )
qt4_add_dbus_adaptor(
   amaroklib_LIB_SRCS
   dbus/mpris1/org.freedesktop.MediaPlayer.player.xml
   dbus/mpris1/PlayerHandler.h
   Mpris1::PlayerHandler
   Mpris1PlayerAdaptor
   Mpris1PlayerAdaptor )
qt4_add_dbus_adaptor(
   amaroklib_LIB_SRCS
   dbus/mpris1/org.freedesktop.MediaPlayer.tracklist.xml
   dbus/mpris1/TrackListHandler.h
   Mpris1::TrackListHandler
   Mpris1TrackListAdaptor
   Mpris1TrackListAdaptor )
qt4_add_dbus_adaptor(
   amaroklib_LIB_SRCS
   dbus/org.kde.amarok.App.xml
   dbus/mpris1/RootHandler.h
   Mpris1::RootHandler
   Mpris1AmarokAppAdaptor
   Mpris1AmarokAppAdaptor )
qt4_add_dbus_adaptor(
   amaroklib_LIB_SRCS
   dbus/mpris1/org.kde.amarok.Mpris1Extensions.Player.xml
   dbus/mpris1/PlayerHandler.h
   Mpris1::PlayerHandler
   Mpris1AmarokPlayerAdaptor
   Mpris1AmarokPlayerAdaptor )

qt4_add_dbus_adaptor(
   amaroklib_LIB_SRCS
   dbus/org.kde.amarok.Collection.xml
   dbus/CollectionDBusHandler.h
   CollectionDBusHandler
   CollectionAdaptor
   CollectionAdaptor )

qt4_add_dbus_adaptor(
   amaroklib_LIB_SRCS
   dbus/mpris2/org.mpris.MediaPlayer2.root.xml
   dbus/mpris2/Mpris2DBusHandler.h
   Amarok::Mpris2DBusHandler
   Mpris2RootAdaptor
   Mpris2RootAdaptor )
qt4_add_dbus_adaptor(
   amaroklib_LIB_SRCS
   dbus/mpris2/org.mpris.MediaPlayer2.Player.xml
   dbus/mpris2/Mpris2DBusHandler.h
   Amarok::Mpris2DBusHandler
   Mpris2PlayerAdaptor
   Mpris2PlayerAdaptor )
qt4_add_dbus_adaptor(
   amaroklib_LIB_SRCS
   dbus/org.kde.amarok.App.xml
   dbus/mpris2/Mpris2DBusHandler.h
   Amarok::Mpris2DBusHandler
   Mpris2AmarokAppAdaptor
   Mpris2AmarokAppAdaptor )
qt4_add_dbus_adaptor(
   amaroklib_LIB_SRCS
   dbus/mpris2/org.kde.amarok.Mpris2Extensions.Player.xml
   dbus/mpris2/Mpris2DBusHandler.h
   Amarok::Mpris2DBusHandler
   Mpris2AmarokPlayerAdaptor
   Mpris2AmarokPlayerAdaptor )

set( amaroklib_DEPENDS "amarokpud" )
set( amaroklib_DEPENDS "amarokcore" )

kde4_add_kcfg_files(amaroklib_LIB_SRCS amarokconfig.kcfgc)
add_custom_target(amarokconfig_h DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/amarokconfig.h)


kde4_add_ui_files(amaroklib_LIB_SRCS
    aboutdialog/OcsPersonItem.ui
    dialogs/EditFilterDialog.ui
    dialogs/EqualizerDialog.ui
    dialogs/MusicBrainzTagger.ui
    dialogs/TagDialogBase.ui
    dialogs/FilenameLayoutDialog.ui
    dialogs/OrganizeCollectionDialogBase.ui
    likeback/LikeBackBar.ui
    likeback/LikeBackDialog.ui
    playlist/layouts/PlaylistLayoutEditDialog.ui
    core-impl/podcasts/sql/PodcastSettingsBase.ui
    core-impl/podcasts/sql/SqlPodcastProviderSettingsWidget.ui
    core-impl/podcasts/sql/PodcastFilenameLayoutConfigWidget.ui
    browsers/playlistbrowser/PodcastCategoryBase.ui
)

kde4_add_library(amaroklib SHARED ${amaroklib_LIB_SRCS})

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
)

if( TAGLIB_FOUND )
    include_directories(${TAGLIB_INCLUDES})
    add_definitions(${TAGLIB_CFLAGS})
    target_link_libraries(amaroklib ${TAGLIB_LIBRARIES})
endif( TAGLIB_FOUND )
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

set(amarok_SRCS equalizer/EqualizerPresets.cpp main.cpp )
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}")

kde4_add_app_icon(amarok_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/images/hi*-app-amarok.png)

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

install(FILES  amarokconfig.kcfg DESTINATION ${KCFG_INSTALL_DIR} )
install(FILES dbus/mpris1/org.freedesktop.MediaPlayer.root.xml
              dbus/mpris1/org.freedesktop.MediaPlayer.player.xml
              dbus/mpris1/org.freedesktop.MediaPlayer.tracklist.xml
              dbus/org.kde.amarok.App.xml
              dbus/org.kde.amarok.Collection.xml
              dbus/mpris1/org.kde.amarok.Mpris1Extensions.Player.xml
              dbus/mpris2/org.kde.amarok.Mpris2Extensions.Player.xml
              DESTINATION ${DBUS_INTERFACES_INSTALL_DIR})


install(FILES data/DefaultPlaylistLayouts.xml
              data/first_run_jingle.ogg
              services/InfoParserLoading.html
              browsers/hover_info_template.html
              DESTINATION ${DATA_INSTALL_DIR}/amarok/data)

install( FILES data/amarok_homerc DESTINATION ${CONFIG_INSTALL_DIR} )
install( FILES data/amarok.notifyrc DESTINATION ${DATA_INSTALL_DIR}/amarok )

kde4_install_icons( ${ICON_INSTALL_DIR} )

set(libcore_interfaces_SRCS
    interfaces/MetaCapability.cpp
)

set(libcore_meta_SRCS
    meta/Base.cpp
    meta/Meta.cpp
    meta/Observer.cpp
    meta/Statistics.cpp
    meta/TrackEditor.cpp
    meta/support/MetaConstants.cpp
    meta/support/MetaUtility.cpp
    meta/support/MetaKeys.cpp
    meta/support/PrivateMetaRegistry.cpp
)

set(libcore_playlists_SRCS
    playlists/Playlist.cpp
    playlists/PlaylistFormat.cpp
    playlists/PlaylistProvider.cpp
)

set(libcore_capabilities_SRCS
    capabilities/Capability.cpp
    capabilities/ActionsCapability.cpp
    capabilities/BookmarkThisCapability.cpp
    capabilities/BoundedPlaybackCapability.cpp
    capabilities/CollectionScanCapability.cpp
    capabilities/CollectionImportCapability.cpp
    capabilities/FindInSourceCapability.cpp
    capabilities/MultiPlayableCapability.cpp
    capabilities/MultiSourceCapability.cpp
    capabilities/OrganiseCapability.cpp
    capabilities/ReadLabelCapability.cpp
    capabilities/SourceInfoCapability.cpp
    capabilities/StreamInfoCapability.cpp
    capabilities/TranscodeCapability.cpp
    capabilities/WriteLabelCapability.cpp
)

set(libcore_collection_SRCS
    collections/Collection.cpp
    collections/CollectionLocation.cpp
    collections/MetaQueryMaker.cpp
    collections/QueryMaker.cpp
    collections/support/TrackForUrlWorker.cpp
)

set(libcore_storage_SRCS
    storage/StorageFactory.cpp
)

set(libcore_podcasts_SRCS
    podcasts/PodcastReader.cpp
    podcasts/PodcastMeta.cpp
    podcasts/PodcastImageFetcher.cpp
    podcasts/PodcastProvider.cpp
)

set(libcore_support_SRCS
    support/Amarok.cpp
    support/Components.cpp
    support/SemaphoreReleaser.cpp
    support/PluginFactory.cpp
    support/Debug.cpp
)

set(libcore_transcoding_SRCS
    transcoding/formats/TranscodingNullFormat.cpp
    transcoding/formats/TranscodingAacFormat.cpp
    transcoding/formats/TranscodingAlacFormat.cpp
    transcoding/formats/TranscodingFlacFormat.cpp
    transcoding/formats/TranscodingMp3Format.cpp
    transcoding/formats/TranscodingOpusFormat.cpp
    transcoding/formats/TranscodingVorbisFormat.cpp
    transcoding/formats/TranscodingWmaFormat.cpp
    transcoding/TranscodingConfiguration.cpp
    transcoding/TranscodingController.cpp
    transcoding/TranscodingProperty.cpp
)

set(libcore_logger_SRCS
    logger/Logger.cpp
)

#####################################################################
# LIBCORE
#####################################################################
set(libcore_LIB_SRCS
    ${libcore_podcasts_SRCS}
    ${libcore_interfaces_SRCS}
    ${libcore_collection_SRCS}
    ${libcore_storage_SRCS}
    ${libcore_playlists_SRCS}
    ${libcore_meta_SRCS}
    ${libcore_capabilities_SRCS}
    ${libcore_support_SRCS}
    ${libcore_transcoding_SRCS}
    ${libcore_logger_SRCS}
)

add_library(amarokcore SHARED ${libcore_LIB_SRCS})
generate_export_header(amarokcore BASE_NAME amarokcore)
target_include_directories( amarokcore PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/support
    ${CMAKE_CURRENT_SOURCE_DIR}/meta
)

target_link_libraries(amarokcore
    amarokshared
    ${CMAKE_DL_LIBS}
    Threads::Threads
    Qt::Core
    Qt::Network
    KF5::KIOCore
    KF5::ConfigCore
    KF5::ThreadWeaver
    KF5::I18n
    KF5::XmlGui
)

if(APPLE)
    set_target_properties(amarokcore PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif()

set_target_properties(amarokcore PROPERTIES VERSION 1.0.0 SOVERSION 1 )
install(TARGETS amarokcore ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )

include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/shared
    ${KDE4_INCLUDE_DIR}
    ${QT_INCLUDES}
)

set(libcore_plugin_SRCS
    plugins/Plugin.cpp
    plugins/PluginManager.cpp
    plugins/PluginConfig.cpp
)

set(libcore_interfaces_SRCS
    interfaces/Logger.cpp
)

set(libcore_meta_SRCS
    meta/Meta.cpp
    meta/support/MetaUtility.cpp
    meta/support/PrivateMetaRegistry.cpp
)

set(libcore_playlists_SRCS
    playlists/Playlist.cpp
    playlists/PlaylistFormat.cpp
    playlists/PlaylistProvider.cpp
)

set(libcore_statistics_SRCS
    statistics/StatisticsProvider.cpp
)

set(libcore_capabilities_SRCS
    capabilities/Capability.cpp
    capabilities/BookmarkThisCapability.cpp
    capabilities/BoundedPlaybackCapability.cpp
    capabilities/CollectionCapability.cpp
    capabilities/CurrentTrackActionsCapability.cpp
    capabilities/CustomActionsCapability.cpp
    capabilities/DecoratorCapability.cpp
    capabilities/EditCapability.cpp
    capabilities/EditablePlaylistCapability.cpp
    capabilities/FindInSourceCapability.cpp
    capabilities/LastFmCapability.cpp
    capabilities/MultiPlayableCapability.cpp
    capabilities/MultiSourceCapability.cpp
    capabilities/OrganiseCapability.cpp
    capabilities/ReadLabelCapability.cpp
    capabilities/SourceInfoCapability.cpp
    capabilities/StatisticsCapability.cpp
    capabilities/StreamInfoCapability.cpp
    capabilities/UpdateCapability.cpp
    capabilities/WriteLabelCapability.cpp
)

set(libcore_collection_SRCS
    collections/Collection.cpp
    collections/CollectionLocation.cpp
    collections/MetaQueryMaker.cpp
    collections/QueryMaker.cpp
    collections/support/TrackForUrlWorker.cpp
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
    support/SmartPointerList.cpp
    ../../shared/Debug.cpp
)

set(libcore_engine_SRCS
    engine/EngineObserver.cpp
)

#####################################################################
# LIBCORE
#####################################################################
set(libcore_LIB_SRCS
    ${libcore_plugin_SRCS}
    ${libcore_podcasts_SRCS}
    ${libcore_interfaces_SRCS}
    ${libcore_collection_SRCS}
    ${libcore_statistics_SRCS}
    ${libcore_playlists_SRCS}
    ${libcore_meta_SRCS}
    ${libcore_capabilities_SRCS}
    ${libcore_engine_SRCS}
    ${libcore_support_SRCS}
)

kde4_add_library(amarokcore SHARED ${libcore_LIB_SRCS})

target_link_libraries(amarokcore
    ${CMAKE_THREAD_LIBS_INIT}
    ${KDE4_KIO_LIBS}
    ${KDE4_KUTILS_LIBS}
    ${KDE4_SOLID_LIBRARY}
    ${KDE4_THREADWEAVER_LIBRARIES}
    ${QT_CORE_LIBRARY}
)

if(APPLE)
    SET_TARGET_PROPERTIES(amarokcore PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)

if(LIBDL_FOUND)
    target_link_libraries(amarokcore dl)
endif(LIBDL_FOUND)

if(CMAKE_COMPILER_IS_GNUCXX AND CMAKE_SYSTEM_NAME MATCHES Linux)
    set ( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=iso9899:1999" )
endif(CMAKE_COMPILER_IS_GNUCXX AND CMAKE_SYSTEM_NAME MATCHES Linux)

set_target_properties(amarokcore PROPERTIES VERSION 1.0.0 SOVERSION 1 )
install(TARGETS amarokcore ${INSTALL_TARGETS_DEFAULT_ARGS} )



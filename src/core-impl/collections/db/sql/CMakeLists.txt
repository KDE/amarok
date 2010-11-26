
include_directories(
    .
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/utilities
    ${CMAKE_BINARY_DIR}/src
    ${KDE4_INCLUDE_DIR}
    ${STRIGI_INCLUDE_DIR}
    ${QT_INCLUDES}
    ${TAGLIB_INCLUDES}
    ${TAGLIB-EXTRAS_INCLUDES}
)

if (NOT WIN32)
 set (extra_src
    ../sql/XesamDbus.cpp
    ../sql/XesamCollectionBuilder.cpp)
endif (NOT WIN32)

set(amarok_collection-sqlcollection_SRCS
    CapabilityDelegateImpl.cpp
    DatabaseUpdater.cpp
    MountPointManager.cpp
    ../ScanManager.cpp
    ../DatabaseCollection.cpp
    ../ScanResultProcessor.cpp
    SqlCollection.cpp
    SqlCollectionFactory.cpp
    SqlCollectionLocation.cpp
    SqlQueryMaker.cpp
    SqlQueryMakerInternal.cpp
    SqlReadLabelCapability.cpp
    SqlRegistry.cpp
    SqlRegistry_p.cpp
    SqlMeta.cpp
    SqlWriteLabelCapability.cpp
    SqlScanResultProcessor.cpp
    ${CMAKE_SOURCE_DIR}/utilities/collectionscanner/Album.cpp
    ${CMAKE_SOURCE_DIR}/utilities/collectionscanner/BatchFile.cpp
    ${CMAKE_SOURCE_DIR}/utilities/collectionscanner/Directory.cpp
    ${CMAKE_SOURCE_DIR}/utilities/collectionscanner/Playlist.cpp
    ${CMAKE_SOURCE_DIR}/utilities/collectionscanner/Track.cpp
    ${CMAKE_SOURCE_DIR}/shared/FileType.cpp
    ${CMAKE_SOURCE_DIR}/src/core-impl/collections/support/ArtistHelper.cpp
    ${extra_src}
 )

kde4_add_library(amarok-sqlcollection SHARED ${amarok_collection-sqlcollection_SRCS})

target_link_libraries(amarok-sqlcollection
    ${KDE4_KDECORE_LIBS}
    ${KDE4_KDEUI_LIBS}
    ${KDE4_KIO_LIBS}
    ${KDE4_SOLID_LIBRARY}
    ${KDE4_THREADWEAVER_LIBRARIES}
    ${QT_QTCORE_LIBRARY}
    ${QT_QTGUI_LIBRARY}
    ${TAGLIB-EXTRAS_LIBRARIES}
    ${TAGLIB_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    amarokcore
    amaroklib
)

if(APPLE)
    SET_TARGET_PROPERTIES(amarok-sqlcollection PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)

set_target_properties(amarok-sqlcollection PROPERTIES VERSION 1.0.0 SOVERSION 1 )
install(TARGETS amarok-sqlcollection ${INSTALL_TARGETS_DEFAULT_ARGS} )

add_subdirectory( device )

if( BUILD_MYSQLE_COLLECTION )
    add_subdirectory( mysqlecollection )
endif( BUILD_MYSQLE_COLLECTION )
if( MYSQL_FOUND )
    add_subdirectory( mysqlservercollection )
endif( MYSQL_FOUND )


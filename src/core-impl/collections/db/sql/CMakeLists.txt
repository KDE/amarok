set( amarok_collection-sqlcollection_SRCS
    SqlCapabilities.cpp
    DatabaseUpdater.cpp
    ../DatabaseCollection.cpp
    ../MountPointManager.cpp
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
    device/massstorage/MassStorageDeviceHandler.cpp
    device/nfs/NfsDeviceHandler.cpp
    device/smb/SmbDeviceHandler.cpp
)

add_library(amarok-sqlcollection SHARED ${amarok_collection-sqlcollection_SRCS})

target_link_libraries(amarok-sqlcollection
    KF5::KIOCore
    ${KDE4_SOLID_LIBRARY}
    KF5::ThreadWeaver
    Qt5::Core
    Qt5::Gui
    Threads::Threads
    amarokcore
    amaroklib
    amarok-transcoding
    amarokshared
)

if(APPLE)
    set_target_properties(amarok-sqlcollection PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif()

set_target_properties(amarok-sqlcollection PROPERTIES VERSION 1.0.0 SOVERSION 1 )
install(TARGETS amarok-sqlcollection ${INSTALL_TARGETS_DEFAULT_ARGS} )

add_subdirectory( mysqlcollection )

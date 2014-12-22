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

kde4_add_library(amarok-sqlcollection SHARED ${amarok_collection-sqlcollection_SRCS})

target_link_libraries(amarok-sqlcollection
    ${KDE4_KDECORE_LIBS}
    ${KDE4_KDEUI_LIBS}
    ${KDE4_KIO_LIBS}
    ${KDE4_SOLID_LIBRARY}
    ${KDE4_THREADWEAVER_LIBRARIES}
    ${QT_QTCORE_LIBRARY}
    ${QT_QTGUI_LIBRARY}
    ${CMAKE_THREAD_LIBS_INIT}
    amarokcore
    amaroklib
    amarok-transcoding
    amarokshared
)

if(APPLE)
    SET_TARGET_PROPERTIES(amarok-sqlcollection PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)

set_target_properties(amarok-sqlcollection PROPERTIES VERSION 1.0.0 SOVERSION 1 )
install(TARGETS amarok-sqlcollection ${INSTALL_TARGETS_DEFAULT_ARGS} )

add_subdirectory( mysqlcollection )

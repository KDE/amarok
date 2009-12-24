
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
    ../sqlcollection/XesamDbus.cpp
    ../sqlcollection/XesamCollectionBuilder.cpp)
endif (NOT WIN32)

set(amarok_collection-sqlcollection_SRCS
    CapabilityDelegate.cpp
    DatabaseUpdater.cpp
    ScanManager.cpp
    ScanResultProcessor.cpp
    SqlBookmarkThisCapability.cpp
    SqlCollection.cpp
    SqlCollectionFactory.cpp
    SqlCollectionDBusHandler.cpp
    SqlCollectionLocation.cpp
    SqlQueryMaker.cpp
    SqlReadLabelCapability.cpp
    SqlRegistry.cpp
    SqlMeta.cpp
    SqlWriteLabelCapability.cpp
    ${CMAKE_SOURCE_DIR}/utilities/collectionscanner/AFTUtility.cpp
    ${extra_src}
 )

qt4_add_dbus_adaptor( amarok_collection-sqlcollection_SRCS
    org.kde.amarok.SqlCollection.xml
    SqlCollectionDBusHandler.h
    SqlCollectionDBusHandler SqlCollectionAdaptor
    SqlCollectionAdaptor)

kde4_add_library(amarok-sqlcollection SHARED ${amarok_collection-sqlcollection_SRCS})

target_link_libraries(amarok-sqlcollection
    ${KDE4_CORE_LIBS}
    ${QT_QTCORE_LIBS}
    ${TAGLIB_LIBRARIES}
    ${TAGLIB-EXTRAS_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    amaroklib
)

if(APPLE)
    SET_TARGET_PROPERTIES(amarok-sqlcollection PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)

set_target_properties(amarok-sqlcollection PROPERTIES VERSION 1.0.0 SOVERSION 1 )
install(TARGETS amarok-sqlcollection ${INSTALL_TARGETS_DEFAULT_ARGS} )

install(FILES org.kde.amarok.SqlCollection.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR})

if( BUILD_MYSQLE_COLLECTION )
    add_subdirectory( mysqlecollection )
endif( BUILD_MYSQLE_COLLECTION )
if( MYSQL_FOUND )
    add_subdirectory( mysqlservercollection )
endif( MYSQL_FOUND )


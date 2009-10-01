#build-time only dependency in XesamDbus.h
#remove at some point

include_directories(
        ../..
        ../../plugin
        ../../meta
        ../../collection
        ../../dialogs
        ../../playlistmanager
        ../../../utilities/collectionscanner
        ${CMAKE_CURRENT_BINARY_DIR}/../..
        ${KDE4_INCLUDE_DIR}
        ${STRIGI_INCLUDE_DIR}
        ${QT_INCLUDES}
        ${MYSQL_INCLUDE_DIR}
        ${TAGLIB_INCLUDES}
        ${TAGLIB-EXTRAS_INCLUDES}
        )

if (NOT WIN32)
 set (extra_src
    ../sqlcollection/XesamDbus.cpp
    ../sqlcollection/XesamCollectionBuilder.cpp)
endif (NOT WIN32)

########### mysqlserver ###############

set(amarok_collection-mysqlservercollection_PART_SRCS
    ../sqlcollection/DatabaseUpdater.cpp
    ../sqlcollection/ScanManager.cpp
    ../sqlcollection/ScanResultProcessor.cpp
    ../sqlcollection/SqlBookmarkThisCapability.cpp
    ../sqlcollection/SqlCollection.cpp
    ../sqlcollection/SqlCollectionDBusHandler.cpp
    ../sqlcollection/SqlCollectionLocation.cpp
    ../sqlcollection/SqlQueryMaker.cpp
    ../sqlcollection/SqlRegistry.cpp
    ../sqlcollection/SqlMeta.cpp
    ../mysql-shared/MySqlCollection.cpp
    MySqlServerCollection.cpp
    ../../../utilities/collectionscanner/AFTUtility.cpp
    ${extra_src}
 )

qt4_add_dbus_adaptor( amarok_collection-mysqlservercollection_PART_SRCS
    ../sqlcollection/org.kde.amarok.SqlCollection.xml
    ../sqlcollection/SqlCollectionDBusHandler.h
    SqlCollectionDBusHandler SqlCollectionAdaptor
    SqlCollectionAdaptor)

kde4_add_plugin(amarok_collection-mysqlservercollection ${amarok_collection-mysqlservercollection_PART_SRCS})

STRING(REPLACE "-Wl,--fatal-warnings" "" CMAKE_SHARED_LINKER_FLAGS_NOFATALWARN ${CMAKE_SHARED_LINKER_FLAGS})
SET(CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS_NOFATALWARN} )

STRING(REPLACE "-Wl,--fatal-warnings" "" CMAKE_MODULE_LINKER_FLAGS_NOFATALWARN ${CMAKE_MODULE_LINKER_FLAGS})
SET(CMAKE_MODULE_LINKER_FLAGS ${CMAKE_MODULE_LINKER_FLAGS_NOFATALWARN} )

target_link_libraries(amarok_collection-mysqlservercollection
    amaroklib
    amarokpud
    ${KDE4_KDEUI_LIBS}
    ${KDE4_KIO_LIBS}
    ${KDE4_THREADWEAVER_LIBRARIES}
    ${MYSQL_LIBRARIES}
    ${CMAKE_DL_LIBS}
    ${ZLIB_LIBRARIES}
    ${TAGLIB_LIBRARIES}
    ${TAGLIB-EXTRAS_LIBRARIES}
)

if(NOT WIN32)
    target_link_libraries(amarok_collection-mysqlservercollection crypto ssl)
    if(NOT APPLE)
        target_link_libraries(amarok_collection-mysqlservercollection crypt pthread)
    endif(NOT APPLE)
endif(NOT WIN32)

if(APPLE)
	SET_TARGET_PROPERTIES(amarok_collection-mysqlservercollection PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)

install(TARGETS amarok_collection-mysqlservercollection DESTINATION ${PLUGIN_INSTALL_DIR} )

install(FILES ../sqlcollection/org.kde.amarok.SqlCollection.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR})

install(FILES amarok_collection-mysqlservercollection.desktop DESTINATION ${SERVICES_INSTALL_DIR})


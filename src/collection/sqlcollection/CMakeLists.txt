#build-time only dependency in XesamDbus.h
#remove at some point

include_directories( ../..
        ../../plugin
        ../../meta
        ../../collection
        ../../dialogs
        ../../playlistmanager
        ${CMAKE_CURRENT_BINARY_DIR}/../..
        ${KDE4_INCLUDE_DIR}
        ${STRIGI_INCLUDE_DIR}
        ${QT_INCLUDES}
        ${MYSQL_INCLUDE_DIR}
        )

add_subdirectory( device )

########### next target ###############

if (NOT WIN32)
 set (extra_src
    XesamDbus.cpp
    XesamCollectionBuilder.cpp)
endif (NOT WIN32)

set(amarok_collection-sqlcollection_PART_SRCS
    DatabaseUpdater.cpp
    OrganizeCollectionDialog.cpp
    ScanManager.cpp
    ScanResultProcessor.cpp
    SqlBookmarkThisCapability.cpp
    SqlCollectionCapability.cpp
    SqlCollection.cpp
    SqlCollectionDBusHandler.cpp
    SqlCollectionLocation.cpp
    SqlQueryMaker.cpp
    SqlRegistry.cpp
    SqlMeta.cpp
    MySqlCollection.cpp
    MySqlEmbeddedCollection.cpp
    MySqlServerCollection.cpp
    ${amarok_collection-sqlcollection_PART_SRCS}
    ${extra_src}
 )

kde4_add_ui_files(amarok_collection-sqlcollection_PART_SRCS
    OrganizeCollectionDialogBase.ui
)

qt4_add_dbus_adaptor( amarok_collection-sqlcollection_PART_SRCS org.kde.amarok.sqlcollection.xml SqlCollectionDBusHandler.h SqlCollectionDBusHandler SqlCollectionAdaptor SqlCollectionAdaptor)

kde4_add_plugin(amarok_collection-sqlcollection WITH_PREFIX ${amarok_collection-sqlcollection_PART_SRCS})

STRING(REPLACE "-Wl,--fatal-warnings" "" CMAKE_SHARED_LINKER_FLAGS_NOFATALWARN ${CMAKE_SHARED_LINKER_FLAGS})
SET(CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS_NOFATALWARN} )

STRING(REPLACE "-Wl,--fatal-warnings" "" CMAKE_MODULE_LINKER_FLAGS_NOFATALWARN ${CMAKE_MODULE_LINKER_FLAGS})
SET(CMAKE_MODULE_LINKER_FLAGS ${CMAKE_MODULE_LINKER_FLAGS_NOFATALWARN} )

target_link_libraries(amarok_collection-sqlcollection
    amaroklib
    amarokpud
    ${KDE4_KDEUI_LIBS}
    ${KDE4_KIO_LIBS}
    ${KDE4_THREADWEAVER_LIBRARIES}
    ${MYSQL_EMBEDDED_LIBRARIES}
    ${MYSQL_LIBRARIES}
    ${CMAKE_DL_LIBS}
    ${ZLIB_LIBRARIES}
)

if(NOT WIN32)
    target_link_libraries(amarok_collection-sqlcollection crypto ssl)
    if(NOT APPLE)
        target_link_libraries(amarok_collection-sqlcollection crypt pthread)
    endif(NOT APPLE)
endif(NOT WIN32)

if(APPLE)
	SET_TARGET_PROPERTIES(amarok_collection-sqlcollection PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)

#if(MYSQL_FOUND)
#    set(amarok_mysql_SRCS
#        MySqlCollection.cpp
#        MySqlQueryMaker.cpp)
#
#    kde4_add_plugin(amarok_collection-mysql WITH_PREFIX ${amarok_mysql_SRCS})
#    target_link_libraries(amarok_collection-mysql amaroklib ${MYSQL_LIBS} ${KDE4_KDECORE_LIBS} ssl )
#    install(TARGETS amarok_collection-mysql DESTINATION ${PLUGIN_INSTALL_DIR})
#    install(FILES amarok_collection-mysql.desktop DESTINATION ${SERVICES_INSTALL_DIR})
#endif(MYSQL_FOUND)

install(TARGETS amarok_collection-sqlcollection DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES amarok_collection-sqlcollection.desktop DESTINATION ${SERVICES_INSTALL_DIR})

macro_optional_find_package(MySQL)

#build-time only dependency in XesamDbus.h
#remove at some point

### uncomment three lines to get mysql-embedded: 18, 48, 60
### don't forget to alter src/collection/sqlcollection/SqlCollection.cpp

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
	#	${MYSQL_INCLUDE_DIR}
        )

########### next target ###############

set(extra_src ../../sqlite/sqlite3.c)

if (NOT WIN32)
 set (extra_src     
    XesamDbus.cpp
    XesamCollectionBuilder.cpp)
 set (sqlite_deps dl)
elseif (NOT WIN32)
 set (sqlite_deps )
endif (NOT WIN32)

set(amarok_collection-sqlcollection_PART_SRCS
    DatabaseUpdater.cpp
    OrganizeCollectionDialog.cpp
    ScanManager.cpp
    ScanResultProcessor.cpp
    SqlCollection.cpp
    SqlCollectionLocation.cpp
    SqlQueryMaker.cpp
    SqlRegistry.cpp
    SqlMeta.cpp 
    SqliteCollection.cpp
    ../../sqlite/sqlite3.c
    #    MySqlEmbeddedCollection.cpp
   ${extra_src}
 )

kde4_add_ui_files(amarok_collection-sqlcollection_PART_SRCS
    OrganizeCollectionDialogBase.ui
)

kde4_add_plugin(amarok_collection-sqlcollection WITH_PREFIX ${amarok_collection-sqlcollection_PART_SRCS})

target_link_libraries(amarok_collection-sqlcollection
    amaroklib
    amarokpud
    ${KDE4_KDEUI_LIBS}
    ${KDE4_KIO_LIBS}
    ${KDE4_THREADWEAVER_LIBRARIES}
    ${sqlite_deps}
    #    ${MYSQL_EMBEDDED_LIBRARIES} crypt ssl crypto z
)

if(APPLE)
	SET_TARGET_PROPERTIES(amarok_collection-sqlcollection PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)

#if(MYSQL_FOUND)
#    set(amarok_mysql_SRCS
#        MySqlCollection.cpp
#        MySqlQueryMaker.cpp)
#
#    kde4_add_plugin(amarok_collection-mysql WITH_PREFIX ${amarok_mysql_SRCS})
#    target_link_libraries(amarok_collection-mysql amaroklib ${MYSQL_LIBS} ${KDE4_KDECORE_LIBS} )
#    install(TARGETS amarok_collection-mysql DESTINATION ${PLUGIN_INSTALL_DIR})
#    install(FILES amarok_collection-mysql.desktop DESTINATION ${SERVICES_INSTALL_DIR})
#endif(MYSQL_FOUND)

install(TARGETS amarok_collection-sqlcollection DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES amarok_collection-sqlcollection.desktop DESTINATION ${SERVICES_INSTALL_DIR})

macro_optional_find_package(MySQL)
macro_optional_find_package(PostgreSQL)

#build-time only dependency in XesamDbus.h
#remove at some point
find_package(Strigi REQUIRED)

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../..
        ${CMAKE_CURRENT_SOURCE_DIR}/../../plugin
        ${CMAKE_CURRENT_SOURCE_DIR}/../../amarokcore
        ${CMAKE_CURRENT_SOURCE_DIR}/../../meta
        ${CMAKE_CURRENT_SOURCE_DIR}/../../collection
        ${CMAKE_CURRENT_BINARY_DIR}/../..
        ${KDE4_INCLUDE_DIR}
        ${STRIGI_INCLUDE_DIR}
        ${QT_INCLUDES} )

########### next target ###############

# win32 can't compile these due to missing dependency
# msvc requires sqlite compiled in due to not being able to link it (not dllexported)
if (WIN32)
if (MSVC)
set (extra_src ../../sqlite/sqlite3.c)
else (MSVC)
set (extra_src)
endif (MSVC)
else (WIN32)
set (extra_src     
	XesamDbus.cpp
    XesamCollectionBuilder.cpp)
endif (WIN32)
	
set(amarok_collection-sqlcollection_PART_SRCS
    DatabaseUpdater.cpp
    ScanManager.cpp
    ScanResultProcessor.cpp
    SqlCollection.cpp
    SqlCollectionLocation.cpp
    SqlQueryBuilder.cpp
    SqlRegistry.cpp
    SqlMeta.cpp 
    SqliteCollection.cpp
	${extra_src}
 )


kde4_add_plugin(amarok_collection-sqlcollection WITH_PREFIX ${amarok_collection-sqlcollection_PART_SRCS})

target_link_libraries(amarok_collection-sqlcollection amaroklib ${KDE4_KDECORE_LIBS} ${KDE4_THREADWEAVER_LIBRARIES})


#if(POSTGRESQL_FOUND)
#    set(amarok_postgresql_SRCS
#       postgresqlcollection.cpp
#        postgresqlquerymaker.cpp)
#
#    kde4_add_plugin(amarok_collection-postgresql WITH_PREFIX ${amarok_postgresql_SRCS})
#    target_link_libraries(amarok_collection-postgresql amarok_collection-sqlcollection ${KDE4_KDECORE_LIBS} )
#    install(TARGETS amarok_collection-postgresql DESTINATION ${PLUGIN_INSTALL_DIR})
#    install(FILES amarok_collection-postgresql.desktop DESTINATION ${SERVICES_INSTALL_DIR})
#endif(POSTGRESQL_FOUND)

#if(MYSQL_FOUND)
#    set(amarok_mysql_SRCS
#        mysqlcollection.cpp
#        mysqlquerymaker.cpp)

#    kde4_add_plugin(amarok_collection-mysql WITH_PREFIX ${amarok_mysql_SRCS})
#    target_link_libraries(amarok_collection-mysql amarok_collection-sqlcollection ${KDE4_KDECORE_LIBS} )
#    install(TARGETS amarok_collection-mysql DESTINATION ${PLUGIN_INSTALL_DIR})
#    install(FILES amarok_collection-mysql.desktop DESTINATION ${SERVICES_INSTALL_DIR})
#endif(MYSQL_FOUND)

install(TARGETS amarok_collection-sqlcollection DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES amarok_collection-sqlcollection.desktop DESTINATION ${SERVICES_INSTALL_DIR})

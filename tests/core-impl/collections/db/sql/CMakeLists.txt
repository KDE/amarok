
# macro function to build a test target.
macro(add_database_test test_target test_sources)
    set( test_sources_internal
        ${test_sources}
        ${AMAROK_SOURCE_TREE}/core-impl/storage/sql/mysql-shared/MySqlStorage.cpp
        ${AMAROK_SOURCE_TREE}/core-impl/storage/sql/mysqlestorage/MySqlEmbeddedStorage.cpp
    )

    add_executable( ${test_target} ${test_sources} ${test_sources_internal} )
    add_test(NAME ${test_target} COMMAND $<TARGET_FILE:${test_target}>)
    ecm_mark_as_test(${test_target})

    if(APPLE)
        set_target_properties(${test_target} PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
    endif()

    target_link_libraries(${test_target}
        amarokshared
        amarokcore
        amaroklib
        amarok-sqlcollection
        KF${KF_MAJOR_VERSION}::KIOCore
        KF${KF_MAJOR_VERSION}::ThreadWeaver
        Qt::Test
        Qt::Core
        ${MYSQLE_LIBRARIES}
        ${CMAKE_DL_LIBS}
        ${GOOGLEMOCK_LIBRARIES}
    )
    if(${CMAKE_HOST_SYSTEM_NAME} MATCHES "FreeBSD")
        target_link_libraries(${test_target} ${MYSQL_LIBRARIES} crypt)
    endif()
endmacro(add_database_test)


include_directories(
    ${AMAROK_TEST_TREE}
    ${AMAROK_SOURCE_TREE}
    ${AMAROK_SOURCE_TREE}/core-impl/collections/db/sql
    ${AMAROK_UTILITY_TREE}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_BINARY_DIR}/tests
    ${AMAROK_COLLECTION_SUPPORT_DIR}
)
include_directories(SYSTEM
    ${MYSQL_INCLUDE_DIR}
)

#------------------------------- DatabaseUpdater Test -------------------------------

set( databaseupdatertest_SRCS
        TestDatabaseUpdater.cpp
        ${GOOGLEMOCK_SRCS}
    )

add_database_test( testsqldatabaseupdater "${databaseupdatertest_SRCS}" )

#-------------------------------- Test SqlAlbum -----------------------

set( testsqlalbum_SRCS
        TestSqlAlbum.cpp
        ${GOOGLEMOCK_SRCS}
    )

add_database_test( testsqlalbum "${testsqlalbum_SRCS}" )

#-------------------------------- Test SqlArtist -----------------------

set( testsqlartist_SRCS
        TestSqlArtist.cpp
        ${GOOGLEMOCK_SRCS}
    )


add_database_test( testsqlartist "${testsqlartist_SRCS}" )
#-------------------------------- Test SqlCollection -----------------------

set( testsqlcollection_SRCS
        TestSqlCollection.cpp
        ${GOOGLEMOCK_SRCS}
    )


add_database_test( testsqlcollection "${testsqlcollection_SRCS}" )

#-------------------------------- Test SqlQueryMaker -----------------------

set( testsqlquerymaker_SRCS
        TestSqlQueryMaker.cpp
        ${GOOGLEMOCK_SRCS}
    )


add_database_test( testsqlquerymaker "${testsqlquerymaker_SRCS}" )

#-------------------------------- Test SqlScanManager -----------------------

set( testsqlscanmanager_SRCS
        TestSqlScanManager.cpp
        ${GOOGLEMOCK_SRCS}
    )

add_database_test( testsqlscanmanager "${testsqlscanmanager_SRCS}" )

#-------------------------------- Test SqlTrack -----------------------

set( testsqltrack_SRCS
        TestSqlTrack.cpp
        ${AMAROK_TEST_TREE}/MetaNotificationSpy.cpp
        ${GOOGLEMOCK_SRCS}
    )


add_database_test( testsqltrack "${testsqltrack_SRCS}" )

#-------------------------------- Test SqlCollectionLocation -----------------------

set(testsqlcollectionlocation_SRCS
    TestSqlCollectionLocation.cpp
    ${AMAROK_SOURCE_TREE}/core/logger/Logger.cpp
    ${GOOGLEMOCK_SRCS}
    )

add_database_test( testsqlcollectionlocation "${testsqlcollectionlocation_SRCS}" )

if(NOT WIN32 AND NOT APPLE)
    target_link_libraries( testsqlcollectionlocation crypt pthread )
endif()


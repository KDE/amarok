macro(link_database_test test_target)

    if(APPLE)
        SET_TARGET_PROPERTIES(${test_target} PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
    endif(APPLE)

    add_dependencies( ${test_target} amarokconfig_h )
    add_dependencies( ${test_target} amaroklib)
    add_dependencies( ${test_target} amarok-sqlcollection)

    target_link_libraries(${test_target}
        amaroklib
        amarok-sqlcollection
        ${KDE4_KDECORE_LIBS}
        ${KDE4_KDEUI_LIBS}
        ${QT_QTTEST_LIBRARY}
        ${QT_QTCORE_LIBRARY}
        ${OPENSSL_LIBRARIES}
        ${MYSQL_EMBEDDED_LIBRARIES}
        ${CMAKE_DL_LIBS}
        ${ZLIB_LIBRARIES})

    if(NOT WIN32)
        target_link_libraries(${test_target} crypto ssl)
        if(NOT APPLE)
            target_link_libraries(${test_target} crypt pthread)
        endif(NOT APPLE)
    endif(NOT WIN32)

endmacro(link_database_test)


include_directories(
  ${AMAROK_TEST_TREE}
  ${AMAROK_SOURCE_TREE}
  ${AMAROK_SOURCE_TREE}/collection/sqlcollection
  ${AMAROK_UTILITY_TREE}
  ${CMAKE_BINARY_DIR}/src
  ${CMAKE_BINARY_DIR}/tests
  ${AMAROK_COLLECTION_SUPPORT_DIR}
  ${KDE4_INCLUDE_DIR}
  ${QT_INCLUDES}
  ${OPENSSL_INCLUDE_DIR}
  ${MYSQL_INCLUDE_DIR}
  )

#------------------------------- DatabaseUpdater Test -------------------------------

set( databaseupdatertest_SRCS
        DatabaseUpdaterTest.cpp
        ${AMAROK_SOURCE_TREE}/collection/sqlcollection/DatabaseUpdater.cpp
        ${AMAROK_SOURCE_TREE}/collection/sqlcollection/mysql-shared/MySqlStorage.cpp
        ${AMAROK_SOURCE_TREE}/collection/sqlcollection/mysqlecollection/MySqlEmbeddedStorage.cpp
        ${AMAROK_SOURCE_TREE}/meta/MetaUtility.cpp
        ${AMAROK_SOURCE_TREE}/Amarok.cpp
        ${AMAROK_SOURCE_TREE}/meta/PlaylistFileSupport.cpp
    )

kde4_add_unit_test( testdatabaseupdater TESTNAME amarok-testdatabaseupdater ${databaseupdatertest_SRCS} )
link_database_test( testdatabaseupdater )

#-------------------------------- Test Album Compilation Change -----------------------

set( testalbumcompilationchange_SRCS
        TestAlbumCompilationChange.cpp
        ${AMAROK_SOURCE_TREE}/meta/MetaUtility.cpp
        ${AMAROK_SOURCE_TREE}/Amarok.cpp
        ${AMAROK_SOURCE_TREE}/meta/PlaylistFileSupport.cpp
        ${AMAROK_SOURCE_TREE}/collection/sqlcollection/mysql-shared/MySqlStorage.cpp
        ${AMAROK_SOURCE_TREE}/collection/sqlcollection/mysqlecollection/MySqlEmbeddedStorage.cpp
        ${AMAROK_SOURCE_TREE}/collection/sqlcollection/SqlCollection.cpp
        ${AMAROK_SOURCE_TREE}/collection/sqlcollection/SqlMeta.cpp
        ${AMAROK_SOURCE_TREE}/collection/sqlcollection/SqlRegistry.cpp
        ${AMAROK_SOURCE_TREE}/collection/sqlcollection/DatabaseUpdater.cpp
        ${AMAROK_SOURCE_TREE}/meta/Meta.cpp
        ${AMAROK_SOURCE_TREE}/collection/Collection.cpp
        ${AMAROK_TEST_TREE}/mocks/AftUtilityMock.cpp
    )


kde4_add_unit_test( testalbumcompilationchange TESTNAME amarok-testalbumcompilationchange ${testalbumcompilationchange_SRCS} )

link_database_test( testalbumcompilationchange )


#-------------------------------- Test SqlTrack Editing -----------------------

set( testsqltrackediting_SRCS
        TestSqlTrackEditing.cpp
        ${AMAROK_SOURCE_TREE}/collection/sqlcollection/mysql-shared/MySqlStorage.cpp
        ${AMAROK_SOURCE_TREE}/collection/sqlcollection/mysqlecollection/MySqlEmbeddedStorage.cpp
        ${AMAROK_SOURCE_TREE}/collection/sqlcollection/SqlCollection.cpp
        ${AMAROK_SOURCE_TREE}/collection/sqlcollection/SqlMeta.cpp
        ${AMAROK_SOURCE_TREE}/collection/sqlcollection/SqlQueryMaker.cpp
        ${AMAROK_SOURCE_TREE}/collection/sqlcollection/SqlQueryMakerInternal.cpp
        ${AMAROK_SOURCE_TREE}/collection/sqlcollection/SqlRegistry.cpp
        ${AMAROK_SOURCE_TREE}/collection/sqlcollection/DatabaseUpdater.cpp
        ${AMAROK_SOURCE_TREE}/meta/Meta.cpp
        ${AMAROK_SOURCE_TREE}/collection/Collection.cpp
        ${AMAROK_SOURCE_TREE}/collection/QueryMaker.cpp
        ${AMAROK_TEST_TREE}/mocks/AftUtilityMock.cpp
        ${AMAROK_TEST_TREE}/MetaNotificationSpy.cpp
    )


kde4_add_unit_test( testsqltrackediting ${testsqltrackediting_SRCS} )

link_database_test( testsqltrackediting )
#-------------------------------- Test SqlArtist -----------------------

set( testsqlartist_SRCS
        TestSqlArtist.cpp
        ${AMAROK_SOURCE_TREE}/collection/sqlcollection/SqlMeta.cpp
        ${AMAROK_SOURCE_TREE}/meta/Meta.cpp
        ${AMAROK_TEST_TREE}/mocks/AftUtilityMock.cpp
    )


kde4_add_unit_test( testsqlartist ${testsqlartist_SRCS} )

link_database_test( testsqlartist )
#-------------------------------- Test SqlCollection -----------------------

set( testsqlcollection_SRCS
        TestSqlCollection.cpp
        ${AMAROK_SOURCE_TREE}/collection/sqlcollection/mysql-shared/MySqlStorage.cpp
        ${AMAROK_SOURCE_TREE}/collection/sqlcollection/mysqlecollection/MySqlEmbeddedStorage.cpp
        ${AMAROK_SOURCE_TREE}/collection/sqlcollection/SqlCollection.cpp
        ${AMAROK_SOURCE_TREE}/collection/sqlcollection/DatabaseUpdater.cpp
        ${AMAROK_SOURCE_TREE}/collection/Collection.cpp
        ${AMAROK_TEST_TREE}/mocks/AftUtilityMock.cpp
    )


kde4_add_unit_test( testsqlcollection ${testsqlcollection_SRCS} )

link_database_test( testsqlcollection )

#-------------------------------- Test SqlQueryMaker -----------------------

set( testsqlquerymaker_SRCS
        TestSqlQueryMaker.cpp
        ${AMAROK_SOURCE_TREE}/collection/sqlcollection/mysql-shared/MySqlStorage.cpp
        ${AMAROK_SOURCE_TREE}/collection/sqlcollection/mysqlecollection/MySqlEmbeddedStorage.cpp
        ${AMAROK_SOURCE_TREE}/collection/sqlcollection/SqlCollection.cpp
        ${AMAROK_SOURCE_TREE}/collection/sqlcollection/SqlMeta.cpp
        ${AMAROK_SOURCE_TREE}/collection/sqlcollection/SqlQueryMaker.cpp
        ${AMAROK_SOURCE_TREE}/collection/sqlcollection/SqlQueryMakerInternal.cpp
        ${AMAROK_SOURCE_TREE}/collection/sqlcollection/SqlRegistry.cpp
        ${AMAROK_SOURCE_TREE}/collection/sqlcollection/DatabaseUpdater.cpp
        ${AMAROK_SOURCE_TREE}/meta/Meta.cpp
        ${AMAROK_SOURCE_TREE}/collection/Collection.cpp
        ${AMAROK_SOURCE_TREE}/collection/QueryMaker.cpp
        ${AMAROK_TEST_TREE}/mocks/AftUtilityMock.cpp
    )


kde4_add_unit_test( testsqlquerymaker ${testsqlquerymaker_SRCS} )

link_database_test( testsqlquerymaker )

#-------------------------------- Test ScanResultProcessor Full scan mode -----------------------

set( testscanresultprocessorfull_SRCS
        TestScanResultProcessorFull.cpp
        ${AMAROK_SOURCE_TREE}/collection/sqlcollection/mysql-shared/MySqlStorage.cpp
        ${AMAROK_SOURCE_TREE}/collection/sqlcollection/mysqlecollection/MySqlEmbeddedStorage.cpp
        ${AMAROK_SOURCE_TREE}/collection/sqlcollection/SqlCollection.cpp
        ${AMAROK_SOURCE_TREE}/collection/sqlcollection/ScanResultProcessor.cpp
        ${AMAROK_SOURCE_TREE}/collection/sqlcollection/DatabaseUpdater.cpp
        ${AMAROK_SOURCE_TREE}/collection/Collection.cpp
    )


kde4_add_unit_test( testscanresultprocessorfull ${testscanresultprocessorfull_SRCS} )

link_database_test( testscanresultprocessorfull )
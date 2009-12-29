
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
    )

kde4_add_unit_test( testdatabaseupdater TESTNAME amarok-testdatabaseupdater ${databaseupdatertest_SRCS} )

if(APPLE)
    SET_TARGET_PROPERTIES(testdatabaseupdater PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)

target_link_libraries(testdatabaseupdater
    ${KDE4_KDECORE_LIBS}
    ${QT_QTTEST_LIBRARY}
    ${QT_QTCORE_LIBRARY}
    ${OPENSSL_LIBRARIES}
    ${MYSQL_EMBEDDED_LIBRARIES}
    ${CMAKE_DL_LIBS}
    ${ZLIB_LIBRARIES})

#-------------------------------- Test Album Compilation Change -----------------------

set( testalbumcompilationchange_SRCS
        TestAlbumCompilationChange.cpp
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

target_link_libraries(testalbumcompilationchange
    ${KDE4_KDECORE_LIBS}
    ${QT_QTTEST_LIBRARY}
    ${QT_QTCORE_LIBRARY}
    ${OPENSSL_LIBRARIES}
    ${MYSQL_EMBEDDED_LIBRARIES}
    ${CMAKE_DL_LIBS}
    ${ZLIB_LIBRARIES})



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

target_link_libraries(testsqltrackediting
    ${KDE4_KDECORE_LIBS}
    ${KDE4_THREADWEAVER_LIBRARIES}
    ${QT_QTTEST_LIBRARY}
    ${QT_QTCORE_LIBRARY}
    ${OPENSSL_LIBRARIES}
    ${MYSQL_EMBEDDED_LIBRARIES}
    ${CMAKE_DL_LIBS}
    ${ZLIB_LIBRARIES})

#-------------------------------- Test SqlArtist -----------------------

set( testsqlartist_SRCS
        TestSqlArtist.cpp
        ${AMAROK_SOURCE_TREE}/collection/sqlcollection/SqlMeta.cpp
        ${AMAROK_SOURCE_TREE}/meta/Meta.cpp
        ${AMAROK_TEST_TREE}/mocks/AftUtilityMock.cpp
    )


kde4_add_unit_test( testsqlartist ${testsqlartist_SRCS} )

target_link_libraries(testsqlartist
    ${KDE4_KDECORE_LIBS}
    ${QT_QTTEST_LIBRARY}
    ${QT_QTCORE_LIBRARY})

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

target_link_libraries(testsqlcollection
    ${KDE4_KDECORE_LIBS}
    ${KDE4_THREADWEAVER_LIBRARIES}
    ${QT_QTTEST_LIBRARY}
    ${QT_QTCORE_LIBRARY}
    ${OPENSSL_LIBRARIES}
    ${MYSQL_EMBEDDED_LIBRARIES}
    ${CMAKE_DL_LIBS}
    ${ZLIB_LIBRARIES})

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

target_link_libraries(testsqlquerymaker
    ${KDE4_KDECORE_LIBS}
    ${KDE4_THREADWEAVER_LIBRARIES}
    ${QT_QTTEST_LIBRARY}
    ${QT_QTCORE_LIBRARY}
    ${OPENSSL_LIBRARIES}
    ${MYSQL_EMBEDDED_LIBRARIES}
    ${CMAKE_DL_LIBS}
    ${CMAKE_THREAD_LIBS_INIT}
    ${ZLIB_LIBRARIES})

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

target_link_libraries(testscanresultprocessorfull
    ${KDE4_KDECORE_LIBS}
    ${KDE4_THREADWEAVER_LIBRARIES}
    ${QT_QTTEST_LIBRARY}
    ${QT_QTCORE_LIBRARY}
    ${OPENSSL_LIBRARIES}
    ${MYSQL_EMBEDDED_LIBRARIES}
    ${CMAKE_DL_LIBS}
    ${ZLIB_LIBRARIES})

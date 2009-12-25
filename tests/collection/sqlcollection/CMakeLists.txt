
include_directories(
  ${AMAROK_TEST_TREE}
  ${AMAROK_SOURCE_TREE}
  ${AMAROK_SOURCE_TREE}/collection/sqlcollection
  ${AMAROK_UTILITY_TREE}
  ${CMAKE_BINARY_DIR}/src
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

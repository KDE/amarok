
include_directories(
  ${AMAROK_TEST_TREE}
  ${AMAROK_SOURCE_TREE}
  ${AMAROK_SOURCE_TREE}/collection/sqlcollection
  ${CMAKE_BINARY_DIR}/src
  ${AMAROK_COLLECTION_SUPPORT_DIR}
  ${KDE4_INCLUDE_DIR}
  ${QT_INCLUDES}
  ${OPENSSL_INCLUDE_DIR}
  ${MYSQL_INCLUDE_DIR}
  )

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

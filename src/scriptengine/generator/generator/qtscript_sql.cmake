set( Generated_QtSql_SRCS
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_sql/qtscript_QSql.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_sql/qtscript_QSqlDatabase.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_sql/qtscript_QSqlDriver.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_sql/qtscript_QSqlDriverCreatorBase.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_sql/qtscript_QSqlError.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_sql/qtscript_QSqlField.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_sql/qtscript_QSqlIndex.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_sql/qtscript_QSqlQuery.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_sql/qtscript_QSqlQueryModel.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_sql/qtscript_QSqlRecord.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_sql/qtscript_QSqlRelation.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_sql/qtscript_QSqlRelationalTableModel.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_sql/qtscript_QSqlResult.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_sql/qtscript_QSqlTableModel.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_sql/qtscriptshell_QSqlDatabase.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_sql/qtscriptshell_QSqlDriver.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_sql/qtscriptshell_QSqlDriverCreatorBase.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_sql/qtscriptshell_QSqlQueryModel.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_sql/qtscriptshell_QSqlRelationalTableModel.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_sql/qtscriptshell_QSqlResult.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_sql/qtscriptshell_QSqlTableModel.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_sql/main.cpp
)
set_source_files_properties( ${Generated_QtSql_SRCS} PROPERTIES GENERATED true )
#qtscript bindings don't use moc
add_library( qtscript_sql MODULE ${Generated_QtSql_SRCS} )
add_dependencies( qtscript_sql generator )
target_link_libraries( qtscript_sql ${QT_LIBRARIES})
install( TARGETS qtscript_sql DESTINATION ${LIB_INSTALL_DIR}/kde4/plugins/script )

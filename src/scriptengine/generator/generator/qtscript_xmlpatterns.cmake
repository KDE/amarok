set( Generated_QtXMLPatterns_SRCS
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xmlpatterns/qtscript_QAbstractMessageHandler.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xmlpatterns/qtscript_QAbstractUriResolver.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xmlpatterns/qtscript_QAbstractXmlNodeModel.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xmlpatterns/qtscript_QAbstractXmlReceiver.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xmlpatterns/qtscript_QSimpleXmlNodeModel.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xmlpatterns/qtscript_QSourceLocation.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xmlpatterns/qtscript_QXmlFormatter.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xmlpatterns/qtscript_QXmlItem.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xmlpatterns/qtscript_QXmlName.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xmlpatterns/qtscript_QXmlNamePool.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xmlpatterns/qtscript_QXmlNodeModelIndex.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xmlpatterns/qtscript_QXmlQuery.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xmlpatterns/qtscript_QXmlResultItems.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xmlpatterns/qtscript_QXmlSerializer.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xmlpatterns/qtscriptshell_QAbstractMessageHandler.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xmlpatterns/qtscriptshell_QAbstractUriResolver.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xmlpatterns/qtscriptshell_QAbstractXmlNodeModel.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xmlpatterns/qtscriptshell_QAbstractXmlReceiver.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xmlpatterns/qtscriptshell_QSimpleXmlNodeModel.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xmlpatterns/qtscriptshell_QXmlFormatter.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xmlpatterns/qtscriptshell_QXmlResultItems.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xmlpatterns/qtscriptshell_QXmlSerializer.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xmlpatterns/main.cpp
)
set_source_files_properties( ${Generated_QtXMLPatterns_SRCS} PROPERTIES GENERATED true )
#qtscript bindings don't use moc
add_library( qtscript_xmlpatterns MODULE ${Generated_QtXMLPatterns_SRCS} )
add_dependencies( qtscript_xmlpatterns generator )
target_link_libraries( qtscript_xmlpatterns ${QT_LIBRARIES})
install( TARGETS qtscript_xmlpatterns DESTINATION lib/kde4/plugins/script )

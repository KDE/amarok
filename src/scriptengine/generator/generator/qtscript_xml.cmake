set( Generated_QtXML_SRCS
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xml/qtscript_QDomAttr.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xml/qtscript_QDomCDATASection.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xml/qtscript_QDomCharacterData.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xml/qtscript_QDomComment.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xml/qtscript_QDomDocument.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xml/qtscript_QDomDocumentFragment.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xml/qtscript_QDomDocumentType.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xml/qtscript_QDomElement.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xml/qtscript_QDomEntity.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xml/qtscript_QDomEntityReference.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xml/qtscript_QDomImplementation.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xml/qtscript_QDomNamedNodeMap.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xml/qtscript_QDomNode.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xml/qtscript_QDomNodeList.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xml/qtscript_QDomNotation.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xml/qtscript_QDomProcessingInstruction.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xml/qtscript_QDomText.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xml/qtscript_QXmlAttributes.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xml/qtscript_QXmlContentHandler.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xml/qtscript_QXmlDTDHandler.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xml/qtscript_QXmlDeclHandler.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xml/qtscript_QXmlDefaultHandler.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xml/qtscript_QXmlEntityResolver.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xml/qtscript_QXmlErrorHandler.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xml/qtscript_QXmlInputSource.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xml/qtscript_QXmlLexicalHandler.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xml/qtscript_QXmlLocator.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xml/qtscript_QXmlParseException.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xml/qtscript_QXmlReader.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xml/qtscript_QXmlSimpleReader.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xml/qtscriptshell_QXmlAttributes.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xml/qtscriptshell_QXmlContentHandler.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xml/qtscriptshell_QXmlDTDHandler.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xml/qtscriptshell_QXmlDeclHandler.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xml/qtscriptshell_QXmlDefaultHandler.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xml/qtscriptshell_QXmlEntityResolver.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xml/qtscriptshell_QXmlErrorHandler.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xml/qtscriptshell_QXmlInputSource.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xml/qtscriptshell_QXmlLexicalHandler.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xml/qtscriptshell_QXmlLocator.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xml/qtscriptshell_QXmlReader.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xml/qtscriptshell_QXmlSimpleReader.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_xml/main.cpp
)
set_source_files_properties( ${Generated_QtXML_SRCS} PROPERTIES GENERATED true )
#qtscript bindings don't use moc
add_library( qtscript_xml MODULE ${Generated_QtXML_SRCS} )
add_dependencies( qtscript_xml generator )
target_link_libraries( qtscript_xml ${QT_LIBRARIES})
install( TARGETS qtscript_xml DESTINATION lib/kde4/plugins/script )

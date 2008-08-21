set( Generated_QtWebkit_SRCS
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_webkit/qtscript_QWebFrame.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_webkit/qtscript_QWebHistoryInterface.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_webkit/qtscript_QWebHitTestResult.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_webkit/qtscript_QWebPage.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_webkit/qtscript_QWebPluginFactory.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_webkit/qtscript_QWebSettings.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_webkit/qtscript_QWebView.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_webkit/qtscriptshell_QWebHistoryInterface.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_webkit/qtscriptshell_QWebPage.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_webkit/qtscriptshell_QWebPluginFactory.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_webkit/qtscriptshell_QWebView.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_webkit/main.cpp
)
set_source_files_properties( ${Generated_QtWebkit_SRCS} PROPERTIES GENERATED true )
#qtscript bindings don't use moc
add_library( qtscript_webkit MODULE ${Generated_QtWebkit_SRCS} )
add_dependencies( qtscript_webkit generator )
target_link_libraries( qtscript_webkit ${QT_LIBRARIES})
install( TARGETS qtscript_webkit DESTINATION lib/kde4/plugins/script )

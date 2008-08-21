set( Generated_QtUiTools_SRCS
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_uitools/qtscript_QUiLoader.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_uitools/qtscriptshell_QUiLoader.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_uitools/main.cpp
)
set_source_files_properties( ${Generated_QtUiTools_SRCS} PROPERTIES GENERATED true )
add_library( qtscript_uitools MODULE ${Generated_QtUiTools_SRCS} )
add_dependencies( qtscript_uitools generator )
target_link_libraries( qtscript_uitools ${QT_LIBRARIES})
install( TARGETS qtscript_uitools DESTINATION lib/kde4/plugins/script )

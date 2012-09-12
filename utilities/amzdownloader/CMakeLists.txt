include_directories( ../
                     ${CMAKE_SOURCE_DIR}
                     ${QT_INCLUDES}
                     ${CMAKE_CURRENT_BINARY_DIR}
                   )

########### next target ###############
set( amzdownloader_UI
    AmzDownloader.ui
)

set( amzdownloader_SRCS
    main.cpp
    AmzDownloader.cpp
)

set( amzdownloader_MOC_HDRS
    AmzDownloader.h
)

qt4_wrap_ui( amzdownloader_SRCS ${amzdownloader_UI} )

qt4_wrap_cpp( amzdownloader_MOC ${amzdownloader_MOC_HDRS} )

add_executable( amzdownloader ${amzdownloader_SRCS} ${amzdownloader_MOC})

target_link_libraries( amzdownloader
                      ${QT_QTGUI_LIBRARY}
                      ${QT_QTCORE_LIBRARY}
                      )

install( TARGETS amzdownloader RUNTIME DESTINATION ${BIN_INSTALL_DIR} )

find_program( XDG-MIME_EXECUTABLE xdg-mime )
find_program( XDG-DESKTOP-MENU_EXECUTABLE xdg-desktop-menu )
execute_process( COMMAND ${XDG-MIME_EXECUTABLE} install --novendor ${CMAKE_CURRENT_LIST_DIR}/amzdownloader.xml )
execute_process( COMMAND ${XDG-DESKTOP-MENU_EXECUTABLE} install --novendor ${CMAKE_CURRENT_LIST_DIR}/amzdownloader.desktop )
execute_process( COMMAND ${XDG-MIME_EXECUTABLE} default amzdownloader.desktop "audio/x-amzxml" )

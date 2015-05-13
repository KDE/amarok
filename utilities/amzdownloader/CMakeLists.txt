macro_optional_find_package( SharedMimeInfo )

include_directories( ../
                     ${CMAKE_SOURCE_DIR}
                     
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

ki18n_wrap_ui( amzdownloader_SRCS ${amzdownloader_UI} )

qt5_wrap_cpp( amzdownloader_MOC ${amzdownloader_MOC_HDRS} )

add_executable( amzdownloader ${amzdownloader_SRCS} ${amzdownloader_MOC})

target_link_libraries( amzdownloader
                      Qt5::Gui
                      Qt5::Core
                      )

install( TARGETS amzdownloader RUNTIME DESTINATION ${BIN_INSTALL_DIR} )
install( PROGRAMS amzdownloader.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} )

if( SHAREDMIMEINFO_FOUND )
    install( FILES amzdownloader.xml DESTINATION ${XDG_MIME_INSTALL_DIR} )
    update_xdg_mimetypes( ${XDG_MIME_INSTALL_DIR} )
endif( SHAREDMIMEINFO_FOUND )

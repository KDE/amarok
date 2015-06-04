#include( KDELibsDependencies ) # for KDE version

add_subdirectory( applets )
add_subdirectory( engines )
add_subdirectory( containments )
add_subdirectory( tools )

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}
                    ..
                    ${CMAKE_CURRENT_BINARY_DIR}/..)

########### next target ###############

set(amarokpud_LIB_SRCS
    popupdropper/libpud/PopupDropper.cpp
    popupdropper/libpud/PopupDropperItem.cpp
    popupdropper/libpud/PopupDropperView.cpp
)

qt4_automoc(
    popupdropper/libpud/PopupDropper.cpp
    popupdropper/libpud/PopupDropperItem.cpp
    popupdropper/libpud/PopupDropperView.cpp
)

add_library(amarokpud SHARED ${amarokpud_LIB_SRCS})

set_target_properties( amarokpud PROPERTIES DEFINE_SYMBOL MAKE_POPUPDROPPER_LIB )

target_link_libraries(amarokpud
    Qt5::Core
    Qt5::Gui
    Qt5::Svg
    Qt5::Widgets
    Qt5::Svg
)

set_target_properties(amarokpud PROPERTIES VERSION 1.0.0 SOVERSION 1)
install(TARGETS amarokpud ${INSTALL_TARGETS_DEFAULT_ARGS} )


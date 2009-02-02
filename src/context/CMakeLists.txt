# NOTE we link libplasma here instead of in the CMakeLists in plasma/ because we need control over the installation of header files and library name
# this requires periodic syncing of this file with plasma/CMakeLists.txt as it changes, but thats better than the alternative. (the only option really.)

add_subdirectory( applets )
add_subdirectory( engines )
add_subdirectory( containments )


include_directories( ${CMAKE_CURRENT_SOURCE_DIR}
                    ..
                    plasma
                    ${CMAKE_CURRENT_BINARY_DIR}/..)

########### next target ###############

set(amarokpud_LIB_SRCS
    popupdropper/libpud/PopupDropper.cpp
    popupdropper/libpud/PopupDropperAction.cpp
    popupdropper/libpud/PopupDropperItem.cpp
    popupdropper/libpud/PopupDropperView.cpp
)

QT4_AUTOMOC(
    popupdropper/libpud/PopupDropper.cpp
    popupdropper/libpud/PopupDropperAction.cpp
    popupdropper/libpud/PopupDropperItem.cpp
    popupdropper/libpud/PopupDropperView.cpp
)

ADD_LIBRARY(amarokpud SHARED ${amarokpud_LIB_SRCS})

SET_TARGET_PROPERTIES( amarokpud PROPERTIES DEFINE_SYMBOL MAKE_POPUPDROPPER_LIB )

target_link_libraries(amarokpud
    ${QT_QTCORE_LIBRARY}
    ${QT_QTGUI_LIBRARY}
    ${QT_QTSVG_LIBRARY}
)

set_target_properties(amarokpud PROPERTIES VERSION 1.0.0 SOVERSION 1)
install(TARGETS amarokpud ${INSTALL_TARGETS_DEFAULT_ARGS} )


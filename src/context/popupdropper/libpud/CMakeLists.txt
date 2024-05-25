
set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}/popupdropper
    ${CMAKE_CURRENT_BINARY_DIR}/..
)


########### next target ###############

set(pud_LIB_SRCS
    PopupDropper.cpp
    PopupDropperItem.cpp
    PopupDropperView.cpp
)

add_library(pud SHARED ${pud_LIB_SRCS})

target_link_libraries(pud Qt::Core Qt::Gui Qt::Svg)

set_target_properties(pud PROPERTIES
        VERSION
        ${POPUPDROPPER_LIB_MAJOR_VERSION}.${POPUPDROPPER_LIB_MINOR_VERSION}.${POPUPDROPPER_LIB_PATCH_VERSION}
        SOVERSION ${POPUPDROPPER_LIB_MAJOR_VERSION}
        INSTALL_NAME_DIR ${KDE_INSTALL_LIBDIR}
)
install(TARGETS pud
	LIBRARY DESTINATION ${KDE_INSTALL_LIBDIR}
	RUNTIME DESTINATION bin
	ARCHIVE DESTINATION ${KDE_INSTALL_LIBDIR}
)

install( FILES  PopupDropper.h PopupDropperItem.h
    PopupDropperView.h PopupDropper_Export.h DESTINATION
    ${KDE_INSTALL_INCLUDEDIR}/popupdropper COMPONENT Devel)

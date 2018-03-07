
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

target_link_libraries(pud Qt5::Core Qt5::Gui Qt5::Svg)

set_target_properties(pud PROPERTIES
        VERSION
        ${POPUPDROPPER_LIB_MAJOR_VERSION}.${POPUPDROPPER_LIB_MINOR_VERSION}.${POPUPDROPPER_LIB_PATCH_VERSION}
        SOVERSION ${POPUPDROPPER_LIB_MAJOR_VERSION}
        INSTALL_NAME_DIR ${LIB_INSTALL_DIR}
)
install(TARGETS pud
	LIBRARY DESTINATION ${LIB_INSTALL_DIR}
	RUNTIME DESTINATION bin
	ARCHIVE DESTINATION  ${LIB_INSTALL_DIR}
)

install( FILES  PopupDropper.h PopupDropperItem.h
    PopupDropperView.h PopupDropper_Export.h DESTINATION
    ${INCLUDE_INSTALL_DIR}/popupdropper COMPONENT Devel)

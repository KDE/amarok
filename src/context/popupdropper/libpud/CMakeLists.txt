
set(CMAKE_INCLUDE_CURRENT_DIR ON)
INCLUDE_DIRECTORIES(
    ${CMAKE_CURRENT_BINARY_DIR}/popupdropper
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${QT_INCLUDES}
)

########### next target ###############

SET(pud_LIB_SRCS
    PopupDropper.cpp
    PopupDropperAction.cpp
    PopupDropperItem.cpp
    PopupDropperView.cpp
)

QT4_AUTOMOC(
    PopupDropper.cpp 
    PopupDropperAction.cpp 
    PopupDropperItem.cpp
    PopupDropperView.cpp
)


ADD_LIBRARY(pud SHARED ${pud_LIB_SRCS})

TARGET_LINK_LIBRARIES(pud ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${QT_QTSVG_LIBRARY})

SET_TARGET_PROPERTIES(pud PROPERTIES
        VERSION
        ${POPUPDROPPER_LIB_MAJOR_VERSION}.${POPUPDROPPER_LIB_MINOR_VERSION}.${POPUPDROPPER_LIB_PATCH_VERSION}
        SOVERSION ${POPUPDROPPER_LIB_MAJOR_VERSION}
        INSTALL_NAME_DIR ${LIB_INSTALL_DIR}
)
INSTALL(TARGETS pud
	LIBRARY DESTINATION ${LIB_INSTALL_DIR}
	RUNTIME DESTINATION bin
	ARCHIVE DESTINATION  ${LIB_INSTALL_DIR}
)

INSTALL( FILES  PopupDropper.h PopupDropperAction.h PopupDropperItem.h
    PopupDropperView.h PopupDropper_Export.h DESTINATION
    ${INCLUDE_INSTALL_DIR}/popupdropper COMPONENT Devel)

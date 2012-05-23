include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/utilities
    ${CMAKE_BINARY_DIR}/src
    ${KDE4_INCLUDE_DIR}
    ${QT_INCLUDES}
)

set(amarok_statsyncing_SRCS
    Controller.cpp
    TrackDelegate.cpp
    TrackDelegateProvider.cpp
)

kde4_add_ui_files(amarok_statsyncing_SRCS
)

kde4_add_library(amarok-statsyncing SHARED ${amarok_statsyncing_SRCS})

target_link_libraries(amarok-statsyncing
    ${KDE4_KDECORE_LIBS}
    ${KDE4_KDEUI_LIBS}
    ${QT_QTCORE_LIBRARY}
    ${QT_QTGUI_LIBRARY}
    ${CMAKE_THREAD_LIBS_INIT}
    amarokcore
)

if(APPLE)
    SET_TARGET_PROPERTIES(amarok-statsyncing PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)

set_target_properties(amarok-statsyncing PROPERTIES VERSION 1.0.0 SOVERSION 1 )
install(TARGETS amarok-statsyncing ${INSTALL_TARGETS_DEFAULT_ARGS} )

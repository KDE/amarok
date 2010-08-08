
include_directories(
    .
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/utilities
    ${CMAKE_BINARY_DIR}/src
    ${KDE4_INCLUDE_DIR}
    ${QT_INCLUDES}
)

set(amarok_transcoding_SRCS
    TranscodingJob.cpp
    TranscodingAssistantDialog.cpp
    TranscodingOptionsStackedWidget.cpp
    TranscodingPropertyWidget.cpp
)

kde4_add_ui_files(amarok_transcoding_SRCS
    TranscodingAssistantDialog.ui
)

kde4_add_library(amarok-transcoding SHARED ${amarok_transcoding_SRCS})

target_link_libraries(amarok-transcoding
    ${KDE4_KDECORE_LIBS}
    ${KDE4_KDEUI_LIBS}
    ${KDE4_KIO_LIBS}
    ${KDE4_THREADWEAVER_LIBRARIES}
    ${QT_QTCORE_LIBRARY}
    ${QT_QTGUI_LIBRARY}
    ${CMAKE_THREAD_LIBS_INIT}
    amarokcore
)

if(APPLE)
    SET_TARGET_PROPERTIES(amarok-transcoding PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)

set_target_properties(amarok-transcoding PROPERTIES VERSION 1.0.0 SOVERSION 1 )
install(TARGETS amarok-transcoding ${INSTALL_TARGETS_DEFAULT_ARGS} )


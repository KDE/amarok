
include_directories(
    .
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/utilities
    ${CMAKE_BINARY_DIR}/src
    
    
)

set(amarok_transcoding_SRCS
    TranscodingJob.cpp
    TranscodingAssistantDialog.cpp
    TranscodingOptionsStackedWidget.cpp
    TranscodingPropertyWidget.cpp
    TranscodingPropertySliderWidget.cpp
    TranscodingSelectConfigWidget.cpp
)

ki18n_wrap_ui(amarok_transcoding_SRCS
    TranscodingAssistantDialog.ui
)

add_library(amarok-transcoding SHARED ${amarok_transcoding_SRCS})
generate_export_header(amarok-transcoding BASE_NAME amarok_transcoding)
target_link_libraries(amarok-transcoding
    KF5::KIOCore
    KF5::ThreadWeaver
    Qt::Core
    Qt::Gui
    Threads::Threads
    amarokcore
)

if(APPLE)
    set_target_properties(amarok-transcoding PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif()

set_target_properties(amarok-transcoding PROPERTIES VERSION 1.0.0 SOVERSION 1 )
install(TARGETS amarok-transcoding ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )


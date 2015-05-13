include_directories(
    ${Amarok_SOURCE_DIR}/src
    ${Amarok_SOURCE_DIR}/src/context
    ${Amarok_SOURCE_DIR}/src/network
    ${CMAKE_CURRENT_BINARY_DIR} # for amarok_config.h
)

set( photos_engine_SRCS
    PhotosEngine.cpp
)

add_library(amarok_data_engine_photos MODULE ${photos_engine_SRCS})
target_link_libraries( amarok_data_engine_photos amarokcore amaroklib KF5::Plasma KF5::KIOCore)

install( TARGETS amarok_data_engine_photos DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES amarok-data-engine-photos.desktop DESTINATION ${SERVICES_INSTALL_DIR} )


include_directories( ${Amarok_SOURCE_DIR}/src
                     ${Amarok_SOURCE_DIR}/src/context
                     ${Amarok_SOURCE_DIR}/src/network
                     ${LIBLASTFM_INCLUDE_DIR}
                     ${LIBLASTFM_INCLUDE_DIR}/..
                     ${CMAKE_CURRENT_BINARY_DIR} # for amarok_config.h
)

set( similarArtists_engine_SRCS
    SimilarArtistsEngine.cpp
     ../../applets/similarartists/SimilarArtist.cpp
)

add_library(amarok_data_engine_similarArtists MODULE ${similarArtists_engine_SRCS})
target_link_libraries( amarok_data_engine_similarArtists amarokcore amaroklib ${KDE4_PLASMA_LIBS} KF5::KIOCore ${LIBLASTFM_LIBRARY} )

install( TARGETS amarok_data_engine_similarArtists DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES amarok-data-engine-similarArtists.desktop DESTINATION ${SERVICES_INSTALL_DIR} )

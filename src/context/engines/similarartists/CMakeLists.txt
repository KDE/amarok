
include_directories(SYSTEM
                     ${LIBLASTFM_INCLUDE_DIR}
                     ${LIBLASTFM_INCLUDE_DIR}/..
)
include_directories( ${Amarok_SOURCE_DIR}/src
                     ${Amarok_SOURCE_DIR}/src/context
                     ${Amarok_SOURCE_DIR}/src/network
                     ${CMAKE_CURRENT_BINARY_DIR} # for amarok_config.h
)
set( similarArtists_engine_SRCS
    SimilarArtistsEngine.cpp
     ../../applets/similarartists/SimilarArtist.cpp
)

add_library(amarok_data_engine_similarArtists MODULE ${similarArtists_engine_SRCS})
target_link_libraries( amarok_data_engine_similarArtists amarokcore amaroklib KF${KF_MAJOR_VERSION}::Plasma KF${KF_MAJOR_VERSION}::KIOCore ${LIBLASTFM_LIBRARY} )

install( TARGETS amarok_data_engine_similarArtists DESTINATION ${KDE_INSTALL_PLUGINDIR} )
install( FILES amarok-data-engine-similarArtists.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR} )


include_directories( ../../..
                     ../../../context
                     ${CMAKE_CURRENT_BINARY_DIR} # for amarok_config.h
)

set( similarArtists_engine_SRCS
    SimilarArtistsEngine.cpp
     ../../applets/similarartists/SimilarArtist.cpp
)

kde4_add_plugin(amarok_data_engine_similarArtists ${similarArtists_engine_SRCS})
target_link_libraries( amarok_data_engine_similarArtists amaroklib ${KDE4_PLASMA_LIBS} ${KDE4_KIO_LIBS} ${LIBLASTFM_LIBRARY} )

install( TARGETS amarok_data_engine_similarArtists DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES amarok-data-engine-similarArtists.desktop DESTINATION ${SERVICES_INSTALL_DIR} )

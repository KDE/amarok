# project(context-similarartists)

include_directories(
    ../..
    ../../..
    ${CMAKE_SOURCE_DIR}/src
    )

set( similarArtists_SRCS SimilarArtist.cpp ArtistWidget.cpp SimilarArtistsApplet.cpp)
ki18n_wrap_ui( similarArtists_SRCS similarArtistsSettings.ui)

add_library(amarok_context_applet_similarArtists MODULE ${similarArtists_SRCS})
if(APPLE)
   SET_TARGET_PROPERTIES(amarok_context_applet_similarArtists PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)
target_link_libraries(amarok_context_applet_similarArtists
    amarokcore
    amaroklib
    KF5::Plasma
    
)

install(TARGETS amarok_context_applet_similarArtists DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES amarok-context-applet-similarArtists.desktop DESTINATION ${SERVICES_INSTALL_DIR})

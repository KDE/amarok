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
   set_target_properties(amarok_context_applet_similarArtists PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif()
target_link_libraries(amarok_context_applet_similarArtists
    amarokcore
    amaroklib
    KF${KF_MAJOR_VERSION}::Plasma
    
)

install(TARGETS amarok_context_applet_similarArtists DESTINATION ${KDE_INSTALL_PLUGINDIR})
install(FILES amarok-context-applet-similarArtists.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

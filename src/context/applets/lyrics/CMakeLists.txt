project(context-currenttrack)

set(lyrics_SRCS
    LyricsApplet.cpp
    LyricsBrowser.cpp
    LyricsSuggestionsListWidget.cpp)

include_directories( ../..
                     ../../.. )

ki18n_wrap_ui( lyrics_SRCS lyricsSettings.ui )
add_library(amarok_context_applet_lyrics MODULE ${lyrics_SRCS})
if(APPLE)
   set_target_properties(amarok_context_applet_lyrics PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif()
target_link_libraries(amarok_context_applet_lyrics
    amarokcore
    amaroklib
    KF5::Plasma
)

install(TARGETS amarok_context_applet_lyrics DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES amarok-context-applet-lyrics.desktop DESTINATION ${SERVICES_INSTALL_DIR})

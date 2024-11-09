project(context-currenttrack)

set(songkick_SRCS SongkickApplet.cpp )

include_directories( ../..
                     ../../.. )

add_library(amarok_context_applet_songkick MODULE ${songkick_SRCS})
if(APPLE)
   set_target_properties(amarok_context_applet_songkick PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif()
target_link_libraries(amarok_context_applet_songkick amarokcore amaroklib KF${KF_MAJOR_VERSION}::Plasma KF${KF_MAJOR_VERSION}::KIOCore)

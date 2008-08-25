project(context-video)

set(video_SRCS Video.cpp )

include_directories( ../../..
                    ../.. )

kde4_add_plugin(amarok_context_applet_video ${video_SRCS})

if(APPLE)
   SET_TARGET_PROPERTIES(amarok_context_applet_video PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)
target_link_libraries(amarok_context_applet_video
    amaroklib
    amarokplasma
    ${KDE4_KDECORE_LIBS}
    ${QT_QTGUI_LIBRARY}
    ${KDE4_PHONON_LIBRARY}
)

install(TARGETS amarok_context_applet_video DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES amarok-context-applet-video.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES amarok-video.svg DESTINATION ${DATA_INSTALL_DIR}/desktoptheme/default/widgets/)


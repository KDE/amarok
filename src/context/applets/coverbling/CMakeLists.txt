project(context-coverbling)

find_package(OpenGL)

set(coverbling_SRCS
pictureflow.cpp
CoverBlingApplet.cpp
ImageLoader.cpp
PhotoBrowser.cpp
)

include_directories( ../../..
                    ../..
                    ${KDE4_INCLUDE_DIR}/amarok ) # this way we don't need to prefix it with amarok/ (and it compiles this way too :)

kde4_add_ui_files( coverbling_SRCS coverblingSettings.ui )
kde4_add_plugin(amarok_context_applet_coverbling ${coverbling_SRCS})


target_link_libraries(amarok_context_applet_coverbling
    amaroklib
    ${KDE4_PLASMA_LIBS} 
    ${KDE4_KIO_LIBS}
)

install(TARGETS amarok_context_applet_coverbling DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES amarok-context-applet-coverbling.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES blingfastback.png DESTINATION ${DATA_INSTALL_DIR}/amarok/images/ )
install(FILES blingtofirst.png DESTINATION ${DATA_INSTALL_DIR}/amarok/images/ )
install(FILES blingfastforward.png DESTINATION ${DATA_INSTALL_DIR}/amarok/images/ )
install(FILES blingtolast.png DESTINATION ${DATA_INSTALL_DIR}/amarok/images/ )
install(FILES blingdefaultcover.png DESTINATION ${DATA_INSTALL_DIR}/amarok/images/ )
install(FILES blingfullscreen.png DESTINATION ${DATA_INSTALL_DIR}/amarok/images/ )
install(FILES blingjumptoplaying.png DESTINATION ${DATA_INSTALL_DIR}/amarok/images/ )

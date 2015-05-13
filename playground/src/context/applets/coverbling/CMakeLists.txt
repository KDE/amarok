project(context-coverbling)

set(coverbling_SRCS
pictureflow.cpp
CoverBlingApplet.cpp
ImageLoader.cpp
PhotoBrowser.cpp
SearchBarTextItem.cpp
)

include_directories( ../../..
                    ../..
                    /amarok ) # this way we don't need to prefix it with amarok/ (and it compiles this way too :)

ki18n_wrap_ui( coverbling_SRCS coverblingSettings.ui )
add_library(amarok_context_applet_coverbling MODULE ${coverbling_SRCS})


target_link_libraries(amarok_context_applet_coverbling
    amarokcore
    amaroklib
    ${KDE4_PLASMA_LIBS} 
    KF5::KIOCore
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
install(FILES blingsearchalbum.png DESTINATION ${DATA_INSTALL_DIR}/amarok/images/ )
install(FILES blingsearchartist.png DESTINATION ${DATA_INSTALL_DIR}/amarok/images/ )

project(context-coverbling)

set(coverbling_SRCS
pictureflow.cpp
CoverBlingApplet.cpp
ImageLoader.cpp
PhotoBrowser.cpp
SearchBarTextItem.cpp
)

#include_directories( ../../..
#                    ../..
#                    ${KDE4_INCLUDE_DIR}/amarok ) # this way we don't need to prefix it with amarok/ (and it compiles this way too :)

ki18n_wrap_ui( coverbling_SRCS coverblingSettings.ui )
add_library(amarok_context_applet_coverbling MODULE ${coverbling_SRCS})


target_link_libraries(amarok_context_applet_coverbling
    amarokcore
    amaroklib
    KF5::Plasma 
    KF5::KIOCore
)

install(TARGETS amarok_context_applet_coverbling DESTINATION ${KDE_INSTALL_PLUGINDIR})
install(FILES amarok-context-applet-coverbling.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
install(FILES blingfastback.png DESTINATION ${KDE_INSTALL_DATADIR}/amarok/images/ )
install(FILES blingtofirst.png DESTINATION ${KDE_INSTALL_DATADIR}/amarok/images/ )
install(FILES blingfastforward.png DESTINATION ${KDE_INSTALL_DATADIR}/amarok/images/ )
install(FILES blingtolast.png DESTINATION ${KDE_INSTALL_DATADIR}/amarok/images/ )
install(FILES blingdefaultcover.png DESTINATION ${KDE_INSTALL_DATADIR}/amarok/images/ )
install(FILES blingfullscreen.png DESTINATION ${KDE_INSTALL_DATADIR}/amarok/images/ )
install(FILES blingjumptoplaying.png DESTINATION ${KDE_INSTALL_DATADIR}/amarok/images/ )
install(FILES blingsearchalbum.png DESTINATION ${KDE_INSTALL_DATADIR}/amarok/images/ )
install(FILES blingsearchartist.png DESTINATION ${KDE_INSTALL_DATADIR}/amarok/images/ )

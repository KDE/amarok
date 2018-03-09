project(context-covergrid)

set(covergrid_SRCS
    CoverGridApplet.cpp
    AlbumItem.cpp
)

#include_directories( ../../..
#                    ../..
#                    ${KDE4_INCLUDE_DIR}/amarok # this way we don't need to prefix it with amarok/ (and it compiles this way too :)
#)

ki18n_wrap_ui( covergrid_SRCS CoverGridSettings.ui )
add_library(amarok_context_applet_covergrid MODULE ${covergrid_SRCS})


target_link_libraries(amarok_context_applet_covergrid
    amarokcore
    amaroklib
    KF5::Plasma 
    KF5::KIOCore
)

install(TARGETS amarok_context_applet_covergrid DESTINATION ${KDE_INSTALL_PLUGINDIR})
install(FILES amarok-context-applet-covergrid.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

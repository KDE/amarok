project(context-currenttrack)

set(currenttrack_SRCS CurrentTrack.cpp )

include_directories( ../../
                    ../../.. )

add_library(amarok_context_applet_currenttrack MODULE ${currenttrack_SRCS})
target_link_libraries(amarok_context_applet_currenttrack
    amarokcore
    amaroklib
    KF${KF_MAJOR_VERSION}::Plasma
)

install(TARGETS amarok_context_applet_currenttrack DESTINATION ${KDE_INSTALL_PLUGINDIR})
install(FILES amarok-context-applet-currenttrack.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
install(FILES amarok-currenttrack.svg DESTINATION ${KDE_INSTALL_DATADIR}/desktoptheme/default/widgets/ )

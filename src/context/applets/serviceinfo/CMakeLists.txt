project(context-serviceinfo)

set(serviceinfo_SRCS
ServiceInfo.cpp
../../Applet.cpp )

include_directories( ../../..
                    ../..
                    ${KDE4_INCLUDE_DIR}/amarok ) # this way we don't need to prefix it with amarok/ (and it compiles this way too :)

kde4_add_plugin(amarok_context_applet_serviceinfo ${serviceinfo_SRCS})
target_link_libraries(amarok_context_applet_serviceinfo amaroklib ${QT_QTWEBKIT_LIBRARY} ${KDE4_KIO_LIBS})

install(TARGETS amarok_context_applet_serviceinfo DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES amarok-context-applet-serviceinfo.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES amarok-serviceinfo.svg DESTINATION ${DATA_INSTALL_DIR}/desktoptheme/default/widgets/ )

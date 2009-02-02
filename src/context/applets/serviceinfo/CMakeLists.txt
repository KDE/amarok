project(context-serviceinfo)

set(serviceinfo_SRCS
ServiceInfo.cpp )

include_directories( ../..
                    ../../.. )

kde4_add_plugin(amarok_context_applet_serviceinfo ${serviceinfo_SRCS})
if(APPLE)
   SET_TARGET_PROPERTIES(amarok_context_applet_serviceinfo PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)
target_link_libraries(amarok_context_applet_serviceinfo amaroklib ${KDE4_PLASMA_LIBS} ${QT_QTWEBKIT_LIBRARY} ${KDE4_KIO_LIBS})

install(TARGETS amarok_context_applet_serviceinfo DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES amarok-context-applet-serviceinfo.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES amarok-serviceinfo.svg DESTINATION ${DATA_INSTALL_DIR}/desktoptheme/default/widgets/ )
install(FILES ServiceInfoCustomStyle.css DESTINATION ${DATA_INSTALL_DIR}/amarok/data )

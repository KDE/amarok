project(context-info)

set(info_SRCS
InfoApplet.cpp )

include_directories(
                    ../..
                    ../../..
                    )

kde4_add_plugin(amarok_context_applet_info ${info_SRCS})
if(APPLE)
   SET_TARGET_PROPERTIES(amarok_context_applet_info PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)
target_link_libraries(amarok_context_applet_info amarokcore amaroklib ${KDE4_PLASMA_LIBS} ${QT_QTWEBKIT_LIBRARY} ${KDE4_KIO_LIBS})

install(TARGETS amarok_context_applet_info DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES amarok-context-applet-info.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES amarok-info-applet.svg DESTINATION ${DATA_INSTALL_DIR}/desktoptheme/default/widgets/ )
install(FILES InfoAppletCustomStyle.css DESTINATION ${DATA_INSTALL_DIR}/amarok/data )

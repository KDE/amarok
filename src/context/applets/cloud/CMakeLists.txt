project(context-cloud)

set(cloud_SRCS
Cloud.cpp
${CMAKE_SOURCE_DIR}/src/context/Applet.cpp )

include_directories( ../../..
                    ../..
                    ${KDE4_INCLUDE_DIR}/amarok ) # this way we don't need to prefix it with amarok/ (and it compiles this way too :)

kde4_add_plugin(amarok_context_applet_cloud ${cloud_SRCS})
target_link_libraries(amarok_context_applet_cloud amaroklib ${QT_QTWEBKIT_LIBRARY} ${KDE4_KIO_LIBS})

install(TARGETS amarok_context_applet_cloud DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES amarok-context-applet-cloud.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES amarok-cloud.svg DESTINATION ${DATA_INSTALL_DIR}/desktoptheme/default/widgets/ )

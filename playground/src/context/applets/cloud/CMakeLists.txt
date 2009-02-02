project(context-cloud)

set(cloud_SRCS Cloud.cpp )

include_directories( ../../../../../src/
                     ../../../../../src/context )

kde4_add_plugin(amarok_context_applet_cloud ${cloud_SRCS})
if(APPLE)
   SET_TARGET_PROPERTIES(amarok_context_applet_cloud PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)
target_link_libraries(amarok_context_applet_cloud amaroklib ${KDE4_PLASMA_LIBS} ${QT_QTWEBKIT_LIBRARY} ${KDE4_KIO_LIBS})

install(TARGETS amarok_context_applet_cloud DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES amarok-context-applet-cloud.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES amarok-cloud.svg DESTINATION ${DATA_INSTALL_DIR}/desktoptheme/default/widgets/ )

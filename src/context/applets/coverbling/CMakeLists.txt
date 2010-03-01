project(context-coverbling)

set(coverbling_SRCS
pictureflow.cpp
CoverBlingApplet.cpp)

include_directories( ../../..
                    ../..
                    ${KDE4_INCLUDE_DIR}/amarok ) # this way we don't need to prefix it with amarok/ (and it compiles this way too :)

kde4_add_plugin(amarok_context_applet_coverbling ${coverbling_SRCS})

target_link_libraries(amarok_context_applet_coverbling
    amaroklib
    ${KDE4_PLASMA_LIBS} 
    ${QT_QTOPENGL_LIBRARY}
    ${OPENGL_gl_LIBRARY}
    ${OPENGL_glu_LIBRARY}
    ${KDE4_KIO_LIBS}
)

install(TARGETS amarok_context_applet_coverbling DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES amarok-context-applet-coverbling.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES blingdefaultcover.png DESTINATION ${DATA_INSTALL_DIR}/amarok/images/ )

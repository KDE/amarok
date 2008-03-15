project(context-containment)

include_directories( ../../..
                     ../plasma
                     ../..
                     ..
                     ${KDE4_INCLUDE_DIR}/amarok )
                     
set(context_SRCS
    ColumnApplet.cpp
    ../../widgets/SvgHandler.cpp)

kde4_add_plugin(amarok_containment_context ${context_SRCS})
target_link_libraries(amarok_containment_context amaroklib ${KDE4_KIO_LIBS})

install(TARGETS amarok_containment_context DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES amarok-containment-context.desktop DESTINATION ${SERVICES_INSTALL_DIR})

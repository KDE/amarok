project(context-containment)

include_directories( ../../..
                     ../..
                     ..
                     ../plasma )

set(context_SRCS ColumnContainment.cpp )

kde4_add_plugin(amarok_containment_context ${context_SRCS})
if(APPLE)
   SET_TARGET_PROPERTIES(amarok_containment_context PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)
target_link_libraries(amarok_containment_context amaroklib ${KDE4_KIO_LIBS})

install(TARGETS amarok_containment_context DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES amarok-containment-context.desktop DESTINATION ${SERVICES_INSTALL_DIR})

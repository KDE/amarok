project(context-vertical-containment)

include_directories( toolbar
		     ../../..
                     ../..
                     ..
                     ../plasma )

set(context_SRCS 
	VerticalToolbarContainment.cpp 
	VerticalAppletLayout.cpp
	)

kde4_add_plugin(amarok_containment_vertical ${context_SRCS})
if(APPLE)
   SET_TARGET_PROPERTIES(amarok_containment_vertical PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)
target_link_libraries(amarok_containment_vertical amarokcore amaroklib ${KDE4_PLASMA_LIBS} ${KDE4_KIO_LIBS})

install(TARGETS amarok_containment_vertical DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES amarok-containment-vertical.desktop DESTINATION ${SERVICES_INSTALL_DIR})

project(context-horizontal-containment)

include_directories( ../../..
                     ../..
                     ..
                     ../plasma )

set(context_SRCS 
	HorizontalToolbarContainment.cpp 
	AppletToolbar.cpp 
	AppletToolbarItem.cpp
	HorizontalAppletLayout.cpp
	)

kde4_add_plugin(amarok_containment_horizontal ${context_SRCS})
if(APPLE)
   SET_TARGET_PROPERTIES(amarok_containment_horizontal PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)
target_link_libraries(amarok_containment_horizontal amaroklib amarokplasma ${KDE4_KIO_LIBS})

install(TARGETS amarok_containment_horizontal DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES amarok-containment-horizontal.desktop DESTINATION ${SERVICES_INSTALL_DIR})

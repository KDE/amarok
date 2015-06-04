project(context-vertical-containment)

include_directories( ../../..
                     ../..
                     .. )

set(context_SRCS 
	VerticalToolbarContainment.cpp 
	VerticalAppletLayout.cpp
	)

add_library(amarok_containment_vertical MODULE ${context_SRCS})
if(APPLE)
   set_target_properties(amarok_containment_vertical PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif()
target_link_libraries(amarok_containment_vertical amarokcore amaroklib KF5::Plasma KF5::KIOCore)

install(TARGETS amarok_containment_vertical DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES amarok-containment-vertical.desktop DESTINATION ${SERVICES_INSTALL_DIR})

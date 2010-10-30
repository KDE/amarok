project(context-labels)

set(labels_SRCS LabelsApplet.cpp LabelGraphicsItem.cpp LabelOverlayButton.cpp)

include_directories( ../..
                     ../../.. )

kde4_add_ui_files( labels_SRCS labelsSettings.ui )
kde4_add_plugin(amarok_context_applet_labels ${labels_SRCS})
if(APPLE)
   SET_TARGET_PROPERTIES(amarok_context_applet_labels PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)
target_link_libraries(amarok_context_applet_labels amarokcore amaroklib ${KDE4_PLASMA_LIBS} ${KDE4_KIO_LIBS})

install(TARGETS amarok_context_applet_labels DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES amarok-context-applet-labels.desktop DESTINATION ${SERVICES_INSTALL_DIR})

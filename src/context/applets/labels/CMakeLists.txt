project(context-labels)

set(labels_SRCS LabelsApplet.cpp LabelGraphicsItem.cpp LabelOverlayButton.cpp)

include_directories( ../..
                     ../../.. )

ki18n_wrap_ui( labels_SRCS labelsGeneralSettings.ui labelsBlacklistSettings.ui labelsReplacementSettings.ui )
add_library(amarok_context_applet_labels MODULE ${labels_SRCS})
if(APPLE)
   SET_TARGET_PROPERTIES(amarok_context_applet_labels PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)
target_link_libraries(amarok_context_applet_labels
    amarokcore
    amaroklib
    ${KDE4_PLASMA_LIBS}
)

install(TARGETS amarok_context_applet_labels DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES amarok-context-applet-labels.desktop DESTINATION ${SERVICES_INSTALL_DIR})

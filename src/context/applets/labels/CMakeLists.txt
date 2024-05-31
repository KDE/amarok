project(context-labels)

set(labels_SRCS LabelsApplet.cpp LabelGraphicsItem.cpp LabelOverlayButton.cpp)

include_directories( ../..
                     ../../.. )

ki18n_wrap_ui( labels_SRCS labelsGeneralSettings.ui labelsBlacklistSettings.ui labelsReplacementSettings.ui )
add_library(amarok_context_applet_labels MODULE ${labels_SRCS})
if(APPLE)
   set_target_properties(amarok_context_applet_labels PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif()
target_link_libraries(amarok_context_applet_labels
    amarokcore
    amaroklib
    KF${KF_MAJOR_VERSION}::Plasma
)

install(TARGETS amarok_context_applet_labels DESTINATION ${KDE_INSTALL_PLUGINDIR})
install(FILES amarok-context-applet-labels.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

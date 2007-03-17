
#add_subdirectory( config )
add_subdirectory( equalizer )

include_directories(
${CMAKE_CURRENT_SOURCE_DIR}}/../../engine/gst10/config
${CMAKE_CURRENT_SOURCE_DIR}}/../../plugin
${CMAKE_CURRENT_SOURCE_DIR}}/../..
${CMAKE_BINARY_DIR}/amarok/src #for amarokconfig.h
${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${GOBJECT_INCLUDE_DIR}
${GSTREAMER_INCLUDE_DIR}  )

add_definitions( ${GOBJECT_DEFINITIONS} ${GSTREAMER_DEFINITIONS} )

########### next target ###############

set(amarok_gst10engine_plugin_PART_SRCS
	gstconfigdialog.cpp
	gstengine.cpp
	streamprovider.cpp
	streamsrc.cpp
	equalizer/gstequalizer.cpp
	gstconfig.cpp
	)

kde4_add_ui3_files(amarok_gst10engine_plugin_PART_SRCS gstconfigdialogbase.ui )

kde4_add_kcfg_files( amarok_gst10engine_PART_SRCS gstconfig.kcfgc )

kde4_automoc(${amarok_gst10engine_plugin_PART_SRCS})

kde4_add_plugin(amarok_gst10engine_plugin WITH_PREFIX
${amarok_gst10engine_plugin_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} amarok_gst10engine_plugin )

target_link_libraries(amarok_gst10engine_plugin  ${KDE4_KDECORE_LIBS} amarok
${GOBJECT_LIBRARIES} ${GSTREAMER_LIBRARIES} ${GSTREAMER_INTERFACE_LIBRARY} )

install(TARGETS amarok_gst10engine_plugin DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES  amarok_gst10engine_plugin.desktop DESTINATION
${SERVICES_INSTALL_DIR})
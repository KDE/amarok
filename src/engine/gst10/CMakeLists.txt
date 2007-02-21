
#add_subdirectory( config )
add_subdirectory( equalizer )

include_directories(
${CMAKE_SOURCE_DIR}/amarok/src/engine/gst10/config
${CMAKE_SOURCE_DIR}/amarok/src/plugin
${CMAKE_SOURCE_DIR}/amarok/src
${CMAKE_BINARY_DIR}/amarok/src #for amarokconfig.h
${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${GOBJECT_INCLUDE_DIR} ${GSTREAMER_INCLUDE_DIR}  )

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

kde4_add_plugin(amarok_gst10engine_plugin WITH_PREFIX ${amarok_gst10engine_plugin_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} amarok_gst10engine_plugin )

target_link_libraries(amarok_gst10engine_plugin  ${KDE4_KDECORE_LIBS} amarok
${GOBJECT_LIBRARIES} ${GSTREAMER_LIBRARIES} ${GSTREAMER_INTERFACE_LIBRARY} )

install(TARGETS amarok_gst10engine_plugin DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES  amarok_gst10engine_plugin.desktop DESTINATION ${SERVICES_INSTALL_DIR})



#original Makefile.am contents follow:

#kde_module_LTLIBRARIES = \
#    libamarok_gst10engine_plugin.la
#
#SUBDIRS = \
#    config equalizer
#
#INCLUDES = \
#    -I$(top_srcdir)/amarok/src/engine/gst10/config \
#    -I$(top_srcdir)/amarok/src/plugin \
#    -I$(top_srcdir)/amarok/src \
#    $(CFLAGS_GST10) \
#    $(all_includes)
#
#libamarok_gst10engine_plugin_la_LIBADD = \
#    $(top_builddir)/amarok/src/libamarok.la \
#    $(top_builddir)/amarok/src/engine/gst10/config/libgstconfig.la \
#    $(top_builddir)/amarok/src/engine/gst10/equalizer/libgstequalizer.la \
#    $(top_builddir)/amarok/src/plugin/libplugin.la \
#    $(LIB_KFILE) \
#    $(LIB_KDECORE)
#
#libamarok_gst10engine_plugin_la_LDFLAGS = \
#    $(LIB_GST10) \
#    $(KDE_PLUGIN) \
#    $(all_libraries)
#
#libamarok_gst10engine_plugin_la_SOURCES = \
#    gstconfigdialog.cpp \
#    gstconfigdialogbase.ui \
#    gstengine.cpp
#
#METASOURCES = \
#    AUTO
#
#kde_services_DATA = \
#    amarok_gst10engine_plugin.desktop

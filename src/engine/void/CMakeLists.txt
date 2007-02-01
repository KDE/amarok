
include_directories( ${CMAKE_SOURCE_DIR}/amarok/src/plugin ${CMAKE_SOURCE_DIR}/amarok/src ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )


########### next target ###############

set(amarok_void-engine_plugin_PART_SRCS void-engine.cpp )

kde4_automoc(${amarok_void-engine_plugin_PART_SRCS})

kde4_add_plugin(amarok_void-engine_plugin WITH_PREFIX ${amarok_void-engine_plugin_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} amarok_void-engine_plugin )

target_link_libraries(amarok_void-engine_plugin  ${KDE4_KDECORE_LIBS} )

install_targets(${PLUGIN_INSTALL_DIR} amarok_void-engine_plugin )


########### install files ###############

install_files( ${SERVICES_INSTALL_DIR} FILES  amarok_void-engine_plugin.desktop )



#original Makefile.am contents follow:

#kde_module_LTLIBRARIES = \
#    libamarok_void-engine_plugin.la
#
#INCLUDES = \
#    -I$(top_srcdir)/amarok/src/plugin \
#    -I$(top_srcdir)/amarok/src \
#    $(all_includes)
#
#libamarok_void_engine_plugin_la_LIBADD = \
#    $(top_builddir)/amarok/src/libamarok.la \
#    $(top_builddir)/amarok/src/plugin/libplugin.la \
#    $(LIB_KDECORE)
#
#libamarok_void_engine_plugin_la_SOURCES = \
#    void-engine.cpp
#
#libamarok_void_engine_plugin_la_LDFLAGS = \
#    $(KDE_PLUGIN) \
#    $(all_libraries)
#
#METASOURCES = \
#    AUTO
#
#kde_services_DATA = \
#    amarok_void-engine_plugin.desktop

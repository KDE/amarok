
include_directories( ${CMAKE_SOURCE_DIR}/amarok/src/engine ${CMAKE_SOURCE_DIR}/amarok/src/plugin ${CMAKE_SOURCE_DIR}/amarok/src ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )


########### next target ###############

set(amarok_masengine_plugin_PART_SRCS masengine.cpp )

kde4_automoc(${amarok_masengine_plugin_PART_SRCS})

kde4_add_plugin(amarok_masengine_plugin WITH_PREFIX ${amarok_masengine_plugin_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} amarok_masengine_plugin )

target_link_libraries(amarok_masengine_plugin  ${KDE4_KDECORE_LIBS} engine plugin ${KDE4_KDEUI_LIBS} )

install(TARGETS amarok_masengine_plugin DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install(FILES  amarok_masengine_plugin.desktop DESTINATION  ${SERVICES_INSTALL_DIR} )



#original Makefile.am contents follow:

#kde_module_LTLIBRARIES = libamarok_masengine_plugin.la
#kde_services_DATA = amarok_masengine_plugin.desktop
#
#INCLUDES = \
#    -I$(top_srcdir)/amarok/src/engine \
#    -I$(top_srcdir)/amarok/src/plugin \
#    -I$(top_srcdir)/amarok/src \
#    $(CFLAGS_MAS) $(all_includes)
#
#libamarok_masengine_plugin_la_LIBADD = \
#    $(top_builddir)/amarok/src/engine/libengine.la \
#    $(top_builddir)/amarok/src/plugin/libplugin.la \
#    $(LIB_KFILE) $(LIB_KDEUI) $(LIB_KDECORE) \
#    $(LIBS_MAS)
#
#libamarok_masengine_plugin_la_SOURCES = \
#    masengine.cpp
#
#libamarok_masengine_plugin_la_LDFLAGS = \
#    -module \
#    -no-undefined \
#    $(KDE_PLUGIN) \
#    $(all_libraries) 
#
#METASOURCES = AUTO
#
#
#

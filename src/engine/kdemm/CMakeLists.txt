
include_directories( ${CMAKE_SOURCE_DIR}/amarok/src/engine ${CMAKE_SOURCE_DIR}/amarok/src/plugin ${CMAKE_SOURCE_DIR}/amarok/src ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )


########### next target ###############

set(amarok_kdemmengine_plugin_PART_SRCS kdemmengine.cpp )

kde4_automoc(${amarok_kdemmengine_plugin_PART_SRCS})

kde4_add_plugin(amarok_kdemmengine_plugin WITH_PREFIX ${amarok_kdemmengine_plugin_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} amarok_kdemmengine_plugin )

target_link_libraries(amarok_kdemmengine_plugin  ${KDE4_KDECORE_LIBS} amarok plugin ${KDE4_KDEUI_LIBS} kdemm )

install_targets(${PLUGIN_INSTALL_DIR} amarok_kdemmengine_plugin )


########### install files ###############

install_files( ${SERVICES_INSTALL_DIR} FILES  amarok_kdemmengine_plugin.desktop )



#original Makefile.am contents follow:

#kde_module_LTLIBRARIES = libamarok_kdemmengine_plugin.la
#kde_services_DATA = amarok_kdemmengine_plugin.desktop
#
#INCLUDES = \
#    -I$(top_srcdir)/amarok/src/engine \
#    -I$(top_srcdir)/amarok/src/plugin \
#    -I$(top_srcdir)/amarok/src \
#    $(CFLAGS_MAS) $(all_includes)
#
#libamarok_kdemmengine_plugin_la_LIBADD = \
#    $(top_builddir)/amarok/src/libamarok.la \
#    $(top_builddir)/amarok/src/plugin/libplugin.la \
#    $(LIB_KFILE) $(LIB_KDEUI) $(LIB_KDECORE) \
#    $(LIBS_KDEMM) -lkdemm
#
#libamarok_kdemmengine_plugin_la_SOURCES = \
#    kdemmengine.cpp
#
#libamarok_kdemmengine_plugin_la_LDFLAGS = \
#    -module \
#    -no-undefined \
#    $(KDE_PLUGIN) \
#    $(all_libraries)
#
#METASOURCES = AUTO
#
#
#

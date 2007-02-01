
add_subdirectory( helix-sp )
add_subdirectory( config )

include_directories( ${CMAKE_SOURCE_DIR}/amarok/src ${CMAKE_SOURCE_DIR}/amarok/src/amarokcore ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )


########### next target ###############

set(amarok_helixengine_plugin_PART_SRCS 
   helix-engine.cpp 
   helix-errors.cpp 
   helix-configdialog.cpp 
   hxplayercontrol.cpp )

kde4_automoc(${amarok_helixengine_plugin_PART_SRCS})

kde4_add_plugin(amarok_helixengine_plugin WITH_PREFIX ${amarok_helixengine_plugin_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} amarok_helixengine_plugin )

target_link_libraries(amarok_helixengine_plugin  ${KDE4_KDECORE_LIBS} amarok helixconfig plugin kdeui kdecore )

install_targets(${PLUGIN_INSTALL_DIR} amarok_helixengine_plugin )


########### install files ###############

install_files( ${SERVICES_INSTALL_DIR} FILES  amarok_helixengine_plugin.desktop )



#original Makefile.am contents follow:

#kde_module_LTLIBRARIES = \
#    libamarok_helixengine_plugin.la
#
#SUBDIRS = \
#    helix-sp \
#    config
#
#INCLUDES = \
#    -I$(top_srcdir)/amarok/src \
#    -I$(top_srcdir)/amarok/src/amarokcore \
#    $(all_includes)
#
#AM_CPPFLAGS = \
#    -D_UNIX \
#    -Wall -Wreturn-type \
#    -fno-exceptions \
#    -I$(top_srcdir)/amarok/src/engine/helix/helix-sp \
#    -I$(top_srcdir)/amarok/src/engine/helix/config \
#    -I$(top_srcdir)/amarok/src/ \
#    -include $(top_srcdir)/amarok/src/engine/helix/helix-sp/helixdefines.h \
#    $(all_includes)
#
#libamarok_helixengine_plugin_la_SOURCES = \
#    helix-engine.cpp \
#    helix-errors.cpp \
#    helix-configdialog.cpp \
#    hxplayercontrol.cpp
#
#libamarok_helixengine_plugin_la_LIBADD = \
#    $(top_builddir)/amarok/src/libamarok.la \
#    $(top_builddir)/amarok/src/engine/helix/helix-sp/libhelix-sp.la \
#    $(top_builddir)/amarok/src/engine/helix/config/libhelixconfig.la \
#    $(top_builddir)/amarok/src/plugin/libplugin.la \
#    -lkdeui -lkdecore $(ALSALIB_LIBS)
#
#libamarok_helixengine_plugin_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)
#
#METASOURCES = \
#    AUTO
#
#kde_services_DATA = \
#    amarok_helixengine_plugin.desktop
#
#KDE_OPTIONS=nofinal
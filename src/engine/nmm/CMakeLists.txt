
add_subdirectory( icons )

include_directories( ${CMAKE_SOURCE_DIR}/amarok/src ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )


########### next target ###############

set(amarok_nmmengine_plugin_PART_SRCS 
   nmm_engine.cpp 
   nmm_configdialog.cpp 
   NmmLocation.cpp 
   HostList.cpp 
   HostListItem.cpp 
   ServerregistryPing.cpp )

kde4_automoc(${amarok_nmmengine_plugin_PART_SRCS})

kde4_add_ui_files(amarok_nmmengine_plugin_PART_SRCS nmm_configdialogbase.ui )

kde4_add_kcfg_files(amarok_nmmengine_plugin_PART_SRCS nmm_kdeconfig.kcfgc )

kde4_add_plugin(amarok_nmmengine_plugin WITH_PREFIX ${amarok_nmmengine_plugin_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} amarok_nmmengine_plugin )

target_link_libraries(amarok_nmmengine_plugin  ${KDE4_KDECORE_LIBS} amarok plugin nmmbase nmmgraphmgr nmmiprogress nmmiaudiodevice nmmigeneral )

install_targets(${PLUGIN_INSTALL_DIR} amarok_nmmengine_plugin )


########### install files ###############

install_files( ${KCFG_INSTALL_DIR} FILES  nmm_kdeconfig.kcfg )
install_files( ${SERVICES_INSTALL_DIR} FILES  amarok_nmmengine_plugin.desktop )
install_files( ${DATA_INSTALL_DIR}/amarok/images FILES  nmm-gradient-left.png      nmm-gradient-right.png      nmm-volume-inset.png )



#original Makefile.am contents follow:

#imagesdir = $(kde_datadir)/amarok/images
#images_DATA = \
#    nmm-gradient-left.png \
#    nmm-gradient-right.png \
#    nmm-volume-inset.png
#
#kde_module_LTLIBRARIES = \
#    libamarok_nmmengine_plugin.la
#
#INCLUDES = \
#    -I$(top_srcdir)/amarok/src \
#    $(CFLAGS_NMM) \
#    $(all_includes)
#
#libamarok_nmmengine_plugin_la_LIBADD = \
#    $(top_builddir)/amarok/src/libamarok.la \
#    $(top_builddir)/amarok/src/plugin/libplugin.la \
#    $(LIB_KDECORE) $(LIB_KFILE) \
#    -lnmmbase -lnmmgraphmgr -lnmmiprogress -lnmmiaudiodevice -lnmmigeneral
#
#libamarok_nmmengine_plugin_la_SOURCES = \
#    nmm_engine.cpp \
#    nmm_configdialog.cpp \
#    nmm_configdialogbase.ui \
#    NmmLocation.cpp \
#    HostList.cpp \
#    HostListItem.cpp \
#    ServerregistryPing.cpp \
#    nmm_kdeconfig.kcfgc
#
#libamarok_nmmengine_plugin_la_LDFLAGS = \
#    -module \
#    $(KDE_PLUGIN) \
#    $(LDFLAGS_NMM) \
#    $(all_libraries)
#
#METASOURCES = \
#    AUTO
#
#KDE_CXXFLAGS = $(USE_EXCEPTIONS)
#
#kde_services_DATA = \
#    amarok_nmmengine_plugin.desktop
#
#kde_kcfg_DATA = nmm_kdeconfig.kcfg
#
#SUBDIRS = icons

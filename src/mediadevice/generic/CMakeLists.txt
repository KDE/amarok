
include_directories(
    ../..
	../../amarokcore 
	../../engine
	../../mediadevice
    ../../meta
	${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )


########### next target ###############

set(amarok_generic-mediadevice_PART_SRCS
    genericmediadevice.cpp
    genericmediadeviceconfigdialog.ui.h )

kde4_add_ui3_files(amarok_generic-mediadevice_PART_SRCS
    genericmediadeviceconfigdialog.ui )

kde4_add_plugin(amarok_generic-mediadevice WITH_PREFIX ${amarok_generic-mediadevice_PART_SRCS})

target_link_libraries(amarok_generic-mediadevice ${KDE4_KDECORE_LIBS} amaroklib)

install(TARGETS amarok_generic-mediadevice DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install(FILES amarok_generic-mediadevice.desktop DESTINATION ${SERVICES_INSTALL_DIR})


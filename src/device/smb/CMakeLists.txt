include_directories( ${CMAKE_SOURCE_DIR}/amarok/src ${CMAKE_SOURCE_DIR}/amarok/s
rc/device ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )

########### next target ###############

set(libamarok_smb-device_PART_SRCS smbdevicehandler.cpp )

kde4_automoc(${libamarok_smb-device_PART_SRCS})

kde4_add_plugin(libamarok_smb-device WITH_PREFIX
${libamarok_smb-device_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} libamarok_smb-device )

target_link_libraries(libamarok_smb-device  ${KDE4_KDECORE_LIBS} )

install( TARGETS libamarok_smb-device DETINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES amarok_smb-device.desktop DESTINATION ${SERVICES_INSTALL_DIR} )


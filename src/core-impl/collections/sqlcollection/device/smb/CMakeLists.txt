include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../device
    ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} )

########### next target ###############

set(amarok_smb-device_PART_SRCS SmbDeviceHandler.cpp )

kde4_add_plugin(amarok_smb-device 
${amarok_smb-device_PART_SRCS})

target_link_libraries(amarok_smb-device amarok  ${KDE4_KDECORE_LIBS} )

install( TARGETS amarok_smb-device DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES amarok_smb-device.desktop DESTINATION ${SERVICES_INSTALL_DIR} )


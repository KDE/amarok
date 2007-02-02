include_directories( ${CMAKE_SOURCE_DIR}/amarok/src ${CMAKE_SOURCE_DIR}/amarok/s
rc/device ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )

########### next target ###############

set(libamarok_nfs-device_PART_SRCS nfsdevicehandler.cpp )

kde4_automoc(${libamarok_nfs-device_PART_SRCS})

kde4_add_plugin(libamarok_nfs-device WITH_PREFIX
${libamarok_nfs-device_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} libamarok_nfs-device )

target_link_libraries(libamarok_nfs-device  ${KDE4_KDECORE_LIBS} )

install( TARGETS libamarok_nfs-device DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES amarok_nfs-device.desktop DESTINATION ${SERVICES_INSTALL_DIR} )


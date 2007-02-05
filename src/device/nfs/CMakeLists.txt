include_directories( ${CMAKE_SOURCE_DIR}/amarok/src ${CMAKE_SOURCE_DIR}/amarok/s
rc/device ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )

########### next target ###############

set(amarok_nfs-device_PART_SRCS nfsdevicehandler.cpp )

kde4_automoc(${amarok_nfs-device_PART_SRCS})

kde4_add_plugin(amarok_nfs-device WITH_PREFIX
${amarok_nfs-device_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} amarok_nfs-device )

target_link_libraries(amarok_nfs-device amarok ${KDE4_KDECORE_LIBS} )

install( TARGETS amarok_nfs-device DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES amarok_nfs-device.desktop DESTINATION ${SERVICES_INSTALL_DIR} )


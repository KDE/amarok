include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../device
    ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )

########### next target ###############

set(amarok_nfs-device_PART_SRCS NfsDeviceHandler.cpp )

kde4_add_plugin(amarok_nfs-device 
${amarok_nfs-device_PART_SRCS})

target_link_libraries(amarok_nfs-device amarok ${KDE4_KDECORE_LIBS} )

install( TARGETS amarok_nfs-device DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES amarok_nfs-device.desktop DESTINATION ${SERVICES_INSTALL_DIR} )


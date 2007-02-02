include_directories( ${CMAKE_SOURCE_DIR}/amarok/src ${CMAKE_SOURCE_DIR}/amarok/s
rc/device ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )

########### next target ###############

set(libamarok_massstorage-device_PART_SRCS massstoragedevicehandler.cpp )

kde4_automoc(${libamarok_massstorage-device_PART_SRCS})

kde4_add_plugin(libamarok_massstorage-device WITH_PREFIX 
${libamarok_massstorage-device_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} libamarok_massstorage-device )

target_link_libraries(libamarok_massstorage-device  ${KDE4_KDECORE_LIBS} )

install_targets(${PLUGIN_INSTALL_DIR} libamarok_massstorage-device )


########### install files ###############

install_files( ${SERVICES_INSTALL_DIR} FILES  amarok_massstorage-device.desktop )


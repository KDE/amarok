include_directories( ${CMAKE_SOURCE_DIR}/amarok/src ${CMAKE_SOURCE_DIR}/amarok/src/device ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )

########### next target ###############

set(amarok_massstorage-device_PART_SRCS massstoragedevicehandler.cpp )

kde4_automoc(${amarok_massstorage-device_PART_SRCS})

kde4_add_plugin(amarok_massstorage-device WITH_PREFIX 
${amarok_massstorage-device_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} amarok_massstorage-device )

target_link_libraries(amarok_massstorage-device amarok ${KDE4_KDECORE_LIBS} )

install(TARGETS amarok_massstorage-device DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install(FILES  amarok_massstorage-device.desktop  DESTINATION ${SERVICES_INSTALL_DIR})


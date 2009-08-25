include_directories(
    ../../..
    ../../../plugin
    ../../../meta
    ../../../collection
    ${CMAKE_CURRENT_SOURCE_DIR}/sqlcollection
    ${CMAKE_CURRENT_BINARY_DIR}/sqlcollection
    ${KDE4_INCLUDE_DIR}
    ${QT_INCLUDES}
    )

########### next target ###############

set(amarok_massstorage-device_PART_SRCS MassStorageDeviceHandler.cpp )

kde4_add_plugin(amarok_massstorage-device 
${amarok_massstorage-device_PART_SRCS})

target_link_libraries(amarok_massstorage-device
    amaroklib
    ${KDE4_KDECORE_LIBS}
    ${KDE4_SOLID_LIBS} )

install(TARGETS amarok_massstorage-device DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install(FILES  amarok_massstorage-device.desktop  DESTINATION ${SERVICES_INSTALL_DIR})


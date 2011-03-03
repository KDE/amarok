include_directories(
    ${Amarok_SOURCE_DIR}/src/plugin
    ${Amarok_SOURCE_DIR}/src/collection/sqlcollection
    ${KDE4_INCLUDE_DIR}
    ${QT_INCLUDES}
    )

########### next target ###############

set(amarok_device_smb_PART_SRCS SmbDeviceHandler.cpp )

kde4_add_plugin(amarok_device_smb
${amarok_device_smb_PART_SRCS})

target_link_libraries(amarok_device_smb
    amarok-sqlcollection
    amaroklib
    amarokcore
    ${KDE4_KDECORE_LIBS}
    ${KDE4_SOLID_LIBS} )

install(TARGETS amarok_device_smb DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install(FILES  amarok_device_smb.desktop  DESTINATION ${SERVICES_INSTALL_DIR})


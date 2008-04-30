
include_directories(
    ../..
    ../../engine
    ../../mediadevice
    ../../meta
    ../../collection
    ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}
    )


########### next target ###############

ADD_DEFINITIONS(-DHAVE_MTP)
set( mtp_LIBS
    mtp
)

set(amarok_mtp-mediadevice_PART_SRCS
    mtpmediadevice.cpp )

kde4_add_plugin(amarok_mtp-mediadevice WITH_PREFIX ${amarok_mtp-mediadevice_PART_SRCS})

target_link_libraries(amarok_mtp-mediadevice  ${KDE4_KDECORE_LIBS} ${mtp_LIBS} amaroklib)

install(TARGETS amarok_mtp-mediadevice DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install(FILES amarok_mtp-mediadevice.desktop DESTINATION ${SERVICES_INSTALL_DIR})


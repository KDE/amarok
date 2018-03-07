install( FILES amarok_homerc DESTINATION ${CONFIG_INSTALL_DIR} )
install( FILES amarok.notifyrc DESTINATION ${KNOTIFYRC_INSTALL_DIR} )

install(FILES DefaultPlaylistLayouts.xml
              first_run_jingle.ogg
              DESTINATION ${DATA_INSTALL_DIR}/amarok/data)


install( FILES amarok_homerc DESTINATION ${KDE_INSTALL_CONFDIR} )
install( FILES amarok.notifyrc DESTINATION ${KDE_INSTALL_KNOTIFYRCDIR} )

install(FILES DefaultPlaylistLayouts.xml
              first_run_jingle.ogg
              DESTINATION ${KDE_INSTALL_DATADIR}/amarok/data)


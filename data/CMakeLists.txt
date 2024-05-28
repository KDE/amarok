install( FILES amarok_homerc DESTINATION ${KDE_INSTALL_CONFDIR} )
if(BUILD_WITH_QT6)
  install( FILES amarok.notifyrc DESTINATION ${KDE_INSTALL_KNOTIFYRCDIR} )
else()
  install( FILES amarok.notifyrc DESTINATION ${KDE_INSTALL_KNOTIFY5RCDIR} )
endif()

install(FILES DefaultPlaylistLayouts.xml
              first_run_jingle.ogg
              DESTINATION ${KDE_INSTALL_DATADIR}/amarok/data)


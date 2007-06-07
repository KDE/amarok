install(FILES  amarok.kcfg DESTINATION ${KCFG_INSTALL_DIR} )
install(FILES org.kde.amarok.collection.xml
              org.kde.amarok.context.xml
	      org.kde.amarok.contextbrowser.xml
              org.kde.amarok.mediabrowser.xml
              org.kde.amarok.player.xml
              org.kde.amarok.playlist.xml
              org.kde.amarok.playlistbrowse.xml
              org.kde.amarok.script.xml
	      DESTINATION ${DBUS_INTERFACES_DIR})

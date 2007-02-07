include_directories( 
	${CMAKE_SOURCE_DIR}/amarok/src 
	${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  
	${CMAKE_BINARY_DIR}/amarok/src  )

############### next target ##########################

#set( libamarokcore_SRCS crashhandler.cpp )

#kde4_automoc( ${libamarokcore_SRCS} )
#kde4_add_kcfg_files( amarokcore ${CMAKE_SOURCE_DIR}/amarok/src/amarokcore/amarokconfig.kcfgc)

#kde4_add_library( amarokcore SHARED ${libamarokcore_SRCS} )


#target_link_libraries(amarokcore ${KDE4_KUTILS_LIBS} ${KDE4_KHTML_LIBS} ${KDE4_KDE3SUPPORT_LIBS})

#set_target_properties(amarokcore PROPERTIES VERSION 1.0.0 SOVERSION 1 )
#install(TARGETS amarokcore DESTINATION ${LIB_INSTALL_DIR} )


#generate D-Bus files
#qt4_add_dbus_adaptor( libamarokcore_SRCS org.kde.Amarok.xml amarokdcophandler.h DcopPlayerHandler PlayerHandler )

install(FILES  amarok.kcfg DESTINATION ${KCFG_INSTALL_DIR} )
install(FILES org.kde.amarok.collection.xml
	      org.kde.amarok.contextbrowser.xml
              org.kde.amarok.mediabrowser.xml
              org.kde.amarok.player.xml
              org.kde.amarok.playlist.xml
              org.kde.amarok.playlistbrowse.xml
              org.kde.amarok.script.xml
	      DESTINATION ${DBUS_INTERFACES_DIR})

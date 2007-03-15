
FIND_PATH( KONQSIDEBAR_INCLUDE_DIR konqsidebarplugin.h
  ${KDE4_INCLUDE_DIR}
  /usr/include
  /usr/local/include
)

if(KONQSIDEBAR_INCLUDE_DIR)
#   add_subdirectory( konquisidebar ) #disabling for now
else(KONQSIDEBAR_INCLUDE_DIR)
    MESSAGE(STATUS "You're missing the kdebase header files")
    MESSAGE(STATUS "Konqueror sidebar will not be compiled.")
endif(KONQSIDEBAR_INCLUDE_DIR)
ADD_DEFINITIONS(${TAGLIB_CFLAGS})

if(MP4V2_FOUND)
ADD_DEFINITIONS(-DHAVE_MP4V2)
include_directories( ${MP4V2_INCLUDE_DIR} )
endif(MP4V2_FOUND)

add_subdirectory( amarokcore )
add_subdirectory( data )
add_subdirectory( images )
add_subdirectory( loader )
add_subdirectory( scripts )
add_subdirectory( themes )
add_subdirectory( vis )
add_subdirectory( metadata )
add_subdirectory( engine )
add_subdirectory( mediadevice )
add_subdirectory( collectionscanner )
#add_subdirectory( device ) #not necessary at the moment, enable again when devicemanager has been ported to Solid


set(libsqlite_la_SRCS )
if(SQLITE_FOUND)
	set(libsqlite_SRCS
			    sqlite/alter.c
			    sqlite/analyze.c
			    sqlite/attach.c
			    sqlite/auth.c
			    sqlite/btree.c
			    sqlite/build.c
			    sqlite/callback.c
			    sqlite/complete.c
			    sqlite/date.c
			    sqlite/delete.c
			    sqlite/expr.c
			    sqlite/func.c
			    sqlite/hash.c
			    sqlite/insert.c
			    sqlite/legacy.c
			    sqlite/main.c
			    sqlite/opcodes.c
			    sqlite/os.c
			    sqlite/os_unix.c
			    sqlite/os_win.c
			    sqlite/pager.c
			    sqlite/parse.c
			    sqlite/pragma.c
			    sqlite/prepare.c
			    sqlite/printf.c
			    sqlite/random.c
			    sqlite/select.c
			    sqlite/table.c
				sqlite/tokenize.c
			    sqlite/trigger.c
			    sqlite/update.c
			    sqlite/utf.c
			    sqlite/util.c
			    sqlite/vacuum.c
				sqlite/vdbe.c
				sqlite/vdbeapi.c
				sqlite/vdbeaux.c
				sqlite/vdbefifo.c
				sqlite/vdbemem.c
				sqlite/where.c
)
endif(SQLITE_FOUND)


include_directories(
	${CMAKE_SOURCE_DIR}/amarok/src/amarokcore
	${CMAKE_SOURCE_DIR}/amarok/src/analyzers
	${CMAKE_SOURCE_DIR}/amarok/src/plugin
	${CMAKE_SOURCE_DIR}/amarok/src/statusbar
	${CMAKE_SOURCE_DIR}/amarok/src/mediadevice
        ${CMAKE_SOURCE_DIR}/amarok/src/servicebrowser
        ${CMAKE_SOURCE_DIR}/amarok/src/servicebrowser/magnatunestore
	${CMAKE_SOURCE_DIR}/amarok/src
	${CMAKE_SOURCE_DIR}/amarok/src/meta
	${CMAKE_SOURCE_DIR}/amarok/src/collection
	${CMAKE_BINARY_DIR}/amarok/src #for amarokconfig.h
	${KDE4_INCLUDE_DIR}
	${QT_INCLUDES}  )


set(libamarokcore_SRCS
	${CMAKE_SOURCE_DIR}/amarok/src/amarokcore/amarokdbushandler.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/amarokcore/crashhandler.cpp
	)
qt4_add_dbus_adaptor( libamarokcore_SRCS amarokcore/org.kde.amarok.collection.xml ${CMAKE_SOURCE_DIR}/amarok/src/amarokcore/amarokdbushandler.h Amarok::DbusCollectionHandler)
qt4_add_dbus_adaptor( libamarokcore_SRCS amarokcore/org.kde.amarok.contextbrowser.xml ${CMAKE_SOURCE_DIR}/amarok/src/amarokcore/amarokdbushandler.h Amarok::DbusContextBrowserHandler)
qt4_add_dbus_adaptor( libamarokcore_SRCS amarokcore/org.kde.amarok.mediabrowser.xml ${CMAKE_SOURCE_DIR}/amarok/src/amarokcore/amarokdbushandler.h Amarok::DbusMediaBrowserHandler)
qt4_add_dbus_adaptor( libamarokcore_SRCS amarokcore/org.kde.amarok.player.xml ${CMAKE_SOURCE_DIR}/amarok/src/amarokcore/amarokdbushandler.h Amarok::DbusPlayerHandler)
qt4_add_dbus_adaptor( libamarokcore_SRCS amarokcore/org.kde.amarok.playlist.xml ${CMAKE_SOURCE_DIR}/amarok/src/amarokcore/amarokdbushandler.h Amarok::DbusPlaylistHandler)
qt4_add_dbus_adaptor( libamarokcore_SRCS amarokcore/org.kde.amarok.playlistbrowse.xml ${CMAKE_SOURCE_DIR}/amarok/src/amarokcore/amarokdbushandler.h Amarok::DbusPlaylistBrowserHandler)
qt4_add_dbus_adaptor( libamarokcore_SRCS amarokcore/org.kde.amarok.script.xml ${CMAKE_SOURCE_DIR}/amarok/src/amarokcore/amarokdbushandler.h Amarok::DbusScriptHandler)



set(libplugin_SRCS
	${CMAKE_SOURCE_DIR}/amarok/src/plugin/plugin.cpp
        ${CMAKE_SOURCE_DIR}/amarok/src/plugin/pluginconfig.cpp
)


set(libservicebrowser_SRCS
        ${CMAKE_SOURCE_DIR}/amarok/src/servicebrowser/servicebrowser.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/servicebrowser/servicebase.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/servicebrowser/servicemodelitembase.cpp
        ${CMAKE_SOURCE_DIR}/amarok/src/servicebrowser/servicemodelbase.cpp)

set(libscriptableservice_SRCS
        ${CMAKE_SOURCE_DIR}/amarok/src/servicebrowser/scriptableservice/scriptableservicemanager.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/servicebrowser/scriptableservice/scriptableservicecontentitem.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/servicebrowser/scriptableservice/scriptableservice.cpp
        ${CMAKE_SOURCE_DIR}/amarok/src/servicebrowser/scriptableservice/scriptableservicecontentmodel.cpp)


qt4_add_dbus_adaptor( libscriptableservice_SRCS servicebrowser/scriptableservice/org.kde.amarok.ScriptableServiceManager.xml ${CMAKE_SOURCE_DIR}/amarok/src/servicebrowser/scriptableservice/scriptableservicemanager.h ScriptableServiceManager)

set(libmagnatunestore_SRCS
	${CMAKE_SOURCE_DIR}/amarok/src/servicebrowser/magnatunestore/magnatuneinfoparser.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/servicebrowser/magnatunestore/magnatunebrowser.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/servicebrowser/magnatunestore/magnatunedownloaddialog.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/servicebrowser/magnatunestore/magnatunepurchasedialog.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/servicebrowser/magnatunestore/magnatunepurchasehandler.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/servicebrowser/magnatunestore/magnatunetypes.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/servicebrowser/magnatunestore/magnatunexmlparser.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/servicebrowser/magnatunestore/magnatunedatabasehandler.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/servicebrowser/magnatunestore/magnatuneredownloaddialog.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/servicebrowser/magnatunestore/magnatuneredownloadhandler.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/servicebrowser/magnatunestore/magnatunedownloadinfo.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/servicebrowser/magnatunestore/magnatunealbumdownloader.cpp
        ${CMAKE_SOURCE_DIR}/amarok/src/servicebrowser/magnatunestore/magnatunecontentitem.cpp
        ${CMAKE_SOURCE_DIR}/amarok/src/servicebrowser/magnatunestore/magnatunecontentmodel.cpp)

kde4_add_ui3_files(libmagnatunestore_SRCS
	${CMAKE_SOURCE_DIR}/amarok/src/servicebrowser/magnatunestore/magnatunedownloaddialogbase.ui
	${CMAKE_SOURCE_DIR}/amarok/src/servicebrowser/magnatunestore/magnatunepurchasedialogbase.ui
	${CMAKE_SOURCE_DIR}/amarok/src/servicebrowser/magnatunestore/magnatuneredownloaddialogbase.ui)

set(libanalyzers_SRCS
     ${CMAKE_SOURCE_DIR}/amarok/src/analyzers/analyzerbase.cpp
     ${CMAKE_SOURCE_DIR}/amarok/src/analyzers/analyzerfactory.cpp
     #${CMAKE_SOURCE_DIR}/amarok/src/analyzers/baranalyzer.cpp
     ${CMAKE_SOURCE_DIR}/amarok/src/analyzers/blockanalyzer.cpp
     ${CMAKE_SOURCE_DIR}/amarok/src/analyzers/glanalyzer.cpp
     ${CMAKE_SOURCE_DIR}/amarok/src/analyzers/glanalyzer2.cpp
     ${CMAKE_SOURCE_DIR}/amarok/src/analyzers/glanalyzer3.cpp
     #${CMAKE_SOURCE_DIR}/amarok/src/analyzers/sonogram.cpp
     #${CMAKE_SOURCE_DIR}/amarok/src/analyzers/turbine.cpp
     #${CMAKE_SOURCE_DIR}/amarok/src/analyzers/boomanalyzer.cpp
 )

set(libstatusbar_SRCS
    ${CMAKE_SOURCE_DIR}/amarok/src/statusbar/statusBarBase.cpp
    ${CMAKE_SOURCE_DIR}/amarok/src/statusbar/statusbar.cpp
    ${CMAKE_SOURCE_DIR}/amarok/src/statusbar/overlayWidget.cpp
    ${CMAKE_SOURCE_DIR}/amarok/src/statusbar/popupMessage.cpp
    ${CMAKE_SOURCE_DIR}/amarok/src/statusbar/progressBar.cpp
    ${CMAKE_SOURCE_DIR}/amarok/src/statusbar/squeezedtextlabel.cpp
    ${CMAKE_SOURCE_DIR}/amarok/src/statusbar/queueLabel.cpp
	)

########### next target ###############

set(amarok_LIB_SRCS ${libamarokcore_SRCS} ${libplugin_SRCS} ${libstatusbar_SRCS}
        ${libsqlite_SRCS} ${libservicebrowser_SRCS} ${libscriptableservice_SRCS}
        ${libmagnatunestore_SRCS} ${libanalyzers_SRCS}
   collection/sqlregistry.cpp
   collection/sqlmeta.cpp
   Options1.ui.h
   actionclasses.cpp
   app.cpp
   atomicstring.cpp
   collectionbrowser.cpp
   collectiondb.cpp
   columnlist.cpp
   configdialog.cpp
   contextbrowser.cpp
   coverfetcher.cpp
   covermanager.cpp
   cuefile.cpp
   dbsetup.ui.h
   deletedialog.cpp
   deviceconfiguredialog.cpp
   devicemanager.cpp
   directorylist.cpp
   dynamicmode.cpp
   editfilterdialog.cpp
   enginebase.cpp
   enginecontroller.cpp
   engineobserver.cpp
   equalizergraph.cpp
   equalizerpresetmanager.cpp
   equalizersetup.cpp
   expression.cpp
   fht.cpp
   filebrowser.cpp
   hintlineedit.cpp
   htmlview.cpp
   iconloader.cpp
   k3bexporter.cpp
   kbookmarkhandler.cpp
   ktrm.cpp
   lastfm.cpp
   mediabrowser.cpp
   mediadevicemanager.cpp
   medium.cpp
   mediumpluginmanager.cpp
   metabundle.cpp
   metabundlesaver.cpp
   moodbar.cpp
   mountpointmanager.cpp
   mydiroperator.cpp
   osd.cpp
   pixmapviewer.cpp
   playlist.cpp
   playlistbrowser.cpp
   playlistbrowseritem.cpp
   playlistitem.cpp
   playlistloader.cpp
   playlistselection.cpp
   playlistwindow.cpp
   pluginmanager.cpp
   podcastsettings.cpp
   prettypopupmenu.cpp
   querybuilder.cpp
   queuemanager.cpp
   refreshimages.cpp
   scancontroller.cpp
   scriptmanager.cpp
   scrobbler.cpp
   sidebar.h
   sidebarwidget.cpp
   sliderwidget.cpp
   smartplaylisteditor.cpp
   socketserver.cpp
   statistics.cpp
   systray.cpp
   tagdialog.cpp
   tagguesser.cpp
   threadmanager.cpp
   tooltip.cpp
   trackpickerdialog.cpp
   tracktooltip.cpp
   transferdialog.cpp
   xmlloader.cpp
   xspfplaylist.cpp
   dockwidget.cpp )

kde4_add_kcfg_files(amarok_LIB_SRCS ${CMAKE_SOURCE_DIR}/amarok/src/amarokcore/amarokconfig.kcfgc)

set(amarok_collectionscanner_xml  ${CMAKE_SOURCE_DIR}/amarok/src/collectionscanner/org.kde.amarok.scanner.xml)

QT4_ADD_DBUS_INTERFACE( amarok_LIB_SRCS ${amarok_collectionscanner_xml} amarok_collection_scanner_interface )



kde4_automoc(${amarok_LIB_SRCS})

kde4_add_ui3_files(amarok_LIB_SRCS
   Options1.ui
   Options2.ui
   Options4.ui
   Options5.ui
   Options7.ui
   Options8.ui
   dbsetup.ui
#   firstrunwizard.ui
   newdynamic.ui
   organizecollectiondialog.ui
   podcastsettingsbase.ui
   scriptmanagerbase.ui
   tagdialogbase.ui
#   tagguesserconfigdialog.ui
   trackpickerdialogbase.ui )

kde4_add_ui_files(amarok_LIB_SRCS
    tagguesserconfigdialog.ui
    deletedialogbase.ui)

kde4_add_library(amarok SHARED ${amarok_LIB_SRCS})

target_link_libraries(amarok
        ${KDE4_KUTILS_LIBS} ${KDE4_KDEUI_LIBS} ${KDE4_KHTML_LIBS} ${KDE4_KNEWSTUFF_LIBS}
        ${TAGLIB_LIBRARIES}
        ${SQLITE_LIBRARIES}
        ${KDE4_KDE3SUPPORT_LIBS}
        amarok_taglib
)

set_target_properties(amarok PROPERTIES VERSION 1.0.0 SOVERSION 1 )
install(TARGETS amarok DESTINATION ${LIB_INSTALL_DIR} )

# uncomment these to stress-test AtomicString
#set(atomicstring_unittest_SRCS atomicstring_unittest.cpp atomicstring.cpp)
#kde4_add_executable(atomicstring_unittest ${atomicstring_unittest_SRCS})
#target_link_libraries(atomicstring_unittest  ${KDE4_KDECORE_LIBS}  ${KDE4_KDE3SUPPORT_LIBS})

########### next target ###############

set(amarokapp_SRCS main.cpp )



kde4_automoc(${amarokapp_SRCS})

kde4_add_executable(amarokapp ${amarokapp_SRCS})

target_link_libraries(amarokapp  ${KDE4_KDECORE_LIBS} amarok )

install(TARGETS amarokapp DESTINATION ${BIN_INSTALL_DIR})


########### install files ###############

install(FILES amarok.desktop DESTINATION ${XDG_APPS_DIR} )
install(FILES amarok.profile.xml DESTINATION ${DATA_INSTALL_DIR}/profiles )
install(FILES amarokrc DESTINATION ${CONFIG_INSTALL_DIR})
install(FILES amarok_plugin.desktop DESTINATION ${SERVICETYPES_INSTALL_DIR})
install(FILES amarokui.rc DESTINATION ${DATA_INSTALL_DIR}/amarok)
install(FILES amarok_append.desktop DESTINATION ${DATA_INSTALL_DIR}/konqueror/servicemenus)

kde4_install_icons( ${ICON_INSTALL_DIR} )


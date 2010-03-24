include_directories(
    ../
    ../../
    ../../collection
    ../../servicebrowser
    ../../statusbar
    ${CMAKE_CURRENT_BINARY_DIR}/../../..
    ${KDE4_INCLUDE_DIR}
    ${STRIGI_INCLUDE_DIR}
    ${QT_INCLUDES} )

add_subdirectory( images )

########### next target ###############

set(amarok_service_magnatune_PART_SRCS
    MagnatuneActions.cpp
    MagnatuneAlbumDownloader.cpp
    MagnatuneCollectionLocation.cpp
    MagnatuneConfig.cpp
    MagnatuneDatabaseHandler.cpp
    MagnatuneDatabaseWorker.cpp
    MagnatuneDownloadDialog.cpp
    MagnatuneDownloadInfo.cpp
    MagnatuneInfoParser.cpp
    MagnatuneMeta.cpp
    MagnatunePurchaseDialog.cpp
    MagnatunePurchaseHandler.cpp
    MagnatuneRedownloadDialog.cpp
    MagnatuneRedownloadHandler.cpp
    MagnatuneSqlCollection.cpp
    MagnatuneStore.cpp
    MagnatuneUrlRunner.cpp
    MagnatuneXmlParser.cpp

)

kde4_add_ui_files( amarok_service_magnatune_PART_SRCS
    MagnatuneDownloadDialogBase.ui
    MagnatunePurchaseDialogBase.ui
    MagnatuneRedownloadDialogBase.ui

)

kde4_add_plugin(amarok_service_magnatunestore ${amarok_service_magnatune_PART_SRCS})
target_link_libraries(amarok_service_magnatunestore
    amaroklib
    amarokpud
    ${KDE4_KDECORE_LIBS}
    ${KDE4_KDEUI_LIBS}
    ${KDE4_KIO_LIBS}
    ${KDE4_THREADWEAVER_LIBRARIES}
)


install(TARGETS amarok_service_magnatunestore DESTINATION ${PLUGIN_INSTALL_DIR} )

########### next target ###############

set(kcm_amarok_service_magnatune_PART_SRCSS
    MagnatuneSettingsModule.cpp
    MagnatuneConfig.cpp
)

kde4_add_ui_files( kcm_amarok_service_magnatune_PART_SRCSS MagnatuneConfigWidget.ui )


kde4_add_plugin( kcm_amarok_service_magnatunestore ${kcm_amarok_service_magnatune_PART_SRCSS} )


target_link_libraries( kcm_amarok_service_magnatunestore  ${KDE4_KUTILS_LIBS} ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${KDE4_KDECORE_LIBRARY} ${KDE4_KDEUI_LIBS} )

install(TARGETS kcm_amarok_service_magnatunestore  DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES amarok_service_magnatunestore.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install( FILES amarok_service_magnatunestore_config.desktop DESTINATION ${SERVICES_INSTALL_DIR})

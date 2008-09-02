        include_directories( 
            ../
            ../../..
            ../../../plugin
            ../../../meta
            ../../../collection
            ../../../servicebrowser
            ../../../statusbar
            ${CMAKE_CURRENT_BINARY_DIR}/../../..
            ${KDE4_INCLUDE_DIR}
            ${STRIGI_INCLUDE_DIR}
            ${QT_INCLUDES} )

########### next target ###############

        set(amarok_service_magnatune_PART_SRCS
            MagnatuneInfoParser.cpp
            MagnatuneStore.cpp
            MagnatuneDownloadDialog.cpp
            MagnatunePurchaseDialog.cpp
            MagnatunePurchaseHandler.cpp
            MagnatuneMeta.cpp
            MagnatuneXmlParser.cpp
            MagnatuneDatabaseHandler.cpp
            MagnatuneRedownloadDialog.cpp
            MagnatuneRedownloadHandler.cpp
            MagnatuneDownloadInfo.cpp
            MagnatuneAlbumDownloader.cpp
            MagnatuneConfig.cpp
            MagnatuneDatabaseWorker.cpp
            MagnatuneSqlCollection.cpp
            MagnatuneCollectionLocation.cpp
        )

        kde4_add_ui_files( amarok_service_magnatune_PART_SRCS
            MagnatuneDownloadDialogBase.ui
            MagnatuneRedownloadDialogBase.ui
            MagnatunePurchaseDialogBase.ui
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

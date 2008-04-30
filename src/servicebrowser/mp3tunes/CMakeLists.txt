        include_directories(
            ../..
            ../../plugin
            ../../meta
            ../../collection
            ../../servicebrowser
            ../../statusbar
            ${CMAKE_CURRENT_BINARY_DIR}/../..
            ${KDE4_INCLUDE_DIR}
            ${STRIGI_INCLUDE_DIR}
            ${QT_INCLUDES} )

########### next target ###############

        set(amarok_service_mp3tunes_PART_SRCS
            Mp3tunesService.cpp
            Mp3tunesServiceCollection.cpp
            Mp3tunesServiceQueryMaker.cpp
            Mp3tunesMeta.cpp
            Mp3tunesConfig.cpp
        )


        kde4_add_plugin(amarok_service_mp3tunes ${amarok_service_mp3tunes_PART_SRCS})
        target_link_libraries(amarok_service_mp3tunes amaroklib ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} ${KDE4_THREADWEAVER_LIBRARIES})


        install(TARGETS amarok_service_mp3tunes DESTINATION ${PLUGIN_INSTALL_DIR} )

########### next target ###############

        set(kcm_amarok_service_mp3tunes_PART_SRCSS
            Mp3tunesSettingsModule.cpp
            Mp3tunesConfig.cpp
        )

        kde4_add_ui_files( kcm_amarok_service_mp3tunes_PART_SRCSS Mp3tunesConfigWidget.ui )


        kde4_add_plugin( kcm_amarok_service_mp3tunes ${kcm_amarok_service_mp3tunes_PART_SRCSS} )


        target_link_libraries( kcm_amarok_service_mp3tunes  ${KDE4_KUTILS_LIBS} )

        install(TARGETS kcm_amarok_service_mp3tunes  DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

        install( FILES amarok_service_mp3tunes.desktop DESTINATION ${SERVICES_INSTALL_DIR})
        install( FILES amarok_service_mp3tunes_config.desktop DESTINATION ${SERVICES_INSTALL_DIR})

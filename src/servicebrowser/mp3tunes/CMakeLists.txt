if(CURL_FOUND)
    if(LIBXML2_FOUND)
        if(OPENSSL_FOUND AND LOUDMOUTH_FOUND AND GLIB2_FOUND AND GOBJECT_FOUND)
            include_directories(
                ../..
                ../../plugin
                ../../meta
                ../../collection
                ../../servicebrowser
                ../../statusbar
                ./libmp3tunes
                ${GLIB2_INCLUDE_DIR}
                ${GOBJECT_INCLUDE_DIR}
                ${LOUDMOUTH_INCLUDE_DIR}
                ${OPENSSL_INCLUDE_DIR}
                ${LIBXML2_INCLUDE_DIR}
                ${CURL_INCLUDE_DIRS}
                ${CMAKE_CURRENT_BINARY_DIR}/../..
                ${KDE4_INCLUDE_DIR}
                ${STRIGI_INCLUDE_DIR}
                ${QT_INCLUDES} )

########### next target ###############

            set(amarok_service_mp3tunes_PART_SRCS
                Mp3tunesService.cpp
                Mp3tunesServiceCollection.cpp
                Mp3tunesServiceCollectionLocation.cpp
                Mp3tunesServiceQueryMaker.cpp
                Mp3tunesMeta.cpp
                Mp3tunesConfig.cpp
                Mp3tunesLockerMeta.cpp
                Mp3tunesLocker.cpp
                Mp3tunesHarmony.cpp
                Mp3tunesWorkers.cpp

                libmp3tunes/locker.c
                libmp3tunes/harmony.c
            )
            

            kde4_add_plugin(amarok_service_mp3tunes ${amarok_service_mp3tunes_PART_SRCS})
#${OPENSSL_LIBRARIES} returns -lssl, not -lcrypto. we only need -lcrypto.
            target_link_libraries(amarok_service_mp3tunes amaroklib crypto
                ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} ${GLIB2_LIBRARIES} ${GOBJECT_LIBRARIES}
                ${KDE4_THREADWEAVER_LIBRARIES} ${OPENSSL_LIBRARIES} ${LIBXML2_LIBRARIES}
                ${CURL_LIBRARIES} ${LOUDMOUTH_LIBRARIES} )

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

        endif(OPENSSL_FOUND AND LOUDMOUTH_FOUND AND GLIB2_FOUND AND GOBJECT_FOUND)
    endif(LIBXML2_FOUND)
endif(CURL_FOUND)


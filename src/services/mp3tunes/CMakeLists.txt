if(OPENSSL_FOUND OR LIBGCRYPT_FOUND)
if(LIBXML2_FOUND AND CURL_FOUND)
    if(LOUDMOUTH_FOUND AND GLIB2_FOUND AND GOBJECT_FOUND AND QT4_GLIB_SUPPORT)
        include_directories(
            ../
            ../../
            ../../core-impl/collections
            ../../statusbar
            ./libmp3tunes
            ./harmonydaemon
            ${LIBXML2_INCLUDE_DIR}
            ${CURL_INCLUDE_DIRS}
            ${CMAKE_CURRENT_BINARY_DIR}/../../..
            
            
        )
    ########### next target ###############

            include_directories(
                ./harmonydaemon
                ${GLIB2_INCLUDE_DIR}
                ${GOBJECT_INCLUDE_DIR}
                ${LOUDMOUTH_INCLUDE_DIRS}
            )
            if(LIBGCRYPT_FOUND)
                add_definitions(-DHAVE_LIBGCRYPT)
            else(LIBGCRYPT_FOUND)
                include_directories(${OPENSSL_INCLUDE_DIR})
                add_definitions(-DHAVE_OPENSSL)
            endif(LIBGCRYPT_FOUND)

            set(amarok_service_mp3tunes_harmony_PART_SRCS
                harmonydaemon/Mp3tunesHarmonyDownload.cpp
                harmonydaemon/Mp3tunesHarmonyDaemon.cpp
                harmonydaemon/Mp3tunesHarmonyClient.cpp
                harmonydaemon/AmarokClient.cpp
                harmonydaemon/main.cpp
                libmp3tunes/md5.c
                libmp3tunes/locker.c
                libmp3tunes/harmony.c
            )
            qt5_add_dbus_adaptor(amarok_service_mp3tunes_harmony_PART_SRCS
                harmonydaemon/org.kde.amarok.Mp3tunesHarmonyDaemon.xml
                harmonydaemon/Mp3tunesHarmonyDaemon.h
                Mp3tunesHarmonyDaemon
            )

            add_executable(amarokmp3tunesharmonydaemon
ecm_mark_nongui_executable(amarokmp3tunesharmonydaemon)
                ${amarok_service_mp3tunes_harmony_PART_SRCS}
            )

            target_link_libraries(amarokmp3tunesharmonydaemon
                amarokcore
                amaroklib
                KF5::KDELibs4Support
                ${GLIB2_LIBRARIES}
                ${GOBJECT_LIBRARIES}
                ${LOUDMOUTH_LIBRARIES}
                ${LIBXML2_LIBRARIES}
                ${CURL_LIBRARIES}
            )

            if(LIBGCRYPT_FOUND)
                target_link_libraries(amarokmp3tunesharmonydaemon ${LIBGCRYPT_LIBS})
            else(LIBGCRYPT_FOUND)
                target_link_libraries(amarokmp3tunesharmonydaemon crypto ${OPENSSL_LIBRARIES})
            endif(LIBGCRYPT_FOUND)

            install(TARGETS amarokmp3tunesharmonydaemon ${INSTALL_TARGETS_DEFAULT_ARGS} )
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
            Mp3tunesWorkers.cpp
            Mp3tunesHarmonyHandler.cpp

            libmp3tunes/locker.c
            libmp3tunes/md5.c
        )

        qt5_add_dbus_adaptor(amarok_service_mp3tunes_PART_SRCS
                org.kde.amarok.Mp3tunesHarmonyHandler.xml
                Mp3tunesHarmonyHandler.h
                Mp3tunesHarmonyHandler
        )

        add_library(amarok_service_mp3tunes MODULE ${amarok_service_mp3tunes_PART_SRCS})
        target_link_libraries(amarok_service_mp3tunes
            amarokcore
            amaroklib
            KF5::KDELibs4Support
            
            KF5::ThreadWeaver
            ${LIBXML2_LIBRARIES}
            ${CURL_LIBRARIES}
            Qt5::Network
        )

          if(LIBGCRYPT_FOUND)
              target_link_libraries(amarok_service_mp3tunes ${LIBGCRYPT_LIBS})
          else(LIBGCRYPT_FOUND)
                  #${OPENSSL_LIBRARIES} returns -lssl, not -lcrypto. we only need -lcrypto.
              target_link_libraries(amarok_service_mp3tunes crypto ${OPENSSL_LIBRARIES})
          endif(LIBGCRYPT_FOUND)


        install(TARGETS amarok_service_mp3tunes DESTINATION ${PLUGIN_INSTALL_DIR} )

    ########### next target ###############

        set(kcm_amarok_service_mp3tunes_PART_SRCSS
            Mp3tunesSettingsModule.cpp
            Mp3tunesConfig.cpp
        )

        ki18n_wrap_ui( kcm_amarok_service_mp3tunes_PART_SRCSS Mp3tunesConfigWidget.ui )


        add_library(kcm_amarok_service_mp3tunes MODULE ${kcm_amarok_service_mp3tunes_PART_SRCSS} )


        target_link_libraries( kcm_amarok_service_mp3tunes  Qt5::Network   )

        install(TARGETS kcm_amarok_service_mp3tunes  DESTINATION ${PLUGIN_INSTALL_DIR})


    ########### install files ###############

        install( FILES amarok_service_mp3tunes.desktop DESTINATION ${SERVICES_INSTALL_DIR})
        install( FILES amarok_service_mp3tunes_config.desktop DESTINATION ${SERVICES_INSTALL_DIR})
    endif(LOUDMOUTH_FOUND AND GLIB2_FOUND AND GOBJECT_FOUND AND QT4_GLIB_SUPPORT)
endif(LIBXML2_FOUND AND CURL_FOUND)
endif(OPENSSL_FOUND OR LIBGCRYPT_FOUND)


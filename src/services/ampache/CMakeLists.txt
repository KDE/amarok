        include_directories(
            ../
            ../../
            ../../plugin
            ../../meta
            ../../collection
            ../../servicebrowser
            ../../statusbar
            ${CMAKE_CURRENT_BINARY_DIR}/../../..
            ${KDE4_INCLUDE_DIR}
            ${STRIGI_INCLUDE_DIR}
            ${QT_INCLUDES}
            )

if( LIBLASTFM_FOUND )
    include_directories(
        ${LIBLASTFM_INCLUDE_DIR}/..
    )
endif( LIBLASTFM_FOUND )

add_subdirectory( images )

########### next target ###############

        set(amarok_service_ampache_PART_SRCS
            AmpacheService.cpp
            AmpacheServiceCollection.cpp
            AmpacheServiceQueryMaker.cpp
            AmpacheMeta.cpp
            AmpacheConfig.cpp
            sha256/sha256.cpp
        )
        if( LIBLASTFM_FOUND )
            set(amarok_service_ampache_PART_SRCS
                ${amarok_service_ampache_PART_SRCS}
                LastfmInfoParser.cpp )
        endif( LIBLASTFM_FOUND )



        kde4_add_plugin(amarok_service_ampache ${amarok_service_ampache_PART_SRCS})
        target_link_libraries(amarok_service_ampache
            amaroklib
            ${KDE4_KDECORE_LIBS}
            ${KDE4_KDEUI_LIBS}
            ${KDE4_KIO_LIBS}
            ${KDE4_THREADWEAVER_LIBRARIES}
            ${LIBLASTFM_LIBRARY}
        )
        if( LIBLASTFM_FOUND )
            target_link_libraries(amarok_service_ampache
                ${LIBLASTFM_LIBRARY}
            )
        endif( LIBLASTFM_FOUND )


        install(TARGETS amarok_service_ampache DESTINATION ${PLUGIN_INSTALL_DIR} )


########### next target ###############

        set(kcm_amarok_service_ampache_PART_SRCSS AmpacheSettings.cpp AmpacheConfig.cpp )

        kde4_add_ui_files( kcm_amarok_service_ampache_PART_SRCSS AmpacheConfigWidget.ui )


        kde4_add_plugin( kcm_amarok_service_ampache ${kcm_amarok_service_ampache_PART_SRCSS} )


        target_link_libraries( kcm_amarok_service_ampache  ${KDE4_KUTILS_LIBS} ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${KDE4_KDECORE_LIBRARY} ${KDE4_KDEUI_LIBS} )

        install(TARGETS kcm_amarok_service_ampache  DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

        install( FILES amarok_service_ampache.desktop DESTINATION ${SERVICES_INSTALL_DIR})
        install( FILES amarok_service_ampache_config.desktop DESTINATION ${SERVICES_INSTALL_DIR})

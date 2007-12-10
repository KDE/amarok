

        include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../..
        ${CMAKE_CURRENT_SOURCE_DIR}/../../plugin
        ${CMAKE_CURRENT_SOURCE_DIR}/../../amarokcore
        ${CMAKE_CURRENT_SOURCE_DIR}/../../meta
        ${CMAKE_CURRENT_SOURCE_DIR}/../../collection
        ${CMAKE_CURRENT_SOURCE_DIR}/../../
        ${CMAKE_CURRENT_SOURCE_DIR}/../../servicebrowser
        ${CMAKE_CURRENT_SOURCE_DIR}/../../statusbar
        ${CMAKE_CURRENT_BINARY_DIR}/../..
        ${KDE4_INCLUDE_DIR}
        ${STRIGI_INCLUDE_DIR}
        ${QT_INCLUDES} )

########### next target ###############

        set(amarok_service-ampache_PART_SRCS
            AmpacheService.cpp
            AmpacheServiceCollection.cpp
            AmpacheServiceQueryMaker.cpp
            AmpacheMeta.cpp
        )



        kde4_add_plugin(amarok_service-ampache WITH_PREFIX ${amarok_service-ampache_PART_SRCS})
        target_link_libraries(amarok_service-ampache amaroklib ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} ${KDE4_THREADWEAVER_LIBRARIES})


        install(TARGETS amarok_service-ampache DESTINATION ${PLUGIN_INSTALL_DIR} )


########### next target ###############

        set(kcm_amarok_service_ampache_PART_SRCSS AmpacheSettings.cpp AmpacheConfig.cpp )

        kde4_add_ui_files( kcm_amarok_service_ampache_PART_SRCSS AmpacheConfigWidget.ui )


        kde4_add_plugin( kcm_amarok_service_ampache ${kcm_amarok_service_ampache_PART_SRCSS} )


        target_link_libraries( kcm_amarok_service_ampache  ${KDE4_KUTILS_LIBS} )

        install(TARGETS kcm_amarok_service_ampache  DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

        install( FILES amarok_service-ampache.desktop DESTINATION ${SERVICES_INSTALL_DIR})
        install( FILES amarok_service-ampache_config.desktop DESTINATION ${SERVICES_INSTALL_DIR})

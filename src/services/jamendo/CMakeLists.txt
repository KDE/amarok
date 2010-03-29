        include_directories(
            ../
            ../../
            ../../core-implementations/collections
            ../../servicebrowser
            ../../statusbar
            ${CMAKE_CURRENT_BINARY_DIR}/../..
            ${KDE4_INCLUDE_DIR}
            ${STRIGI_INCLUDE_DIR}
            ${QT_INCLUDES} )

add_subdirectory( images )

########### next target ###############

        set(amarok_service_jamendo_PART_SRCS
            JamendoService.cpp
            JamendoMeta.cpp
            JamendoDatabaseHandler.cpp
            JamendoXmlParser.cpp
            JamendoInfoParser.cpp
        )

        kde4_add_plugin(amarok_service_jamendo ${amarok_service_jamendo_PART_SRCS})
        target_link_libraries(amarok_service_jamendo
            amaroklib
            amarokpud
            ${KDE4_KDECORE_LIBS}
            ${KDE4_KDEUI_LIBS}
            ${KDE4_KIO_LIBS}
            ${KDE4_THREADWEAVER_LIBRARIES}
        )


        install(TARGETS amarok_service_jamendo DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

        install( FILES amarok_service_jamendo.desktop DESTINATION ${SERVICES_INSTALL_DIR})

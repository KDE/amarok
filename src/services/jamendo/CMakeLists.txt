        include_directories(
            ../
            ../../
            ../../core-impl/collections
            ../../statusbar
            ${CMAKE_CURRENT_BINARY_DIR}/../..
            
             )

add_subdirectory( images )

########### next target ###############

        set(amarok_service_jamendo_PART_SRCS
            JamendoService.cpp
            JamendoMeta.cpp
            JamendoDatabaseHandler.cpp
            JamendoXmlParser.cpp
            JamendoInfoParser.cpp
        )

        add_library(amarok_service_jamendo MODULE ${amarok_service_jamendo_PART_SRCS})
        target_link_libraries(amarok_service_jamendo
            amarokcore
            amaroklib
            amarokpud
            KF5::KIOCore
            KF5::ThreadWeaver
        )


        install(TARGETS amarok_service_jamendo DESTINATION ${PLUGIN_INSTALL_DIR} )

        kcoreaddons_desktop_to_json(amarok_service_jamendo amarok_service_jamendo.desktop SERVICE_TYPES ${CMAKE_SOURCE_DIR}/src/amarok-plugin.desktop)

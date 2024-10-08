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
            KF${KF_MAJOR_VERSION}::KIOCore
            KF${KF_MAJOR_VERSION}::ThreadWeaver
        )


        install(TARGETS amarok_service_jamendo DESTINATION ${KDE_INSTALL_PLUGINDIR} )

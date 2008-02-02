

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

        set(amarok_service_opmldirectory_PART_SRCS
            OpmlDirectoryService.cpp
            OpmlDirectoryMeta.cpp
            OpmlDirectoryDatabaseHandler.cpp
            OpmlDirectoryXmlParser.cpp
            OpmlDirectoryInfoParser.cpp
        )

        kde4_add_plugin(amarok_service_opmldirectory ${amarok_service_opmldirectory_PART_SRCS})
        target_link_libraries(amarok_service_opmldirectory amaroklib ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} ${KDE4_THREADWEAVER_LIBRARIES})


        install(TARGETS amarok_service_opmldirectory DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

        install( FILES amarok_service_opmldirectory.desktop DESTINATION ${SERVICES_INSTALL_DIR})

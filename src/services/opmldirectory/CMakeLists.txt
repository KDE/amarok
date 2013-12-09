        include_directories(
            ../
            ../..
            ../../core-impl/collections
            ../../statusbar
            ${CMAKE_CURRENT_BINARY_DIR}/../../..
            ${KDE4_INCLUDE_DIR}
            ${QT_INCLUDES} )

########### next target ###############

        set(amarok_service_opmldirectory_PART_SRCS
            OpmlDirectoryService.cpp
            OpmlDirectoryMeta.cpp
            OpmlDirectoryInfoParser.cpp
            OpmlDirectoryModel.cpp
            OpmlDirectoryView.cpp
        )

        kde4_add_ui_files( amarok_service_opmldirectory_PART_SRCS AddOpmlWidget.ui )

        kde4_add_plugin(amarok_service_opmldirectory ${amarok_service_opmldirectory_PART_SRCS})
        target_link_libraries(amarok_service_opmldirectory
            amarokcore
            amaroklib
            ${KDE4_KDECORE_LIBS}
            ${KDE4_KDEUI_LIBS}
            ${KDE4_KIO_LIBS}
            ${KDE4_THREADWEAVER_LIBRARIES}
        )

        install(TARGETS amarok_service_opmldirectory DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

        install( FILES amarok_service_opmldirectory.desktop DESTINATION ${SERVICES_INSTALL_DIR})
        install( FILES podcast_directory.opml DESTINATION ${DATA_INSTALL_DIR}/amarok/data)

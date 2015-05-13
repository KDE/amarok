        include_directories(
            ../
            ../..
            ../../core-impl/collections
            ../../statusbar
            ${CMAKE_CURRENT_BINARY_DIR}/../../..
            
             )

########### next target ###############

        set(amarok_service_opmldirectory_PART_SRCS
            OpmlDirectoryService.cpp
            OpmlDirectoryMeta.cpp
            OpmlDirectoryInfoParser.cpp
            OpmlDirectoryModel.cpp
            OpmlDirectoryView.cpp
        )

        ki18n_wrap_ui( amarok_service_opmldirectory_PART_SRCS AddOpmlWidget.ui )

        add_library(amarok_service_opmldirectory MODULE ${amarok_service_opmldirectory_PART_SRCS})
        target_link_libraries(amarok_service_opmldirectory
            amarokcore
            amaroklib
            KF5::KDELibs4Support
            
            KF5::KIOCore
            KF5::ThreadWeaver
        )

        install(TARGETS amarok_service_opmldirectory DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

        install( FILES amarok_service_opmldirectory.desktop DESTINATION ${SERVICES_INSTALL_DIR})
        install( FILES podcast_directory.opml DESTINATION ${DATA_INSTALL_DIR}/amarok/data)

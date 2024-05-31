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
            KF${KF_MAJOR_VERSION}::IconThemes
            KF${KF_MAJOR_VERSION}::KIOCore
            KF${KF_MAJOR_VERSION}::ThreadWeaver
        )

        install(TARGETS amarok_service_opmldirectory DESTINATION ${KDE_INSTALL_PLUGINDIR} )

########### install files ###############

        install( FILES podcast_directory.opml DESTINATION ${KDE_INSTALL_DATADIR}/amarok/data)

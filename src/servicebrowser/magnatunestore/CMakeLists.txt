

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

        set(amarok_service-magnatune_PART_SRCS
        magnatuneinfoparser.cpp
        MagnatuneStore.cpp
        magnatunedownloaddialog.cpp
        magnatunepurchasedialog.cpp
        magnatunepurchasehandler.cpp
        MagnatuneMeta.cpp
        magnatunexmlparser.cpp
        magnatunedatabasehandler.cpp
        magnatuneredownloaddialog.cpp
        magnatuneredownloadhandler.cpp
        magnatunedownloadinfo.cpp
        magnatunealbumdownloader.cpp )

        kde4_add_ui3_files(amarok_service-magnatune_PART_SRCS
            magnatunedownloaddialogbase.ui
            magnatuneredownloaddialogbase.ui
            magnatunepurchasedialogbase.ui
        )
        
        kde4_add_plugin(amarok_service-magnatunestore WITH_PREFIX ${amarok_service-magnatune_PART_SRCS})
        target_link_libraries(amarok_service-magnatunestore amaroklib ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} ${KDE4_THREADWEAVER_LIBRARIES})


        install(TARGETS amarok_service-magnatunestore DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

        install( FILES amarok_service-magnatunestore.desktop DESTINATION ${SERVICES_INSTALL_DIR})

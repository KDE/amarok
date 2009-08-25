include_directories( ../..
        ../../plugin
        ../../meta
        ../../collection
        ../mediadevicecollection
        ../mediadevicecollection/support
        ../mediadevicecollection/handler
        ${CMAKE_CURRENT_BINARY_DIR}/../..
        ${AMAROK_COLLECTION_SUPPORT_DIR}
        ${KDE4_INCLUDE_DIR}
        ${QT_INCLUDES} )


########### next target ###############

        set(amarok_collection-audiocdcollection_PART_SRCS
        AudioCdCollection.cpp
        AudioCdMeta.cpp
        AudioCdCollectionLocation.cpp
        FormatSelectionDialog.cpp
        AudioCdCollectionCapability.cpp
        handler/AudioCdHandler.cpp
        support/AudioCdDeviceInfo.cpp
        support/AudioCdConnectionAssistant.cpp)

        kde4_add_ui_files(amarok_collection-audiocdcollection_PART_SRCS FormatSelectionDialog.ui)


        kde4_add_plugin(amarok_collection-audiocdcollection ${amarok_collection-audiocdcollection_PART_SRCS})

        target_link_libraries( amarok_collection-audiocdcollection amaroklib amarokpud ${KDE4_THREADWEAVER_LIBRARIES} ${KDE4_KDNSSD_LIBS} ${KDE4_KIO_LIBS} ${KDE4_KUTILS_LIBS} ${KDE4_SOLID_LIBRARY} )

        if(APPLE)
        SET_TARGET_PROPERTIES(amarok_collection-audiocdcollection PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
        endif(APPLE)

        install(TARGETS amarok_collection-audiocdcollection DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES amarok_collection-audiocdcollection.desktop DESTINATION ${SERVICES_INSTALL_DIR})

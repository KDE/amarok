
add_subdirectory( lastfm )

include_directories(
        ../
        ../../
        ../../collection
        ../../meta
        ../../statusbar
        ../../widgets
        ../../context
        liblastfm/
        ${CMAKE_CURRENT_BINARY_DIR}/../.. #for amarokconfig.h
        ${KDE4_INCLUDE_DIR}
        ${STRIGI_INCLUDE_DIR}
        ${QT_INCLUDES}
        #${LIBLASTFM_INCLUDE_DIR}/.. # so lastfm/foo/bar.h references work
        #${LIBLASTFM_INCLUDE_DIR}
        )

########### next target ###############

        add_definitions(${KDE4_ENABLE_EXCEPTIONS})

        set(amarok_service_lastfm_PART_SRCS
            LastFmService.cpp
            LastFmServiceCollection.cpp
            LastFmServiceConfig.cpp
            LastFmServiceQueryMaker.cpp
            LastFmSettings.cpp
            ScrobblerAdapter.cpp
            LastFmTreeModel.cpp
            LastFmTreeView.cpp
            AvatarDownloader.cpp

            meta/LastFmMeta.cpp
            meta/LastFmStreamInfoCapability.cpp

            kdenetwork/knetworkreply.cpp
            kdenetwork/knetworkaccessmanager.cpp
            SimilarArtistsAction.cpp
            LoveTrackAction.cpp

        )

        # add_definitions(-DUNICORN_DLLEXPORT_PRO -DMOOSE_DLLEXPORT_PRO -DQT_XML_LIB)

        kde4_add_plugin(amarok_service_lastfm ${amarok_service_lastfm_PART_SRCS})
        target_link_libraries(amarok_service_lastfm
            amaroklib
            amarokpud
            amarok_service_liblastfm
            ${KDE4_KDECORE_LIBS}
            ${KDE4_KDEUI_LIBS}
            ${KDE4_KIO_LIBS}
            ${KDE4_SOLID_LIBRARY}
            ${KDE4_THREADWEAVER_LIBRARIES}
            ${QT_QTNETWORK_LIBRARY}
        )
        # TODO not sure if this is still relevant...
#         if (APPLE)
#           target_link_libraries(amarok_service_lastfm System/Library/Frameworks/SystemConfiguration.framework)
#         endif (APPLE)

        install(TARGETS amarok_service_lastfm DESTINATION ${PLUGIN_INSTALL_DIR} )

########### next target ###############

        set(kcm_amarok_service_lastfm_PART_SRCS
            LastFmServiceSettings.cpp
            LastFmServiceConfig.cpp
			# sad that these are necessary :-/
			kdenetwork/knetworkreply.cpp
		    kdenetwork/knetworkaccessmanager.cpp
        )

        kde4_add_ui_files( kcm_amarok_service_lastfm_PART_SRCS LastFmConfigWidget.ui )


        kde4_add_plugin( kcm_amarok_service_lastfm ${kcm_amarok_service_lastfm_PART_SRCS} )

        target_link_libraries( kcm_amarok_service_lastfm
            amaroklib
            amarok_service_liblastfm
            ${KDE4_KDEUI_LIBS}
            ${KDE4_KUTILS_LIBS}
            ${KDE4_KIO_LIBS}
		    ${QT_QTNETWORK_LIBRARY} )

        install(TARGETS kcm_amarok_service_lastfm  DESTINATION ${PLUGIN_INSTALL_DIR})

########### install files ###############

        install( FILES amarok_service_lastfm.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
        install( FILES amarok_service_lastfm_config.desktop DESTINATION ${SERVICES_INSTALL_DIR} )

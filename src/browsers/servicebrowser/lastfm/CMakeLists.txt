        include_directories(
        ..
        ../..
        ../../..
        ../../../collection
        ../../../meta
        libUnicorn
        ${CMAKE_CURRENT_BINARY_DIR}/../../.. #for amarokconfig.h
        ${KDE4_INCLUDE_DIR}
        ${STRIGI_INCLUDE_DIR}
        ${QT_INCLUDES} )

########### next target ###############

        if (WIN32)
            set(amarok_service_lastfm_extra_sources
                libUnicorn/UnicornCommonWin.cpp)

            add_definitions(-DUNICODE -D_UNICODE)
        endif (WIN32)

        if (APPLE)
            set(amarok_service_lastfm_extra_sources
                libUnicorn/UnicornCommonMac.cpp)
        endif (APPLE)

        add_definitions(${KDE4_ENABLE_EXCEPTIONS})

        set(amarok_service_lastfm_PART_SRCS
            AudioController.cpp
            LastFmService.cpp
            LastFmServiceCollection.cpp
            LastFmServiceConfig.cpp
            LastFmServiceQueryMaker.cpp
            LastFmSettings.cpp
            RadioAdapter.cpp
            ScrobblerAdapter.cpp

            meta/LastFmMeta.cpp
            meta/LastFmStreamInfoCapability.cpp

            core/Radio.cpp
            core/RadioPlaylist.cpp
            core/Scrobbler-12.cpp
            core/XspfResolver.cpp

            libUnicorn/CachedHttp.cpp
            libUnicorn/CachedHttpJanitor.cpp
            libUnicorn/draglabel.cpp
            libUnicorn/DragMimeData.cpp
            libUnicorn/imagebutton.cpp
            libUnicorn/LastMessageBox.cpp
            libUnicorn/logger.cpp
            libUnicorn/metadata.cpp
            libUnicorn/RedirectHttp.cpp
            libUnicorn/StationUrl.cpp
            libUnicorn/StopWatch.cpp
            libUnicorn/TrackInfo.cpp
            libUnicorn/UnicornCommon.cpp
            libUnicorn/URLLabel.cpp
            libUnicorn/watermarkwidget.cpp
            libUnicorn/WebService/ActionRequest.cpp
            libUnicorn/WebService/AlbumTagsRequest.cpp
            libUnicorn/WebService/ArtistMetaDataRequest.cpp
            libUnicorn/WebService/ArtistTagsRequest.cpp
            libUnicorn/WebService/ChangeStationRequest.cpp
            libUnicorn/WebService/DeleteFriendRequest.cpp
            libUnicorn/WebService/FingerprintQueryRequest.cpp
            libUnicorn/WebService/FriendsRequest.cpp
            libUnicorn/WebService/FrikkinNormanRequest.cpp
            libUnicorn/WebService/GetXspfPlaylistRequest.cpp
            libUnicorn/WebService/Handshake.cpp
            libUnicorn/WebService/NeighboursRequest.cpp
            libUnicorn/WebService/ProxyTestRequest.cpp
            libUnicorn/WebService/RecentTrackRequest.cpp
            libUnicorn/WebService/RecommendRequest.cpp
            libUnicorn/WebService/ReportRebufferingRequest.cpp
            libUnicorn/WebService/Request.cpp
            libUnicorn/WebService/SearchTagsRequest.cpp
            libUnicorn/WebService/SetTagRequest.cpp
            libUnicorn/WebService/SimilarArtistsRequest.cpp
            libUnicorn/WebService/SimilarTagsRequest.cpp
            libUnicorn/WebService/SubmitFullFingerprintRequest.cpp
            libUnicorn/WebService/TopTagsRequest.cpp
            libUnicorn/WebService/TrackMetaDataRequest.cpp
            libUnicorn/WebService/TrackTagsRequest.cpp
            libUnicorn/WebService/TrackToIdRequest.cpp
            libUnicorn/WebService/TrackUploadRequest.cpp
            libUnicorn/WebService/UserLabelsRequest.cpp
            libUnicorn/WebService/UserPicturesRequest.cpp
            libUnicorn/WebService/UserTagsRequest.cpp
            libUnicorn/WebService/VerifyUserRequest.cpp
            libUnicorn/WebService/XmlRpc.cpp
            libUnicorn/WebService.cpp

            ${amarok_service_lastfm_extra_sources}
        )

        add_definitions(-DUNICORN_DLLEXPORT_PRO -DMOOSE_DLLEXPORT_PRO -DQT_XML_LIB)

        kde4_add_plugin(amarok_service_lastfm ${amarok_service_lastfm_PART_SRCS})
        target_link_libraries(amarok_service_lastfm
            amaroklib
            amarokpud
            ${KDE4_KDECORE_LIBS}
            ${KDE4_KDEUI_LIBS}
            ${KDE4_KIO_LIBS}
            ${KDE4_THREADWEAVER_LIBRARIES}
        )
        if (APPLE)
          target_link_libraries(amarok_service_lastfm /System/Library/Frameworks/SystemConfiguration.framework)
        endif (APPLE)

        install(TARGETS amarok_service_lastfm DESTINATION ${PLUGIN_INSTALL_DIR} )

########### next target ###############

        set(kcm_amarok_service_lastfm_PART_SRCS
            LastFmServiceSettings.cpp
            LastFmServiceConfig.cpp
        )

        kde4_add_ui_files( kcm_amarok_service_lastfm_PART_SRCS LastFmConfigWidget.ui )


        kde4_add_plugin( kcm_amarok_service_lastfm ${kcm_amarok_service_lastfm_PART_SRCS} )

        target_link_libraries( kcm_amarok_service_lastfm amaroklib ${KDE4_KDEUI_LIBS} ${KDE4_KUTILS_LIBS} )

        install(TARGETS kcm_amarok_service_lastfm  DESTINATION ${PLUGIN_INSTALL_DIR})

########### install files ###############

        install( FILES amarok_service_lastfm.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
        install( FILES amarok_service_lastfm_config.desktop DESTINATION ${SERVICES_INSTALL_DIR} )

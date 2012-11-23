macro_optional_find_package(NepomukCore)
macro_log_feature(NepomukCore_FOUND "Nepomuk" "Nepomuk Libraries required by Nepomuk Collection" "http://nepomuk.kde.org" FALSE "" "")
macro_optional_find_package(Soprano)
macro_log_feature(Soprano_FOUND "Soprano" "Soprano libraries required by Nepomuk Collection" "http://soprano.sourceforge.net/" FALSE "" "")

if(NepomukCore_FOUND AND Soprano_FOUND)
    include(NepomukAddOntologyClasses)
    
    include_directories( ../..
    	${SOPRANO_INCLUDE_DIR}
	${NEPOMUK_CORE_INCLUDE_DIR}
    )

    ########### next target ###############

    set(amarok_collection-nepomukcollection_PART_SRCS

        NepomukCollection.cpp
        NepomukCollectionFactory.cpp
        NepomukConstructMetaJob.cpp

        meta/NepomukAlbum.cpp
        meta/NepomukArtist.cpp
        meta/NepomukComposer.cpp
        meta/NepomukGenre.cpp
        meta/NepomukTrack.cpp
        meta/NepomukLabel.cpp
        meta/NepomukYear.cpp
    )

    kde4_add_plugin(amarok_collection-nepomukcollection ${amarok_collection-nepomukcollection_PART_SRCS})

    target_link_libraries(amarok_collection-nepomukcollection
        amarokcore
        amaroklib
        ${KDE4_THREADWEAVER_LIBRARIES}
        ${NEPOMUK_CORE_LIBRARY}
        ${KDE4_KDEUI_LIBS}
        ${KDE4_KDECORE_LIBS}
	${SOPRANO_LIBRARIES}
    )

    install(TARGETS amarok_collection-nepomukcollection DESTINATION ${PLUGIN_INSTALL_DIR})

    ########### install files ###############

    install(FILES amarok_collection-nepomukcollection.desktop DESTINATION ${SERVICES_INSTALL_DIR})

endif()

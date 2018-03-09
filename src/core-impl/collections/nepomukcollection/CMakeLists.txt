find_package(NepomukCore QUIET)
set_package_properties(NEPOMUK-CORE PROPERTIES DESCRIPTION "NepomukCore Libraries required by Nepomuk Collection" URL "http://kde.org/download/#v4.9"  TYPE OPTIONAL)
find_package(Soprano QUIET)
set_package_properties(SOPRANO PROPERTIES DESCRIPTION "Soprano libraries required by Nepomuk Collection" URL "http://soprano.sourceforge.net/" TYPE OPTIONAL)

if(NepomukCore_FOUND AND Soprano_FOUND)
    include(NepomukAddOntologyClasses)

    include_directories( ../..
        ${SOPRANO_INCLUDE_DIR}
        ${NEPOMUK_CORE_INCLUDE_DIR}
    )

    ########### next target ###############

    set(amarok_collection-nepomukcollection_PART_SRCS

        NepomukCache.cpp
        NepomukCollection.cpp
        NepomukCollectionFactory.cpp
        NepomukInquirer.cpp
        NepomukParser.cpp
        NepomukQueryMaker.cpp

        meta/NepomukAlbum.cpp
        meta/NepomukArtist.cpp
        meta/NepomukComposer.cpp
        meta/NepomukGenre.cpp
        meta/NepomukTrack.cpp
        meta/NepomukLabel.cpp
        meta/NepomukYear.cpp
    )

    add_library(amarok_collection-nepomukcollection MODULE ${amarok_collection-nepomukcollection_PART_SRCS})

    target_link_libraries(amarok_collection-nepomukcollection
        amarokcore
        amaroklib
        KF5::ThreadWeaver
        ${NEPOMUK_CORE_LIBRARY}
        ${SOPRANO_LIBRARIES}
    )

    install(TARGETS amarok_collection-nepomukcollection DESTINATION ${KDE_INSTALL_PLUGINDIR})

    ########### install files ###############

    install(FILES amarok_collection-nepomukcollection.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

endif()

find_package( Nepomuk REQUIRED )

include(NepomukAddOntologyClasses)
include(SopranoAddOntology)

include_directories( ../..
    ${NEPOMUK_INCLUDES}
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
    meta/NepomukYear.cpp
)

kde4_add_plugin(amarok_collection-nepomukcollection ${amarok_collection-nepomukcollection_PART_SRCS})

target_link_libraries(amarok_collection-nepomukcollection
    amarokcore
    amaroklib
    ${KDE4_THREADWEAVER_LIBRARIES}
    ${NEPOMUK_LIBRARIES}
    ${KDE4_KDEUI_LIBS}
    ${KDE4_KDECORE_LIBS}
    ${NEPOMUK_QUERY_LIBRARIES}
)

install(TARGETS amarok_collection-nepomukcollection DESTINATION ${PLUGIN_INSTALL_DIR} )

########### install files ###############

install( FILES amarok_collection-nepomukcollection.desktop DESTINATION ${SERVICES_INSTALL_DIR})

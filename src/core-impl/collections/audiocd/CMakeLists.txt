include_directories( ../..
    ../../collections
    ${CMAKE_BINARY_DIR}/src
    ${AMAROK_COLLECTION_SUPPORT_DIR}
    ${KDE4_INCLUDE_DIR}
    ${QT_INCLUDES}
)

find_package(CdIO REQUIRED)
find_package(CdIOCDDA REQUIRED)
find_package(Paranoia REQUIRED)
find_package(CDDB REQUIRED)
find_package(ICU REQUIRED)

########### next target ###############

set(amarok_collection-audiocdcollection_PART_SRCS
    AudioCdCollection.cpp
    AudioCdCollectionFactory.cpp
    AudioCdCollectionLocation.cpp
    EncodingSelectionDialog.cpp
    meta/AudioCdTrack.cpp
    meta/AudioCdAlbum.cpp
    helpers/CDDBHelper.cpp
    helpers/CDTEXTHelper.cpp
    helpers/ICUHelper.cpp
    helpers/ParanoiaHelper.cpp
    helpers/WriteWavHelper.cpp
    helpers/MetaDataHelper.cpp
)

kde4_add_plugin(amarok_collection-audiocdcollection ${amarok_collection-audiocdcollection_PART_SRCS})

target_link_libraries(
    amarok_collection-audiocdcollection
    amarokcore
    amaroklib
    ${KDE4_THREADWEAVER_LIBRARIES}
    ${KDE4_KIO_LIBS}
    ${KDE4_KUTILS_LIBS}
    ${KDE4_SOLID_LIBRARY}
    ${CDIO_LIBRARIES}
    ${CDIOCDDA_LIBRARIES}
    ${CDDB_LIBRARIES}
    ${PARANOIA_LIBRARIES}
    ${ICU_I18N_LIBRARIES}
    ${ICU_LIBRARIES}
)
if(APPLE)
    SET_TARGET_PROPERTIES(amarok_collection-audiocdcollection PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)

install(TARGETS amarok_collection-audiocdcollection DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES amarok_collection-audiocdcollection.desktop DESTINATION ${SERVICES_INSTALL_DIR})

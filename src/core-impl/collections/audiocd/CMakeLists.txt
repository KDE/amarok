include_directories( ../..
    ../../collections
    ../mediadevicecollection
    ../mediadevicecollection/support
    ../mediadevicecollection/handler
    ../mediadevicecollection/podcast
    ${CMAKE_BINARY_DIR}/src
    ${AMAROK_COLLECTION_SUPPORT_DIR}
    
    
)


########### next target ###############

set(amarok_collection-audiocdcollection_PART_SRCS
    AudioCdCollection.cpp
    AudioCdMeta.cpp
    AudioCdCollectionLocation.cpp
    FormatSelectionDialog.cpp
    handler/AudioCdHandler.cpp
    support/AudioCdDeviceInfo.cpp
    support/AudioCdConnectionAssistant.cpp
)

ki18n_wrap_ui(amarok_collection-audiocdcollection_PART_SRCS FormatSelectionDialog.ui)

add_library(amarok_collection-audiocdcollection MODULE ${amarok_collection-audiocdcollection_PART_SRCS})

target_link_libraries(
    amarok_collection-audiocdcollection
    amarokcore
    amaroklib
    KF5::ThreadWeaver
    KF5::KIOCore
    
    ${KDE4_SOLID_LIBRARY}
)

if(APPLE)
    SET_TARGET_PROPERTIES(amarok_collection-audiocdcollection PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)

install(TARGETS amarok_collection-audiocdcollection DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES amarok_collection-audiocdcollection.desktop DESTINATION ${SERVICES_INSTALL_DIR})

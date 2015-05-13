find_package(Mtp)


if (MTP_FOUND)
   include_directories(
       ${Amarok_SOURCE_DIR}/src
       ${Amarok_SOURCE_DIR}/src/core-impl/collections
       ${Amarok_SOURCE_DIR}/src/core-impl/collections/mediadevicecollection
       ${Amarok_SOURCE_DIR}/src/core-impl/collections/mediadevicecollection/support
       ${Amarok_SOURCE_DIR}/src/core-impl/collections/mediadevicecollection/handler
       ${Amarok_SOURCE_DIR}/src/core-impl/collections/mediadevicecollection/handler/capabilities
       ${Amarok_SOURCE_DIR}/src/core-impl/collections/mtpcollection/handler
       ${Amarok_SOURCE_DIR}/src/core-impl/collections/mtpcollection/handler/capabilities
       ${Amarok_SOURCE_DIR}/src/core-impl/collections/mtpcollection/support
       ${GLIB2_INCLUDE_DIR}
       ${MTP_INCLUDE_DIR}
       ${CMAKE_BINARY_DIR}/src
       ${AMAROK_COLLECTION_SUPPORT_DIR}
       
       
)


########### next target ###############

set(amarok_collection-mtpcollection_PART_SRCS
    MtpCollection.cpp
    handler/MtpHandler.cpp
    handler/capabilities/MtpPlaylistCapability.cpp
    handler/capabilities/MtpReadCapability.cpp
    handler/capabilities/MtpWriteCapability.cpp
    support/MtpDeviceInfo.cpp
    support/MtpConnectionAssistant.cpp
)
add_library(amarok_collection-mtpcollection MODULE ${amarok_collection-mtpcollection_PART_SRCS})

target_link_libraries(amarok_collection-mtpcollection
    amarokcore
    amaroklib
    KF5::KDELibs4Support
    KF5::KIOCore
    
    ${KDE4_SOLID_LIBRARY}
    KF5::ThreadWeaver
    Qt5::Gui
    ${MTP_LIBRARIES}
)

install(TARGETS amarok_collection-mtpcollection DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES amarok_collection-mtpcollection.desktop DESTINATION ${SERVICES_INSTALL_DIR})

endif (MTP_FOUND)

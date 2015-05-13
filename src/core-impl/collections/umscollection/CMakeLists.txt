include_directories( ../..
  ../../collections
  podcasts
  ${CMAKE_BINARY_DIR}/src
  ${AMAROK_COLLECTION_SUPPORT_DIR}
  
  
  )

########### next target ################

set(amarok_collection-umscollection_PART_SRCS
  UmsCollection.cpp
  UmsCollectionLocation.cpp
  UmsTranscodeCapability.cpp
  podcasts/UmsPodcastProvider.cpp
  podcasts/UmsPodcastMeta.cpp)

ki18n_wrap_ui(amarok_collection-umscollection_PART_SRCS
    UmsConfiguration.ui
)

add_library(amarok_collection-umscollection MODULE ${amarok_collection-umscollection_PART_SRCS})
	
target_link_libraries(amarok_collection-umscollection
  amarokshared
  amaroklib
  amarokcore
  amarok-transcoding
  KF5::KDELibs4Support
  KF5::KIOCore
  
  ${KDE4_SOLID_LIBRARY}
  KF5::ThreadWeaver
  Qt5::Gui
  )

install(TARGETS 
  amarok_collection-umscollection 
  DESTINATION 
  ${PLUGIN_INSTALL_DIR} )

########### install files ###############

install(FILES 
  amarok_collection-umscollection.desktop 
  DESTINATION 
  ${SERVICES_INSTALL_DIR})


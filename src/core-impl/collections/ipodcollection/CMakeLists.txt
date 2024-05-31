if(NOT GDKPIXBUF_FOUND)
  set(GDKPIXBUF_INCLUDE_DIR "")
  set(GDKPIXBUF_LIBRARY "")
endif()

if(IPOD_FOUND AND WITH_IPOD)

  include_directories(
    ${CMAKE_BINARY_DIR}/src
  )
    
    

  include_directories(SYSTEM
    ${GLIB2_INCLUDE_DIR}
    ${GDKPIXBUF_INCLUDE_DIR}
    ${GOBJECT_INCLUDE_DIR}
    ${IPOD_INCLUDE_DIRS}
  )

  ########### set macros for the ipod collection plugin ##########
  # Generate config-ipodcollection.h
  configure_file(config-ipodcollection.h.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/config-ipodcollection.h
  )

  ########### next target ################
  set(amarok_collection-ipodcollection_PART_SRCS
    IpodCollection.cpp
    IpodCollectionFactory.cpp
    IpodCollectionLocation.cpp
    IpodMeta.cpp
    IpodPlaylist.cpp
    IpodPlaylistProvider.cpp
    jobs/IpodCopyTracksJob.cpp
    jobs/IpodDeleteTracksJob.cpp
    jobs/IpodParseTracksJob.cpp
    jobs/IpodWriteDatabaseJob.cpp
    support/IphoneMountPoint.cpp
    support/IpodDeviceHelper.cpp
    support/IpodTranscodeCapability.cpp
  )

  ki18n_wrap_ui(amarok_collection-ipodcollection_PART_SRCS
    support/IpodConfiguration.ui
  )

  link_directories(${IPOD_LIBRARY_DIRS})

  add_library(amarok_collection-ipodcollection MODULE ${amarok_collection-ipodcollection_PART_SRCS})

  target_link_libraries(amarok_collection-ipodcollection
    amarokshared
    amarokcore
    amaroklib
    amarok-transcoding
    KF${KF_MAJOR_VERSION}::KIOCore
    KF${KF_MAJOR_VERSION}::ThreadWeaver
    Qt::Gui
    ${GLIB2_LIBRARIES}
    ${GDKPIXBUF_LIBRARY}
    ${GOBJECT_LIBRARIES}
    ${IPOD_LIBRARIES}
  )

  install(TARGETS
    amarok_collection-ipodcollection
    DESTINATION
    ${KDE_INSTALL_PLUGINDIR}
  )

endif()

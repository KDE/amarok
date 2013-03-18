if(NOT GDKPIXBUF_FOUND)
  set(GDKPIXBUF_INCLUDE_DIR "")
  set(GDKPIXBUF_LIBRARY "")
endif(NOT GDKPIXBUF_FOUND)

if(IPOD_FOUND AND WITH_IPOD)

  include_directories(
    ${CMAKE_BINARY_DIR}/src

    ${KDE4_INCLUDE_DIR}
    ${QT_INCLUDES}

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
    IpodMetaEditCapability.cpp
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

  kde4_add_ui_files(amarok_collection-ipodcollection_PART_SRCS
    support/IpodConfiguration.ui
  )

  link_directories(${IPOD_LIBRARY_DIRS})

  kde4_add_plugin(amarok_collection-ipodcollection ${amarok_collection-ipodcollection_PART_SRCS})

  target_link_libraries(amarok_collection-ipodcollection
    amarokshared
    amarokcore
    amaroklib
    amarok-transcoding
    ${KDE4_KDECORE_LIBS}
    ${KDE4_KIO_LIBS}
    ${KDE4_SOLID_LIBRARY}
    ${KDE4_THREADWEAVER_LIBRARIES}
    ${QT_QTGUI_LIBRARY}
    ${GLIB2_LIBRARIES}
    ${GDKPIXBUF_LIBRARY}
    ${GOBJECT_LIBRARIES}
    ${IPOD_LIBRARIES}
  )

  install(TARGETS
    amarok_collection-ipodcollection
    DESTINATION
    ${PLUGIN_INSTALL_DIR}
  )

  ########### install files ###############

  install(FILES
    amarok_collection-ipodcollection.desktop
    DESTINATION
    ${SERVICES_INSTALL_DIR}
  )

endif( IPOD_FOUND AND WITH_IPOD )

include_directories(
  ${AMAROK_SOURCE_TREE}/context/engines/upcomingevents
  ${AMAROK_SOURCE_TREE}/context/applets/upcomingevents
  ${AMAROK_SOURCE_TREE}/context/engines/similarartists
  ${AMAROK_SOURCE_TREE}/context/applets/similarartists
  ${AMAROK_COLLECTION_SUPPORT_DIR}


)

if(LIBLASTFM_FOUND)
#    add_subdirectory( similarartists )
endif()

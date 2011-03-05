include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )

add_subdirectory( massstorage )
if(${KDE_VERSION} VERSION_GREATER "4.4.99")
   add_subdirectory( nfs )
   add_subdirectory( smb )
endif()

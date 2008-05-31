include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )

set(AMAROK_COLLECTION_SUPPORT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/support)

# daap is currently causing obscure crashes, has many sqlite3 errors, etc.
# add_subdirectory( daap )
# NYI
#add_subdirectory(mediadevicecollection)
add_subdirectory( sqlcollection )

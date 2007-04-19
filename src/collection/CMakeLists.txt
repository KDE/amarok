include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )

set(AMAROK_COLLECTION_SUPPORT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/support)

add_subdirectory( daap )
add_subdirectory( sqlcollection )

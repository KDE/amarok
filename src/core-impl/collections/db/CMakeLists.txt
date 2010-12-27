message(WARNING "Workaround: Adding define for MAKE_AMAROK_DATABASECOLLECTION_LIB, this is needed for Windows, but has to be removed as soon as DatabaseCollection gets its own lib")
add_definitions(-DMAKE_AMAROK_DATABASECOLLECTION_LIB)
add_subdirectory( sql )

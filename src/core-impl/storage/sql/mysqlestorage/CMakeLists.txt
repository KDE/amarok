add_definitions(${MYSQL_EMBEDDED_CFLAGS})

########### mysqle ###############

set( amarok_storage-mysqlestorage_PART_SRCS
     ../mysql-shared/MySqlStorage.cpp
     MySqlEmbeddedStorage.cpp
     MySqlEmbeddedStorageFactory.cpp
)

add_library(amarok_storage-mysqlestorage MODULE ${amarok_storage-mysqlestorage_PART_SRCS})

string(REPLACE "-Wl,--fatal-warnings" "" CMAKE_SHARED_LINKER_FLAGS_NOFATALWARN "${CMAKE_SHARED_LINKER_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS_NOFATALWARN} )

string(REPLACE "-Wl,--fatal-warnings" "" CMAKE_MODULE_LINKER_FLAGS_NOFATALWARN "${CMAKE_MODULE_LINKER_FLAGS}")
set(CMAKE_MODULE_LINKER_FLAGS ${CMAKE_MODULE_LINKER_FLAGS_NOFATALWARN} )

target_link_libraries(amarok_storage-mysqlestorage
    # amarok-sqlstorage
    amarokcore
    amaroklib
    KF5::KDELibs4Support
    Qt5::Core
    Qt5::Gui
    ${MYSQL_EMBEDDED_LIBRARIES}
    ${CMAKE_DL_LIBS}
    ${ZLIB_LIBRARIES}
)

if(NOT WIN32 AND NOT APPLE)
    target_link_libraries( amarok_storage-mysqlestorage crypt pthread )
endif()

if(APPLE)
	SET_TARGET_PROPERTIES(amarok_storage-mysqlestorage PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif()

install(TARGETS amarok_storage-mysqlestorage DESTINATION ${PLUGIN_INSTALL_DIR} )

install(FILES amarok_storage-mysqlestorage.desktop DESTINATION ${SERVICES_INSTALL_DIR})

kcoreaddons_desktop_to_json(amarok_storage-mysqlestorage amarok_storage-mysqlestorage.desktop SERVICE_TYPES ${CMAKE_SOURCE_DIR}/src/amarok-plugin.desktop)

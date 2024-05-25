########### mysqle ###############

include_directories(SYSTEM ${MYSQL_INCLUDE_DIR})

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
    Qt::Core
    Qt::Gui
    ${MYSQLE_LIBRARIES}
    ${CMAKE_DL_LIBS}
    ${ZLIB_LIBRARIES}
)

if(${CMAKE_HOST_SYSTEM_NAME} MATCHES "FreeBSD")
        target_link_libraries(amarok_storage-mysqlestorage ${MYSQL_LIBRARIES})
endif()

if(NOT WIN32 AND NOT APPLE)
    target_link_libraries( amarok_storage-mysqlestorage crypt pthread )
endif()

if(APPLE)
	SET_TARGET_PROPERTIES(amarok_storage-mysqlestorage PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif()

install(TARGETS amarok_storage-mysqlestorage DESTINATION ${KDE_INSTALL_PLUGINDIR} )

install(FILES amarok_storage-mysqlestorage.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

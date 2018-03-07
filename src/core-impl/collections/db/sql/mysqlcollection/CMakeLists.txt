
########### mysql ###############

set( amarok_collection-mysqlcollection_PART_SRCS
    MySqlCollectionFactory.cpp
    MySqlQueryMaker.cpp
)

add_library(amarok_collection-mysqlcollection MODULE ${amarok_collection-mysqlcollection_PART_SRCS})

string(REPLACE "-Wl,--fatal-warnings" "" CMAKE_SHARED_LINKER_FLAGS_NOFATALWARN "${CMAKE_SHARED_LINKER_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS_NOFATALWARN} )

string(REPLACE "-Wl,--fatal-warnings" "" CMAKE_MODULE_LINKER_FLAGS_NOFATALWARN "${CMAKE_MODULE_LINKER_FLAGS}")
set(CMAKE_MODULE_LINKER_FLAGS ${CMAKE_MODULE_LINKER_FLAGS_NOFATALWARN} )

target_link_libraries(amarok_collection-mysqlcollection
    amarok-sqlcollection
    amarokcore
    amaroklib
    Qt5::Core
    Qt5::Gui
    ${CMAKE_DL_LIBS}
    ${ZLIB_LIBRARIES}
)

if(WITH_MYSQL_EMBEDDED)
    target_link_libraries( amarok_collection-mysqlcollection ${MYSQL_EMBEDDED_LIBRARIES} )
endif(WITH_MYSQL_EMBEDDED)

if(NOT WIN32 AND NOT APPLE)
    target_link_libraries( amarok_collection-mysqlcollection crypt pthread )
endif()

if(APPLE)
	SET_TARGET_PROPERTIES(amarok_collection-mysqlcollection PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif()

install(TARGETS amarok_collection-mysqlcollection DESTINATION ${PLUGIN_INSTALL_DIR} )

install(FILES amarok_collection-mysqlcollection.desktop DESTINATION ${SERVICES_INSTALL_DIR})

kcoreaddons_desktop_to_json(amarok_collection-mysqlcollection amarok_collection-mysqlcollection.desktop SERVICE_TYPES ${CMAKE_SOURCE_DIR}/src/amarok-plugin.desktop)

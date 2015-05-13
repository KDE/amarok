
########### mysql ###############

set( amarok_collection-mysqlcollection_PART_SRCS
    MySqlCollectionFactory.cpp
    MySqlQueryMaker.cpp
)

add_library(amarok_collection-mysqlcollection MODULE ${amarok_collection-mysqlcollection_PART_SRCS})

STRING(REPLACE "-Wl,--fatal-warnings" "" CMAKE_SHARED_LINKER_FLAGS_NOFATALWARN "${CMAKE_SHARED_LINKER_FLAGS}")
SET(CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS_NOFATALWARN} )

STRING(REPLACE "-Wl,--fatal-warnings" "" CMAKE_MODULE_LINKER_FLAGS_NOFATALWARN "${CMAKE_MODULE_LINKER_FLAGS}")
SET(CMAKE_MODULE_LINKER_FLAGS ${CMAKE_MODULE_LINKER_FLAGS_NOFATALWARN} )

target_link_libraries(amarok_collection-mysqlcollection
    amarok-sqlcollection
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
    target_link_libraries( amarok_collection-mysqlcollection crypt pthread )
endif(NOT WIN32 AND NOT APPLE)

if(APPLE)
	SET_TARGET_PROPERTIES(amarok_collection-mysqlcollection PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)

install(TARGETS amarok_collection-mysqlcollection DESTINATION ${PLUGIN_INSTALL_DIR} )

install(FILES amarok_collection-mysqlcollection.desktop DESTINATION ${SERVICES_INSTALL_DIR})


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
    Qt::Core
    Qt::Gui
    ${CMAKE_DL_LIBS}
)

if(BUILD_MYSQLE_COLLECTION)
    target_link_libraries( amarok_collection-mysqlcollection ${MYSQLE_LIBRARIES} )
endif()

if(APPLE)
	SET_TARGET_PROPERTIES(amarok_collection-mysqlcollection PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif()

install(TARGETS amarok_collection-mysqlcollection DESTINATION ${KDE_INSTALL_PLUGINDIR} )

install(FILES amarok_collection-mysqlcollection.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

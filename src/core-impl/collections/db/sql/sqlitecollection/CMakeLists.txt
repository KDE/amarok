########### sqlite ###############

set( amarok_collection-sqlitecollection_SRCS
    SqliteCollectionFactory.cpp
)

add_library(amarok_collection-sqlitecollection MODULE ${amarok_collection-sqlitecollection_SRCS})

string(REPLACE "-Wl,--fatal-warnings" "" CMAKE_SHARED_LINKER_FLAGS_NOFATALWARN "${CMAKE_SHARED_LINKER_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS_NOFATALWARN} )

string(REPLACE "-Wl,--fatal-warnings" "" CMAKE_MODULE_LINKER_FLAGS_NOFATALWARN "${CMAKE_MODULE_LINKER_FLAGS}")
set(CMAKE_MODULE_LINKER_FLAGS ${CMAKE_MODULE_LINKER_FLAGS_NOFATALWARN} )

target_link_libraries(amarok_collection-sqlitecollection
    amarok-sqlcollection
    amarokcore
    amaroklib
    Qt::Core
    Qt::Gui
    ${CMAKE_DL_LIBS}
)

if(APPLE)
    SET_TARGET_PROPERTIES(amarok_collection-sqlitecollection PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif()

install(TARGETS amarok_collection-sqlitecollection DESTINATION ${KDE_INSTALL_PLUGINDIR})

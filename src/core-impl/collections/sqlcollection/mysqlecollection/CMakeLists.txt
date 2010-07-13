#build-time only dependency in XesamDbus.h
#remove at some point

add_definitions(${MYSQL_EMBEDDED_CFLAGS})

include_directories(
        .
        ..
        ${CMAKE_SOURCE_DIR}/src
        ${QT_QTCORE_INCLUDES}
        ${MYSQL_INCLUDE_DIR}
        )

########### mysqle ###############

set(amarok_collection-mysqlecollection_PART_SRCS
    ../mysql-shared/MySqlStorage.cpp
    MySqlEmbeddedCollection.cpp
    MySqlEmbeddedStorage.cpp
 )

kde4_add_plugin(amarok_collection-mysqlecollection ${amarok_collection-mysqlecollection_PART_SRCS})

STRING(REPLACE "-Wl,--fatal-warnings" "" CMAKE_SHARED_LINKER_FLAGS_NOFATALWARN ${CMAKE_SHARED_LINKER_FLAGS})
SET(CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS_NOFATALWARN} )

STRING(REPLACE "-Wl,--fatal-warnings" "" CMAKE_MODULE_LINKER_FLAGS_NOFATALWARN ${CMAKE_MODULE_LINKER_FLAGS})
SET(CMAKE_MODULE_LINKER_FLAGS ${CMAKE_MODULE_LINKER_FLAGS_NOFATALWARN} )

target_link_libraries(amarok_collection-mysqlecollection
    amarok-sqlcollection
    amarokcore
    amaroklib
    ${KDE4_KDECORE_LIBS}
    ${QT_QTCORE_LIBRARY}
    ${QT_QTGUI_LIBRARY}
    ${MYSQL_EMBEDDED_LIBRARIES}
    ${CMAKE_DL_LIBS}
    ${ZLIB_LIBRARIES}
)

if(APPLE)
	SET_TARGET_PROPERTIES(amarok_collection-mysqlecollection PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)

install(TARGETS amarok_collection-mysqlecollection DESTINATION ${PLUGIN_INSTALL_DIR} )

install(FILES amarok_collection-mysqlecollection.desktop DESTINATION ${SERVICES_INSTALL_DIR})


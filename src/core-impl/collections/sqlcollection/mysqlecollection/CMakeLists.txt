#build-time only dependency in XesamDbus.h
#remove at some point

include_directories(
        .
        ..
        ${CMAKE_SOURCE_DIR}/src
        ${QT_QTCORE_INCLUDES}
        ${OPENSSL_INCLUDE_DIR}
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
    ${OPENSSL_LIBRARIES}
    ${MYSQL_EMBEDDED_LIBRARIES}
    ${CMAKE_DL_LIBS}
    ${ZLIB_LIBRARIES}
)

if(NOT WIN32)
    target_link_libraries(amarok_collection-mysqlecollection crypto ssl)
    if(NOT APPLE)
        target_link_libraries(amarok_collection-mysqlecollection crypt pthread)
    endif(NOT APPLE)
endif(NOT WIN32)

if(APPLE)
	SET_TARGET_PROPERTIES(amarok_collection-mysqlecollection PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)

install(TARGETS amarok_collection-mysqlecollection DESTINATION ${PLUGIN_INSTALL_DIR} )

install(FILES amarok_collection-mysqlecollection.desktop DESTINATION ${SERVICES_INSTALL_DIR})


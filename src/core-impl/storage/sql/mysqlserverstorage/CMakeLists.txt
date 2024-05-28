########### mysqlserver ###############

include_directories(SYSTEM ${MYSQL_INCLUDE_DIR})

set( amarok_storage-mysqlserverstorage_PART_SRCS
     ../mysql-shared/MySqlStorage.cpp
     MySqlServerStorage.cpp
     MySqlServerStorageFactory.cpp
)

add_library(amarok_storage-mysqlserverstorage MODULE ${amarok_storage-mysqlserverstorage_PART_SRCS})

string(REPLACE "-Wl,--fatal-warnings" "" CMAKE_SHARED_LINKER_FLAGS_NOFATALWARN "${CMAKE_SHARED_LINKER_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS_NOFATALWARN} )

string(REPLACE "-Wl,--fatal-warnings" "" CMAKE_MODULE_LINKER_FLAGS_NOFATALWARN "${CMAKE_MODULE_LINKER_FLAGS}")
set(CMAKE_MODULE_LINKER_FLAGS ${CMAKE_MODULE_LINKER_FLAGS_NOFATALWARN} )

target_link_libraries(amarok_storage-mysqlserverstorage
    # amarok-sqlstorage
    amarokcore
    amaroklib
    Qt::Core
    Qt::Gui
    ${MYSQL_LIBRARIES}
    ${CMAKE_DL_LIBS}
)

if(NOT WIN32 AND NOT APPLE)
    target_link_libraries( amarok_storage-mysqlserverstorage crypt pthread )
endif()

if(APPLE)
	SET_TARGET_PROPERTIES(amarok_storage-mysqlserverstorage PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif()

install(TARGETS amarok_storage-mysqlserverstorage DESTINATION ${KDE_INSTALL_PLUGINDIR} )
if(!BUILD_WITH_QT6)
  install(FILES amarok_storage-mysqlserverstorage.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
endif()

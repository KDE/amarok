########### sqlite ###############

set( amarok_storage-sqlite_PART_SRCS
     SqliteStorage.cpp
     SqliteStorageFactory.cpp
 )

add_library(amarok_storage-sqlite MODULE ${amarok_storage-sqlite_PART_SRCS})

target_link_libraries(amarok_storage-sqlite
    amarokcore
    amaroklib
    Qt::Core
    Qt::Sql
    Qt::Gui
)

install(TARGETS amarok_storage-sqlite DESTINATION ${KDE_INSTALL_PLUGINDIR} )
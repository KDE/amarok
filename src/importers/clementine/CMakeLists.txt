set( amarok_importer-clementine_PART_SRCS
    ClementineConfigWidget.cpp
    ClementineManager.cpp
    ClementineProvider.cpp
    ClementineTrack.cpp
)

add_library(amarok_importer-clementine MODULE ${amarok_importer-clementine_PART_SRCS} )

target_link_libraries( amarok_importer-clementine
    amarokcore
    amaroklib
    KF${KF_MAJOR_VERSION}::KIOCore
    Qt::Sql
)

install( TARGETS amarok_importer-clementine DESTINATION ${KDE_INSTALL_PLUGINDIR} )

set( amarok_importer-banshee_PART_SRCS
    BansheeConfigWidget.cpp
    BansheeManager.cpp
    BansheeProvider.cpp
    BansheeTrack.cpp
)

add_library(amarok_importer-banshee MODULE ${amarok_importer-banshee_PART_SRCS} )

target_link_libraries( amarok_importer-banshee
    amarokcore
    amaroklib
    KF${KF_MAJOR_VERSION}::KIOCore
    Qt::Sql
)

install( TARGETS amarok_importer-banshee DESTINATION ${KDE_INSTALL_PLUGINDIR} )

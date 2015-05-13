include_directories(
    
    
)

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
    KF5::KIOCore
    Qt5::Sql
)

install( TARGETS amarok_importer-banshee DESTINATION ${PLUGIN_INSTALL_DIR} )

install( FILES amarok_importer-banshee.desktop DESTINATION ${SERVICES_INSTALL_DIR} )

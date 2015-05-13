include_directories(
    
    
)

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
    KF5::KIOCore
    Qt5::Sql
)

install( TARGETS amarok_importer-clementine DESTINATION ${PLUGIN_INSTALL_DIR} )

install( FILES amarok_importer-clementine.desktop DESTINATION ${SERVICES_INSTALL_DIR} )

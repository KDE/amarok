include_directories(
    ${CMAKE_BINARY_DIR}/src
    ${AMAROK_COLLECTION_SUPPORT_DIR}
    ${KDE4_INCLUDE_DIR}
    ${QT_INCLUDES}
)

set( amarok_importer-banshee_PART_SRCS
    BansheeConfigWidget.cpp
    BansheeManager.cpp
    BansheeProvider.cpp
)

kde4_add_ui_files( amarok_importer-banshee_PART_SRCS BansheeConfigWidget.ui )

kde4_add_plugin( amarok_importer-banshee ${amarok_importer-banshee_PART_SRCS} )

target_link_libraries( amarok_importer-banshee
    amarokcore
    amaroklib
    ${KDE4_KIO_LIBS}
    ${QT_QTSQL_LIBRARY}
)

install( TARGETS amarok_importer-banshee DESTINATION ${PLUGIN_INSTALL_DIR} )

install( FILES amarok_importer-banshee.desktop DESTINATION ${SERVICES_INSTALL_DIR} )


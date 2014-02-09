include_directories(
    ${KDE4_INCLUDE_DIR}
    ${QT_INCLUDES}
)

set( amarok_importer-amarok_PART_SRCS
    AmarokConfigWidget.cpp
    AmarokManager.cpp
    AmarokProvider.cpp
    AmarokEmbeddedSqlConnection.cpp
    AmarokTrack.cpp
)

kde4_add_ui_files( amarok_importer-amarok_PART_SRCS AmarokConfigWidget.ui )

kde4_add_plugin( amarok_importer-amarok ${amarok_importer-amarok_PART_SRCS} )

target_link_libraries( amarok_importer-amarok
    amarokcore
    amaroklib
    ${KDE4_KIO_LIBS}
    ${QT_QTSQL_LIBRARY}
)

install( TARGETS amarok_importer-amarok DESTINATION ${PLUGIN_INSTALL_DIR} )

install( FILES amarok_importer-amarok.desktop DESTINATION ${SERVICES_INSTALL_DIR} )

include_directories(
    ${KDE4_INCLUDE_DIR}
    ${QT_INCLUDES}
)

set( amarok_importer-clementine_PART_SRCS
    ClementineConfigWidget.cpp
    ClementineManager.cpp
    ClementineProvider.cpp
    ClementineTrack.cpp
)

kde4_add_plugin( amarok_importer-clementine ${amarok_importer-clementine_PART_SRCS} )

target_link_libraries( amarok_importer-clementine
    amarokcore
    amaroklib
    ${KDE4_KIO_LIBS}
    ${QT_QTSQL_LIBRARY}
)

install( TARGETS amarok_importer-clementine DESTINATION ${PLUGIN_INSTALL_DIR} )

install( FILES amarok_importer-clementine.desktop DESTINATION ${SERVICES_INSTALL_DIR} )

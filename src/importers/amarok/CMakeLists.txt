set( amarok_importer-amarok_PART_SRCS
    AmarokConfigWidget.cpp
    AmarokManager.cpp
    AmarokProvider.cpp
    AmarokEmbeddedSqlConnection.cpp
    AmarokTrack.cpp
)

ki18n_wrap_ui( amarok_importer-amarok_PART_SRCS AmarokConfigWidget.ui )

add_library(amarok_importer-amarok MODULE ${amarok_importer-amarok_PART_SRCS} )

target_link_libraries( amarok_importer-amarok
    amarokcore
    amaroklib
    KF${KF_MAJOR_VERSION}::KIOCore
    Qt::Sql
)

install( TARGETS amarok_importer-amarok DESTINATION ${KDE_INSTALL_PLUGINDIR} )

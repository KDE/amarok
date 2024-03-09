set( amarok_importer-rhythmbox_PART_SRCS
    RhythmboxConfigWidget.cpp
    RhythmboxManager.cpp
    RhythmboxProvider.cpp
    RhythmboxTrack.cpp
)

add_library(amarok_importer-rhythmbox MODULE ${amarok_importer-rhythmbox_PART_SRCS} )

target_link_libraries( amarok_importer-rhythmbox
    amarokcore
    amaroklib
    KF5::KIOCore
)

install( TARGETS amarok_importer-rhythmbox DESTINATION ${KDE_INSTALL_PLUGINDIR} )

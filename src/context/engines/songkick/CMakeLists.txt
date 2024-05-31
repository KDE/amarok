include_directories( ../../..
                     ../../../context
                     ../../../dialogs
                     ../../../../external
                   )

set( songkick_engine_SRCS
    SongkickEngine.cpp
)

add_library(amarok_data_engine_songkick MODULE ${songkick_engine_SRCS})
target_link_libraries( amarok_data_engine_songkick amarokcore amaroklib KF${KF_MAJOR_VERSION}::Plasma amarokqtjson KF${KF_MAJOR_VERSION}::KIOCore)

install( TARGETS amarok_data_engine_songkick DESTINATION ${KDE_INSTALL_PLUGINDIR} )
install( FILES amarok-data-engine-songkick.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR} )


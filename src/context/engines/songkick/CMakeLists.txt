include_directories( ../../..
                     ../../../context
                     ../../../dialogs
                     ../../../../external
                   )

set( songkick_engine_SRCS
    SongkickEngine.cpp
)

add_library(amarok_data_engine_songkick MODULE ${songkick_engine_SRCS})
target_link_libraries( amarok_data_engine_songkick amarokcore amaroklib ${KDE4_PLASMA_LIBS} amarokqtjson KF5::KIOCore)

install( TARGETS amarok_data_engine_songkick DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES amarok-data-engine-songkick.desktop DESTINATION ${SERVICES_INSTALL_DIR} )


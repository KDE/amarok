include_directories( ../../..
                     ../../../context
                     ../../../dialogs
                     ../../../../external
                   )

set( songkick_engine_SRCS
    SongkickEngine.cpp
)

kde4_add_plugin(amarok_data_engine_songkick ${songkick_engine_SRCS})
target_link_libraries( amarok_data_engine_songkick amarokcore amaroklib ${KDE4_PLASMA_LIBS} amarokqtjson ${KDE4_KIO_LIBS})

install( TARGETS amarok_data_engine_songkick DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES amarok-data-engine-songkick.desktop DESTINATION ${SERVICES_INSTALL_DIR} )


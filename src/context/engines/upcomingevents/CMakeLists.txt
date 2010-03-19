include_directories( ../../..
                     ../../../context
                     ${CMAKE_CURRENT_BINARY_DIR}/../../.. # for amarok_config.h
)

set( upcomingEvents_engine_SRCS
    UpcomingEventsEngine.cpp
    ../../applets/upcomingevents/LastFmEvent.cpp
)

kde4_add_plugin(amarok_data_engine_upcomingEvents ${upcomingEvents_engine_SRCS})
target_link_libraries( amarok_data_engine_upcomingEvents amaroklib ${KDE4_PLASMA_LIBS} ${KDE4_KIO_LIBS} ${LIBLASTFM_LIBRARY} )

install( TARGETS amarok_data_engine_upcomingEvents DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES amarok-data-engine-upcomingEvents.desktop DESTINATION ${SERVICES_INSTALL_DIR} )

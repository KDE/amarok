include_directories( ${Amarok_SOURCE_DIR}/src
                     ${Amarok_SOURCE_DIR}/src/context
                     ${Amarok_SOURCE_DIR}/src/context/applets/upcomingevents
                     ${LIBLASTFM_INCLUDE_DIR}
                     ${LIBLASTFM_INCLUDE_DIR}/..
                     ${CMAKE_CURRENT_BINARY_DIR}/../../.. # for amarok_config.h
)

set( upcomingEvents_engine_SRCS
     UpcomingEventsEngine.cpp
     ${Amarok_SOURCE_DIR}/src/context/applets/upcomingevents/LastFmEvent.cpp
     ${Amarok_SOURCE_DIR}/src/context/applets/upcomingevents/LastFmEventXmlParser.cpp
)

add_library(amarok_data_engine_upcomingEvents MODULE ${upcomingEvents_engine_SRCS})
target_link_libraries( amarok_data_engine_upcomingEvents amarokcore amaroklib KF5::Plasma KF5::KIOCore ${LIBLASTFM_LIBRARY} )

install( TARGETS amarok_data_engine_upcomingEvents DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES amarok-data-engine-upcomingEvents.desktop DESTINATION ${SERVICES_INSTALL_DIR} )

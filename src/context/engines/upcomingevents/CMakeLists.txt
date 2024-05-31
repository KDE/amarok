include_directories(SYSTEM
                     ${LIBLASTFM_INCLUDE_DIR}
                     ${LIBLASTFM_INCLUDE_DIR}/..
)
include_directories( ${Amarok_SOURCE_DIR}/src
                     ${Amarok_SOURCE_DIR}/src/context
                     ${Amarok_SOURCE_DIR}/src/context/applets/upcomingevents
                     ${CMAKE_CURRENT_BINARY_DIR}/../../.. # for amarok_config.h
)

set( upcomingEvents_engine_SRCS
     UpcomingEventsEngine.cpp
     ${Amarok_SOURCE_DIR}/src/context/applets/upcomingevents/LastFmEvent.cpp
     ${Amarok_SOURCE_DIR}/src/context/applets/upcomingevents/LastFmEventXmlParser.cpp
)

add_library(amarok_data_engine_upcomingEvents MODULE ${upcomingEvents_engine_SRCS})
target_link_libraries( amarok_data_engine_upcomingEvents amarokcore amaroklib KF${KF_MAJOR_VERSION}::Plasma KF${KF_MAJOR_VERSION}::KIOCore ${LIBLASTFM_LIBRARY} )

install( TARGETS amarok_data_engine_upcomingEvents DESTINATION ${KDE_INSTALL_PLUGINDIR} )
install( FILES amarok-data-engine-upcomingEvents.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR} )

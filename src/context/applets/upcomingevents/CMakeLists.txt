project(context-upcomingevents)

include_directories(
    ${Amarok_SOURCE_DIR}/src/context/widgets
    ${Amarok_SOURCE_DIR}/src/network
    ${Amarok_SOURCE_DIR}/src
    )

set( upcomingEvents_SRCS
     LastFmEvent.cpp
     LastFmEventXmlParser.cpp
     UpcomingEventsApplet.cpp
     UpcomingEventsWidget.cpp
     UpcomingEventsStack.cpp
     UpcomingEventsStackItem.cpp
     UpcomingEventsMapWidget.cpp
     UpcomingEventsCalendarWidget.cpp
     )

ki18n_wrap_ui( upcomingEvents_SRCS
                   upcomingEventsGeneralSettings.ui
                   upcomingEventsVenueSettings.ui
                   )
add_library(amarok_context_applet_upcomingEvents MODULE ${upcomingEvents_SRCS})

if(APPLE)
   set_target_properties(amarok_context_applet_upcomingEvents PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif()
target_link_libraries(amarok_context_applet_upcomingEvents
    amarokcore
    amaroklib
    KF5::Plasma
    
    ${KDE4_KDEWEBKIT_LIBS}
    Qt::Network
    Qt::WebKitWidgets
)

install(TARGETS amarok_context_applet_upcomingEvents DESTINATION ${KDE_INSTALL_PLUGINDIR})
install(FILES amarok-context-applet-upcomingEvents.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
install(FILES upcoming-events-map.html DESTINATION ${KDE_INSTALL_DATADIR}/amarok/data)

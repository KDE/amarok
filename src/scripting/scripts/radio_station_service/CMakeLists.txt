
install( FILES
        main.js
        DESTINATION ${KDE_INSTALL_DATADIR}/amarok/scripts/radio_station_service
)

install( FILES ${CMAKE_CURRENT_BINARY_DIR}/script.json
    DESTINATION ${KDE_INSTALL_DATADIR}/amarok/scripts/radio_station_service
)

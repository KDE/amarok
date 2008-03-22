
install( FILES
    COPYING
    README
        radio_station_service_script.spec
        DESTINATION ${DATA_INSTALL_DIR}/amarok/scripts/radio_station_service
)

install( PROGRAMS
        radio_station_service_script.rb
        DESTINATION ${DATA_INSTALL_DIR}/amarok/scripts/radio_station_service

)
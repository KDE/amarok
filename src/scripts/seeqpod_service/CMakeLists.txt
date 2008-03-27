
install( FILES
    COPYING
    README
        seeqpod_service_script.spec
        DESTINATION ${DATA_INSTALL_DIR}/amarok/scripts/seeqpod_service
)

install( PROGRAMS
        seeqpod_service_script.rb
        DESTINATION ${DATA_INSTALL_DIR}/amarok/scripts/seeqpod_service

)
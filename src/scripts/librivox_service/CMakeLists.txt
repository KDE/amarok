
install( FILES
    COPYING
    README
        librivox_service_script.spec
        DESTINATION ${DATA_INSTALL_DIR}/amarok/scripts/librivox_service
)

install( PROGRAMS
        librivox_service_script.rb
        DESTINATION ${DATA_INSTALL_DIR}/amarok/scripts/librivox_service

)
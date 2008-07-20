
install( FILES
    COPYING
    README
        script.spec
        DESTINATION ${DATA_INSTALL_DIR}/amarok/scripts/librivox_service
)

install( PROGRAMS
        main.js
        DESTINATION ${DATA_INSTALL_DIR}/amarok/scripts/librivox_service

)
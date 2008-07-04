
install( FILES
    COPYING
    README
        script.spec
        DESTINATION ${DATA_INSTALL_DIR}/amarok/scripts/file_browser_demo_service
)

install( PROGRAMS
        main.js
        DESTINATION ${DATA_INSTALL_DIR}/amarok/scripts/file_browser_demo_service

)
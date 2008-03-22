
install( FILES
    COPYING
    README
        file_browser_service_script.spec
        DESTINATION ${DATA_INSTALL_DIR}/amarok/scripts/file_browser_demo_service
)

install( PROGRAMS
        file_browser_service_script.rb
        DESTINATION ${DATA_INSTALL_DIR}/amarok/scripts/file_browser_demo_service

)
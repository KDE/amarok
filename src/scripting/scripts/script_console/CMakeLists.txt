install( FILES
        main.js
        DESTINATION ${DATA_INSTALL_DIR}/amarok/scripts/script_console
)

kcoreaddons_desktop_to_json("" script.desktop)

install( FILES ${CMAKE_CURRENT_BINARY_DIR}/script.json
    DESTINATION ${DATA_INSTALL_DIR}/amarok/scripts/script_console
)

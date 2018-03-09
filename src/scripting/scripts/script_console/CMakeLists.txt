install( FILES
        main.js
        DESTINATION ${KDE_INSTALL_DATADIR}/amarok/scripts/script_console
)

kcoreaddons_desktop_to_json("" script.desktop)

install( FILES ${CMAKE_CURRENT_BINARY_DIR}/script.json
    DESTINATION ${KDE_INSTALL_DATADIR}/amarok/scripts/script_console
)

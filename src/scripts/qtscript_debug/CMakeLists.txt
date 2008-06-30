install( FILES
        script.spec
        DESTINATION ${DATA_INSTALL_DIR}/amarok/scripts/qtscript_debug
)

install( PROGRAMS
        debug.js
        main.js
        DESTINATION ${DATA_INSTALL_DIR}/amarok/scripts/qtscript_debug
)
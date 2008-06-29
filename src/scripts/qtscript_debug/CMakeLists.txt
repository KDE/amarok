install( FILES
        qtscript.spec
        DESTINATION ${DATA_INSTALL_DIR}/amarok/scripts/qtscript_debug
)

install( PROGRAMS
        debug.js
        qtscript.js
        DESTINATION ${DATA_INSTALL_DIR}/amarok/scripts/qtscript_debug
)
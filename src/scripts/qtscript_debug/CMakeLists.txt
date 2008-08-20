install( FILES
        script.spec
        DESTINATION ${DATA_INSTALL_DIR}/amarok/scripts/qtscript_debug
)

install( PROGRAMS
        main.js
        DESTINATION ${DATA_INSTALL_DIR}/amarok/scripts/qtscript_debug
)

install( PROGRAMS
        debug/__init__.js
        DESTINATION ${LIB_INSTALL_DIR}/kde4/plugins/script/amarok/qtscript_debug/debug
)
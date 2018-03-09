install( FILES
        script.spec
        DESTINATION ${KDE_INSTALL_DATADIR}/amarok/scripts/qtscript_debug
)

install( PROGRAMS
        main.js
        DESTINATION ${KDE_INSTALL_DATADIR}/amarok/scripts/qtscript_debug
)

install( PROGRAMS
        debug/debug.js
        DESTINATION ${KDE_INSTALL_DATADIR}/amarok/scripts/qtscript_debug/debug
)

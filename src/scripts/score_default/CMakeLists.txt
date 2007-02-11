install( FILES
    COPYING
    README
    score_default.spec
    DESTINATION ${DATA_INSTALL_DIR}/amarok/scripts/score_default
)

install( PROGRAMS
    score_default.rb
    DESTINATION ${DATA_INSTALL_DIR}/amarok/scripts/score_default
)
    

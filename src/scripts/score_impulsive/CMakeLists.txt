install( FILES
    COPYING
    README
    score_impulsive.spec
    DESTINATION ${DATA_INSTALL_DIR}/amarok/scripts/score_impulsive
)

install( PROGRAMS
    score_impulsive.rb
    DESTINATION ${DATA_INSTALL_DIR}/amarok/scripts/score_impulsive
)


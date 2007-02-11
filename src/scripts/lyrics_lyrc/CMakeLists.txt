
install( FILES
    COPYING
    README
    lyrics_lyrc.spec
    DESTINATION ${DATA_INSTALL_DIR}/amarok/scripts/lyrics_lyrc
)

install( PROGRAMS
    lyrics_lyrc.rb
    DESTINATION ${DATA_INSTALL_DIR}/amarok/scripts/lyrics_lyrc
)


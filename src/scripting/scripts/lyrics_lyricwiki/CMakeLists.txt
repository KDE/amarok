install( FILES
        main.js
        DESTINATION ${DATA_INSTALL_DIR}/amarok/scripts/lyrics_lyricwiki
)

# Scripty is tailored to process desktop files only. On the other hand, there are standard spec files
# for RH-packaging. We use desktop files for translation and install them as spec files for use.
install( FILES
        script.desktop
        DESTINATION ${DATA_INSTALL_DIR}/amarok/scripts/lyrics_lyricwiki
        RENAME script.spec
)

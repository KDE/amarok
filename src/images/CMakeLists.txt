
add_subdirectory( icons )

########### install files ###############

install(
        FILES
        FilenameLayoutDialogHint.png
        amarok_icon.svg
        default-theme.svg
        emblem-default.png
        emblem-jamendo.png
        emblem-lastfm.png
        emblem-magnatune.png
        emblem-mp3tunes.png
        emblem-ampache.png
        emblem-scripted.png
        lastfm-default-cover.png
        loading1.png
        loading2.png
        navigation_arrows.svg
        nocover.png
        pud_items.svg
        shadow_albumcover.png
        smallstar.png
        splash_screen.jpg
        star.png
        volume_icon.png
        web_applet_background.svg
        DESTINATION ${DATA_INSTALL_DIR}/amarok/images
)

kde4_install_icons( ${ICON_INSTALL_DIR} )

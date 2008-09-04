
add_subdirectory( icons )

########### install files ###############

install(
        FILES
        FilenameLayoutDialogHint.png
        amarok_icon.svg
        back_stars_grey.png
        currenttrack_pause.png
        currenttrack_play.png
        currenttrack_repeat.png
        currenttrack_repeat_small.png
        currenttrack_stop.png
        currenttrack_stop_small.png
        default-theme.svg
        emblem-default.png
        emblem-jamendo.png
        emblem-lastfm.png
        emblem-magnatune.png
        emblem-mp3tunes.png
        emblem-ampache.png
        icons/svg/sources.svgz
        lastfm-default-cover.png
        loading1.png
        loading2.png
        menu_sidepixmap.png
        more_albums.png
        musicbrainz.png
        navigation_arrows.svg
        nocover.png
        pud_items.svg
        sbinner_stars.png
        service_info_frontpage_bg.png
        service_info_frontpage_logo.png
        service_info_frontpage_shadow.png
        shadow_albumcover.png
        smallstar.png
        splash_screen.jpg
        star.png
        volume_icon.png
        web_applet_background.svg
        DESTINATION ${DATA_INSTALL_DIR}/amarok/images
)

kde4_install_icons( ${ICON_INSTALL_DIR} )

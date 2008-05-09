
add_subdirectory( icons )

########### install files ###############

install(
        FILES
        amarok_icon.svg
        back_stars_grey.png
        currenttrack_play.png
        currenttrack_pause.png
        currenttrack_stop.png
        currenttrack_stop_small.png
        currenttrack_repeat.png
        currenttrack_repeat_small.png
        loading1.png
        loading2.png
        menu_sidepixmap.png
        more_albums.png
        musicbrainz.png
        nocover.png
        shadow_albumcover.png
        sbinner_stars.png
        smallstar.png
        icons/svg/sources.svg
        OsdBackground.svg
        service-browser-element.svg
        sidebar_button.svg
        splash_screen.jpg
        star.png
        toolbar-background.svg
        playlist_items.svg
        emblem-magnatune.png
        emblem-lastfm.png
        emblem-jamendo.png
        emblem-default.png
        sliders.svg
        pud_items.svg
        default-template.svg
        service_info_frontpage_bg.png
        service_info_frontpage_logo.png
        service_info_frontpage_shadow.png
        DESTINATION ${DATA_INSTALL_DIR}/amarok/images
)

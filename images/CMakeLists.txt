add_subdirectory( icons )

########### install files ###############

install(FILES
        amarok_icon.svg
        ball.png
        default-theme-clean.svg
        dot.png
        emblem-amazon.png
        emblem-default.png
        emblem-jamendo.png
        emblem-jamendo-scalable.svgz
        emblem-lastfm.png
        emblem-lastfm-scalable.svg
        emblem-gpodder.png
        emblem-gpodder-scalable.svgz
        emblem-magnatune.png
        emblem-mp3tunes.png
        emblem-ampache.png
        emblem-ampache-scalable.svgz
        emblem-scripted.png
        emblem-scripted-scalable.svgz
        grid.png
        lastfm-default-cover.png
        echonest.png
        lastfm.png
        loading1.png
        loading2.png
        mb_aicon.png
        mb_licon.png
        mb_ticon.png
        navigation_arrows.svg
        nocover.png
        playlist-bookmark-16.png
        playlist-layouts-22.png
        playlist-sorting-16.png
        pud_items.svg
        smallstar.png
        star.png
        volume_icon.png
        volume_muted_icon.png
        wirl1.png
        wirl2.png
        service_info_loading1.png
        service_info_loading2.png
        service_info_loading3.png
        service_info_loading4.png
        service_info_loading5.png
        service_info_loading6.png
        service_info_loading7.png
        service_info_loading8.png
        service_info_loading9.png
        service_info_loading10.png
        service_info_loading11.png
        service_info_loading12.png
        hover_info_collections.png
        hover_info_dynamic_playlists.png
        hover_info_files.png
        hover_info_internet.png
        hover_info_playlists.png
        hover_info_user_playlists.png
        hover_info_podcasts.png
        opendesktop-22.png

        emblem-delicious.png
        emblem-digg.png
        emblem-facebook.png
        emblem-identica.png
        emblem-linkedin.png
        emblem-myspace.png
        emblem-reddit.png
        emblem-stackoverflow.png
        emblem-twitter.png
        emblem-wikipedia.png
        emblem-xing.png

        DESTINATION ${KDE_INSTALL_DATADIR}/amarok/images
)

ecm_install_icons(ICONS
    128-apps-amarok.png
    16-apps-amarok.png
    22-apps-amarok.png
    32-apps-amarok.png
    48-apps-amarok.png
    64-apps-amarok.png
    DESTINATION ${KDE_INSTALL_ICONDIR}
    THEME hicolor
)


add_subdirectory( icons )

########### install files ###############

install(
        FILES
        FilenameLayoutDialogHint.png
        amarok_icon.svg
        default-theme-clean.svg
        emblem-default.png
        emblem-jamendo.png
	emblem-jamendo-scalable.svgz
        emblem-lastfm.png
	emblem-lastfm-scalable.svg
        emblem-magnatune.png
        emblem-mp3tunes.png
        emblem-ampache.png
	emblem-ampache-scalable.svgz
        emblem-scripted.png
	emblem-scripted-scalable.svgz
        lastfm-default-cover.png
        loading1.png
        loading2.png
        navigation_arrows.svg
        nocover.png
        pud_items.svg
        smallstar.png
        splash_screen.jpg
        star.png
        volume_icon.png
        volume_muted_icon.png
        web_applet_background.svg
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
	opendesktop.png
        DESTINATION ${DATA_INSTALL_DIR}/amarok/images
)

kde4_install_icons( ${ICON_INSTALL_DIR} )

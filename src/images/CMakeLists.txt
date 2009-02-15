
add_subdirectory( icons )

########### install files ###############

install(
        FILES
        FilenameLayoutDialogHint.png
        amarok_icon.svg
        default-theme-clean.svg
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
        smallstar.png
        splash_screen.jpg
        star.png
        volume_icon.png
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
        DESTINATION ${DATA_INSTALL_DIR}/amarok/images
)

kde4_install_icons( ${ICON_INSTALL_DIR} )

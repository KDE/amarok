
install( FILES
    COPYING
    README
    main.js
    LibrivoxService.html
    LibrivoxLogo.png
    LibrivoxIcon.png
    LibrivoxEmblem.png
    LibrivoxScalableEmblem.svgz
    audio_book128.png
    DESTINATION ${DATA_INSTALL_DIR}/amarok/scripts/librivox_service
)

# Scripty is tailored to process desktop files only. On the other hand, there are standard spec files
# for RH-packaging. We use desktop files for translation and install them as spec files for use.
install( FILES
    script.desktop
    DESTINATION ${DATA_INSTALL_DIR}/amarok/scripts/librivox_service
    RENAME script.spec
)

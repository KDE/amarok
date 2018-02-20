
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

kcoreaddons_desktop_to_json("" script.desktop)

install( FILES ${CMAKE_CURRENT_BINARY_DIR}/script.json
    DESTINATION ${DATA_INSTALL_DIR}/amarok/scripts/librivox_service
)

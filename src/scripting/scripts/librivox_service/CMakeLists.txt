
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
    DESTINATION ${KDE_INSTALL_DATADIR}/amarok/scripts/librivox_service
)

install( FILES ${CMAKE_CURRENT_BINARY_DIR}/script.json
    DESTINATION ${KDE_INSTALL_DATADIR}/amarok/scripts/librivox_service
)

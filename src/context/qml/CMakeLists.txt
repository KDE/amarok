
SET(qml_files
    globals.js
    RichTextualViewer.qml
    InfoWidget.qml
    Context.qml
    TextualViewer.qml)

install(FILES ${qml_files} DESTINATION ${DATA_INSTALL_DIR}/amarok/qml/)
install(FILES images/theme.svg DESTINATION ${DATA_INSTALL_DIR}/desktoptheme/Amarok/Amarok/)
install(FILES images/metadata.desktop DESTINATION ${DATA_INSTALL_DIR}/desktoptheme/Amarok/)


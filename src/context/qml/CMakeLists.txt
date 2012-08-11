
SET(qml_files
    RichTextualViewer.qml
    Context.qml
    TextualViewer.qml)

install(FILES ${qml_files} DESTINATION ${DATA_INSTALL_DIR}/amarok/qml/)
install(FILES images/theme.svg DESTINATION ${DATA_INSTALL_DIR}/desktoptheme/default/Amarok/)

include_directories( ../../..
                     ../../../context
                     ${CMAKE_CURRENT_BINARY_DIR}/../../.. # for amarok_config.h
)

set( spectrum_analyzer_engine_SRCS SpectrumAnalyzerEngine.cpp )

kde4_add_plugin( amarok_data_engine_spectrum_analyzer ${spectrum_analyzer_engine_SRCS} )
target_link_libraries( amarok_data_engine_spectrum_analyzer amarokcore amaroklib ${KDE4_PLASMA_LIBS} ${KDE4_KDECORE_LIBS} ${QT_QTGUI_LIBRARY} ${KDE4_PHONON_LIBRARY} )

install( TARGETS amarok_data_engine_spectrum_analyzer DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES amarok-data-engine-spectrum-analyzer.desktop DESTINATION ${SERVICES_INSTALL_DIR} )

project(context-analyzer)

set(analyzer_SRCS
    AnalyzerApplet.cpp
    AnalyzerBase.cpp
    BallsAnalyzer.cpp
    BlockAnalyzer.cpp
    DiscoAnalyzer.cpp
    fht.cpp
)

kde4_add_plugin(amarok_context_applet_analyzer ${analyzer_SRCS})

if(APPLE)
    SET_TARGET_PROPERTIES(amarok_context_applet_analyzer PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)

target_link_libraries(amarok_context_applet_analyzer
    amarokcore
    amaroklib
    ${KDE4_PLASMA_LIBS}
    ${QT_QTOPENGL_LIBRARY}
    GL
)

install(TARGETS amarok_context_applet_analyzer DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES amarok-context-applet-analyzer.desktop DESTINATION ${SERVICES_INSTALL_DIR})

set(analyzer_SRCS
    plugin/AnalyzerPlugin.cpp
    plugin/AnalyzerBase.cpp
    plugin/AnalyzerWorker.cpp
    #plugin/BallsAnalyzer.cpp
    plugin/BlockAnalyzer.cpp
    plugin/BlockWorker.cpp
    #plugin/DiscoAnalyzer.cpp
    #plugin/ASCIIAnalyzer.cpp
)

add_library(amarok_context_applet_analyzer SHARED ${analyzer_SRCS})

if(APPLE)
    set_target_properties(amarok_context_applet_analyzer PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif()

target_link_libraries(amarok_context_applet_analyzer
    amarokcore
    amaroklib
    Qt::Qml
    Qt::Quick
    ${FFTW3_LIBRARIES}
)

install(TARGETS amarok_context_applet_analyzer DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/amarok/analyzer)
install(FILES plugin/qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/amarok/analyzer)

kpackage_install_package(package org.kde.amarok.analyzer amarok)

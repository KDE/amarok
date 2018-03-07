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

find_library(FFTW_LIBRARIES NAMES fftw3)
if(NOT FFTW_LIBRARIES)
    message(FATAL_ERROR "fftw not found")
endif(NOT FFTW_LIBRARIES)

add_library(amarok_context_applet_analyzer SHARED ${analyzer_SRCS})

if(APPLE)
    set_target_properties(amarok_context_applet_analyzer PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif()

target_link_libraries(amarok_context_applet_analyzer
    amarokcore
    amaroklib
    Qt5::Qml
    Qt5::Quick
    fftw3
)

install(TARGETS amarok_context_applet_analyzer DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/amarok/analyzer)
install(FILES plugin/qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/amarok/analyzer)

kpackage_install_package(package org.kde.amarok.analyzer amarok)

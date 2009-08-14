cmake_minimum_required (VERSION 2.6)

find_package( KDE4 REQUIRED )

add_definitions(${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
include_directories (${QT_QTCORE_INCLUDE_DIR} ${QT_QTSCRIPT_INCLUDE_DIR} ${KDE4_INCLUDES})

add_executable (bindingstest QtScriptBindingsTest.cpp)
target_link_libraries( bindingstest ${QT_QTSCRIPT_LIBRARY} ${KDE4_KDEUI_LIBS} )


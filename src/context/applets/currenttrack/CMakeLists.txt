project(context-currenttrack)

set(currenttrack_SRCS
CurrentTrack.cpp)

find_package(KDE4 REQUIRED)
include (KDE4Defaults)
include (MacroLibrary)

add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS})

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../../../
                    ${CMAKE_CURRENT_SOURCE_DIR}/../../
                    ${CMAKE_BINARY_DIR} 
                    ${KDE4_INCLUDES} 
                    ${QT_INCLUDES}
                    ${KDE4_INCLUDE_DIR}/amarok ) # this way we don't need to prefix it with amarok/ (and it compiles this way too :)

kde4_add_plugin(amarok_context_applet_currenttrack ${currenttrack_SRCS})
target_link_libraries(amarok_context_applet_currenttrack amaroklib ${KDE4_KIO_LIBS})
set_target_properties(amarok_context_applet_currenttrack PROPERTIES VERSION 1.0.0 SOVERSION 1)

install(TARGETS amarok_context_applet_currenttrack DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES amarok-context-applet-currenttrack.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES currenttrack.svg DESTINATION ${DATA_INSTALL_DIR}/desktoptheme/amarok-default/widgets/ )

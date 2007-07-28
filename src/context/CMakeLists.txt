# NOTE we link libplasma here instead of in the CMakeLists in plasma/ because we need control over the installation of header files and library name
# this requires periodic syncing of this file with plasma/CMakeLists.txt as it changes, but thats better than the alternative. (the only option really.)

add_subdirectory( applets )
add_subdirectory( engines )

include_directories( ${CMAKE_CURRENT_SOURCE_DIR} 
                    ${CMAKE_CURRENT_SOURCE_DIR}/.. 
                    ${CMAKE_CURRENT_SOURCE_DIR}/../amarokcore
                    ${CMAKE_CURRENT_SOURCE_DIR}/widgets
                    ${CMAKE_CURRENT_SOURCE_DIR}/plasma
                    ${CMAKE_CURRENT_BINARY_DIR}/..)


########### next target ###############

set(plasmagik_SRCS
    plasma/packagemetadata.cpp
    plasma/packagestructure.cpp
    plasma/package.cpp
)

set(plasma_LIB_SRCS
    ${plasmagik_SRCS}
    plasma/abstractrunner.cpp
    plasma/animator.cpp
    plasma/applet.cpp
    plasma/configxml.cpp
    plasma/corona.cpp
    plasma/datacontainer.cpp
    plasma/dataengine.cpp
    plasma/dataenginemanager.cpp
    plasma/packages.cpp
    plasma/phase.cpp
    plasma/plasma.cpp
    plasma/plasma_export.h
    plasma/scriptengine.cpp
    plasma/svg.cpp
    plasma/theme.cpp
    plasma/karambamanager.cpp
    plasma/widgets/boxlayout.cpp
    plasma/widgets/checkbox.cpp
    plasma/widgets/hboxlayout.cpp
    plasma/widgets/icon.cpp
    plasma/widgets/lineedit.cpp
    plasma/widgets/pushbutton.cpp
    plasma/widgets/radiobutton.cpp
    plasma/widgets/widget.cpp
    plasma/widgets/layout.cpp
    plasma/widgets/layoutitem.cpp
    plasma/widgets/vboxlayout.cpp
    plasma/widgets/flash.cpp
)

kde4_add_library(amarokplasma SHARED ${plasma_LIB_SRCS})

target_link_libraries(amarokplasma ${KDE4_KIO_LIBS})

set_target_properties(amarokplasma PROPERTIES VERSION 1.0.0 SOVERSION 1)
install(TARGETS amarokplasma DESTINATION ${LIB_INSTALL_DIR})

########### install files ###############

#install(FILES
#    abstractrunner.h
#    animator.h
#    applet.h
#    corona.h
#    dataengine.h
#    dataenginemanager.h
#    datasource.h
#    phase.h
#    plasma.h
#    plasma_export.h
#    svg.h
#    theme.h
#    DESTINATION ${INCLUDE_INSTALL_DIR}/plasma)

#install(FILES
#    widgets/icon.h
#    widgets/layout.h
#    widgets/layoutitem.h
#    widgets/lineedit.h
#    widgets/pushbutton.h
#    widgets/checkbox.h
#    widgets/radiobutton.h
#    widgets/vboxlayout.h
#    widgets/widget.h
#    DESTINATION ${INCLUDE_INSTALL_DIR}/plasma/widgets)

#install(FILES
#    includes/Applet
#    includes/Phase
#    includes/Plasma
#    includes/AbstractRunner
#    includes/Theme
#    includes/DataEngine
#    includes/DataEngineManager
#    includes/DataSource
#    includes/Svg
#    DESTINATION ${INCLUDE_INSTALL_DIR}/KDE/Plasma)
# 
# install(FILES
# #    servicetypes/plasma_animator.desktop
#     servicetypes/amarok_context_applet.desktop
#     servicetypes/amarok_context_dataengine.desktop
#     DESTINATION ${SERVICETYPES_INSTALL_DIR})
# 

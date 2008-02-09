# NOTE we link libplasma here instead of in the CMakeLists in plasma/ because we need control over the installation of header files and library name
# this requires periodic syncing of this file with plasma/CMakeLists.txt as it changes, but thats better than the alternative. (the only option really.)

find_package(OpenGL)

add_subdirectory( applets )
add_subdirectory( engines )
#message(STATUS "TODO: port the amarok containments to the new plasma API")
add_subdirectory( containments )

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}
                    ..
                    ../amarokcore
                    widgets
                    plasma
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
    plasma/appletbrowser.cpp
    plasma/appletbrowser/kcategorizeditemsview.cpp
    plasma/appletbrowser/kcategorizeditemsviewdelegate.cpp
    plasma/appletbrowser/kcategorizeditemsviewmodels.cpp
    plasma/appletbrowser/plasmaappletitemmodel.cpp
    plasma/appletbrowser/customdragtreeview.cpp
    plasma/applethandle.cpp
    plasma/configxml.cpp
    plasma/containment.cpp
    plasma/corona.cpp
    plasma/datacontainer.cpp
    plasma/dataengine.cpp
    plasma/dataenginemanager.cpp
    plasma/dialog.cpp
    plasma/layouts/boxlayout.cpp
    plasma/layouts/borderlayout.cpp
    plasma/layouts/freelayout.cpp
    plasma/layouts/nodelayout.cpp
    plasma/layouts/flowlayout.cpp
    plasma/layouts/fliplayout.cpp
    plasma/layouts/layout.cpp
    plasma/layouts/layoutanimator.cpp
    plasma/layouts/layoutitem.cpp
    plasma/packages.cpp
    plasma/phase.cpp
    plasma/plasma.cpp
    plasma/plasma_export.h
    plasma/searchcontext.cpp
    plasma/searchmatch.cpp
    plasma/shadowitem.cpp
    plasma/svg.cpp
    plasma/theme.cpp
    plasma/desktoptoolbox.cpp
    plasma/scriptengine.cpp
    plasma/uiloader.cpp
    plasma/view.cpp
    plasma/widgets/checkbox.cpp
    plasma/widgets/flash.cpp
    plasma/widgets/icon.cpp
    plasma/widgets/label.cpp
    plasma/widgets/lineedit.cpp
    plasma/widgets/progressbar.cpp
    plasma/widgets/pushbutton.cpp
    plasma/widgets/radiobutton.cpp
#    plasma/widgets/rectangle.cpp
    plasma/widgets/tooltip.cpp
    plasma/widgets/widget.cpp
    plasma/widgets/signalplotter.cpp
    plasma/widgets/meter.cpp
)

kde4_add_ui_files (
plasma_LIB_SRCS
plasma/appletbrowser/kcategorizeditemsviewbase.ui
)

if(QT_QTOPENGL_FOUND AND OPENGL_FOUND)
set(plasma_LIB_SRCS
    ${plasma_LIB_SRCS}
    plasma/glapplet.cpp)
endif(QT_QTOPENGL_FOUND AND OPENGL_FOUND)

kde4_add_library(amarokplasma SHARED ${plasma_LIB_SRCS})

target_link_libraries(amarokplasma
${KDE4_KIO_LIBS}
${QT_QTUITOOLS_LIBRARY}
${QT_QTOPENGL_LIBRARY}
${OPENGL_gl_LIBRARY}
${OPENGL_glu_LIBRARY}
)

add_definitions(-DMAKE_PLASMA_LIB)
set_target_properties(amarokplasma PROPERTIES VERSION 1.0.0 SOVERSION 1)
install(TARGETS amarokplasma DESTINATION ${LIB_INSTALL_DIR})

# base plasma not compiled under windows, so we need to install these here
if(WIN32)
install(FILES
    plasma/servicetypes/plasma-animator.desktop
    plasma/servicetypes/plasma-applet.desktop
    plasma/servicetypes/plasma-dataengine.desktop
    plasma/servicetypes/plasma-runner.desktop
    plasma/servicetypes/plasma-scriptengine.desktop
    DESTINATION ${SERVICETYPES_INSTALL_DIR})
endif(WIN32)

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
#    layouts/layout.h
#    layouts/layoutitem.h
#    widgets/lineedit.h
#    widgets/pushbutton.h
#    widgets/checkbox.h
#    widgets/radiobutton.h
#    layouts/vboxlayout.h
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

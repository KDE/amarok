# NOTE we link libplasma here instead of in the CMakeLists in plasma/ because we need control over the installation of header files and library name
# this requires periodic syncing of this file with plasma/CMakeLists.txt as it changes, but thats better than the alternative. (the only option really.)

find_package(OpenGL)

add_subdirectory( applets )
add_subdirectory( engines )
add_subdirectory( containments )

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}
                    ..
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
    plasma/animationdriver.cpp
    plasma/animator.cpp
    plasma/appletbrowser.cpp
    plasma/appletbrowser/customdragtreeview.cpp
    plasma/appletbrowser/kcategorizeditemsview.cpp
    plasma/appletbrowser/kcategorizeditemsviewdelegate.cpp
    plasma/appletbrowser/kcategorizeditemsviewmodels.cpp
    plasma/appletbrowser/openwidgetassistant.cpp
    plasma/appletbrowser/plasmaappletitemmodel.cpp
    plasma/applet.cpp
    plasma/applethandle.cpp
    plasma/configxml.cpp
    plasma/containment.cpp
    plasma/corona.cpp
    plasma/datacontainer.cpp
    plasma/dataengine.cpp
    plasma/dataenginemanager.cpp
    plasma/delegate.cpp
    plasma/desktoptoolbox.cpp
    plasma/dialog.cpp
    plasma/packages.cpp
    plasma/panelsvg.cpp
    plasma/paneltoolbox.cpp
    plasma/plasma.cpp
    plasma/plasma_export.h
    plasma/scripting/appletscript.cpp
    plasma/scripting/dataenginescript.cpp
    plasma/scripting/runnerscript.cpp
    plasma/scripting/scriptengine.cpp
    plasma/querymatch.cpp
    plasma/runnercontext.cpp
    plasma/runnermanager.cpp
    plasma/shadowitem.cpp
    plasma/svg.cpp
    plasma/theme.cpp
    plasma/toolbox.cpp
    plasma/uiloader.cpp
    plasma/view.cpp
    plasma/widgets/icon.cpp
    plasma/widgets/meter.cpp
    plasma/widgets/signalplotter.cpp
    plasma/widgets/webcontent.cpp
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
    ${QT_QTUITOOLS_LIBRARY}
    ${KDE4_KIO_LIBS}
    ${KDE4_KFILE_LIBS}
    ${KDE4_THREADWEAVER_LIBRARIES}
    ${QT_QTOPENGL_LIBRARY}
    ${OPENGL_gl_LIBRARY}
    ${OPENGL_glu_LIBRARY}
    ${QT_QTWEBKIT_LIBRARY}
)

if(APPLE)
	SET_TARGET_PROPERTIES(amarokplasma PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)

add_definitions(-DMAKE_PLASMA_LIB)
set_target_properties(amarokplasma PROPERTIES VERSION 1.0.0 SOVERSION 1)
install(TARGETS amarokplasma DESTINATION                ${LIB_INSTALL_DIR})

find_package(Plasma)
if(NOT PLASMA_FOUND)
install(FILES
    plasma/servicetypes/plasma-animator.desktop
    plasma/servicetypes/plasma-applet.desktop
    plasma/servicetypes/plasma-dataengine.desktop
    plasma/servicetypes/plasma-runner.desktop
    plasma/servicetypes/plasma-scriptengine.desktop
    DESTINATION ${SERVICETYPES_INSTALL_DIR})

set(default_animator_SRCS
    animators/default/defaultAnimator.cpp
   )

kde4_add_plugin(plasma_animator_default ${default_animator_SRCS})
target_link_libraries(plasma_animator_default ${KDE4_KDECORE_LIBS} amarokplasma)

install(TARGETS plasma_animator_default DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES animators/default/plasma-animator-default.desktop DESTINATION ${SERVICES_INSTALL_DIR} )

endif(NOT PLASMA_FOUND)

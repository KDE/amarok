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
    plasma/applet.cpp
    plasma/configloader.cpp
    plasma/containment.cpp
    plasma/context.cpp
    plasma/corona.cpp
    plasma/datacontainer.cpp
    plasma/dataengine.cpp
    plasma/dataenginemanager.cpp
    plasma/delegate.cpp
    plasma/dialog.cpp
    plasma/extender.cpp
    plasma/extenderitem.cpp
    plasma/framesvg.cpp
    plasma/paintutils.cpp
    plasma/plasma.cpp
    plasma/plasma_export.h
    plasma/popupapplet.cpp
    plasma/private/applethandle.cpp
    plasma/private/datacontainer_p.cpp
    plasma/private/desktoptoolbox.cpp
    plasma/private/extenderapplet.cpp
    plasma/private/nativetabbar.cpp
    plasma/private/packages.cpp
    plasma/private/paneltoolbox.cpp
    plasma/private/style.cpp
    plasma/private/toolbox.cpp
    plasma/private/tooltip.cpp
    plasma/private/windowpreview.cpp
    plasma/querymatch.cpp
    plasma/runnercontext.cpp
    plasma/runnermanager.cpp
    plasma/scripting/appletscript.cpp
    plasma/scripting/dataenginescript.cpp
    plasma/scripting/runnerscript.cpp
    plasma/scripting/scriptengine.cpp
    plasma/service.cpp
    plasma/servicejob.cpp
    plasma/svg.cpp
    plasma/theme.cpp
    plasma/tooltipmanager.cpp
    plasma/uiloader.cpp
    plasma/version.cpp
    plasma/view.cpp    
    plasma/wallpaper.cpp    
    plasma/widgets/checkbox.cpp
    plasma/widgets/combobox.cpp
    plasma/widgets/flashinglabel.cpp
    plasma/widgets/frame.cpp
    plasma/widgets/groupbox.cpp
    plasma/widgets/iconwidget.cpp
    plasma/widgets/label.cpp
    plasma/widgets/lineedit.cpp
    plasma/widgets/meter.cpp
    plasma/widgets/pushbutton.cpp
    plasma/widgets/radiobutton.cpp
    plasma/widgets/scrollbar.cpp
    plasma/widgets/signalplotter.cpp
    plasma/widgets/slider.cpp
    plasma/widgets/tabbar.cpp
    plasma/widgets/textedit.cpp
    plasma/widgets/webview.cpp
)

kde4_add_ui_files (
    plasma_LIB_SRCS
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
    ${QT_QTWEBKIT_LIBRARY}
    ${KDE4_KNEWSTUFF2_LIBS}
    ${KDE4_SOLID_LIBRARY}
    ${X11_LIBRARIES}
)

if(QT_QTOPENGL_FOUND AND OPENGL_FOUND)
    target_link_libraries(amarokplasma
        ${QT_QTOPENGL_LIBRARY}
        ${OPENGL_gl_LIBRARY}
    )
endif(QT_QTOPENGL_FOUND AND OPENGL_FOUND)

if(APPLE)
   SET_TARGET_PROPERTIES(amarokplasma PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)

set_target_properties(amarokplasma PROPERTIES VERSION 2.0.0 SOVERSION 2
    ${KDE4_DISABLE_PROPERTY_}LINK_INTERFACE_LIBRARIES "${QT_QTGUI_LIBRARY}"
)
SET_TARGET_PROPERTIES(amarokplasma PROPERTIES DEFINE_SYMBOL MAKE_PLASMA_LIB )
install(TARGETS amarokplasma ${INSTALL_TARGETS_DEFAULT_ARGS} )

find_package(Plasma)
if(NOT PLASMA_FOUND)
install(FILES
    plasma/servicetypes/plasma-animator.desktop
    plasma/servicetypes/plasma-applet.desktop
    plasma/servicetypes/plasma-dataengine.desktop
    plasma/servicetypes/plasma-runner.desktop
    plasma/servicetypes/plasma-scriptengine.desktop
    plasma/servicetypes/plasma-containment.desktop
    DESTINATION ${SERVICETYPES_INSTALL_DIR})

set(default_animator_SRCS
    animators/default/defaultAnimator.cpp
   )

kde4_add_plugin(plasma_animator_default ${default_animator_SRCS})
target_link_libraries(plasma_animator_default ${KDE4_KDECORE_LIBS} amarokplasma)

install(TARGETS plasma_animator_default DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES animators/default/plasma-animator-default.desktop DESTINATION ${SERVICES_INSTALL_DIR} )

endif(NOT PLASMA_FOUND)

########### next target ###############

set(amarokpud_LIB_SRCS
    popupdropper/PopupDropper.cpp
    popupdropper/PopupDropperAction.cpp
    popupdropper/PopupDropperItem.cpp
    popupdropper/PopupDropperView.cpp
)

QT4_AUTOMOC(
    popupdropper/PopupDropper.cpp
    popupdropper/PopupDropperAction.cpp
    popupdropper/PopupDropperItem.cpp
    popupdropper/PopupDropperView.cpp
)

ADD_LIBRARY(amarokpud SHARED ${amarokpud_LIB_SRCS})

SET_TARGET_PROPERTIES( amarokpud PROPERTIES DEFINE_SYMBOL MAKE_POPUPDROPPER_LIB )

target_link_libraries(amarokpud
    ${QT_QTCORE_LIBRARY}
    ${QT_QTGUI_LIBRARY}
    ${QT_QTSVG_LIBRARY}
)

set_target_properties(amarokpud PROPERTIES VERSION 1.0.0 SOVERSION 1)
install(TARGETS amarokpud ${INSTALL_TARGETS_DEFAULT_ARGS} )


find_package(KDE4 REQUIRED)
find_package(Nepomuk REQUIRED)
include (KDE4Defaults)
include(NepomukMacros)

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${KDEBASE_WORKSPACE_SOURCE_DIR}/libs ${CMAKE_CURRENT_SOURCE_DIR}/.. ${KDE4_INCLUDES})
if(QT_QTOPENGL_FOUND AND OPENGL_FOUND)
    include_directories(${OPENGL_INCLUDE_DIR})
endif(QT_QTOPENGL_FOUND AND OPENGL_FOUND)

add_subdirectory(tests)
add_definitions(-DKDE_DEFAULT_DEBUG_AREA=1209)

########### next target ###############

set(plasmagik_SRCS
    packagemetadata.cpp
    packagestructure.cpp
    package.cpp
    )

set(plasma_LIB_SRCS
    ${plasmagik_SRCS}
    abstractrunner.cpp
    animationdriver.cpp
    animator.cpp
    applet.cpp
    configloader.cpp
    containment.cpp
    context.cpp
    corona.cpp
    datacontainer.cpp
    dataengine.cpp
    dataenginemanager.cpp
    delegate.cpp
    dialog.cpp
    extender.cpp
    extenderitem.cpp
    paintutils.cpp
    framesvg.cpp
    plasma.cpp
    popupapplet.cpp
    private/applethandle.cpp
    private/datacontainer_p.cpp
    private/desktoptoolbox.cpp
    private/extenderapplet.cpp
    private/nativetabbar.cpp
    private/packages.cpp
    private/paneltoolbox.cpp
    private/style.cpp
    private/toolbox.cpp
    private/tooltip.cpp
    private/windowpreview.cpp
    querymatch.cpp
    runnercontext.cpp
    runnermanager.cpp
    scripting/appletscript.cpp
    scripting/dataenginescript.cpp
    scripting/runnerscript.cpp
    scripting/scriptengine.cpp
    service.cpp
    servicejob.cpp
    svg.cpp
    theme.cpp
    tooltipmanager.cpp
    uiloader.cpp
    version.cpp
    view.cpp
    wallpaper.cpp
    widgets/checkbox.cpp
    widgets/combobox.cpp
    widgets/flashinglabel.cpp
    widgets/frame.cpp
    widgets/groupbox.cpp
    widgets/iconwidget.cpp
    widgets/label.cpp
    widgets/lineedit.cpp
    widgets/meter.cpp
    widgets/pushbutton.cpp
    widgets/radiobutton.cpp
    widgets/scrollbar.cpp
    widgets/signalplotter.cpp
    widgets/slider.cpp
    widgets/svgwidget.cpp
    widgets/tabbar.cpp
    widgets/treeview.cpp
    widgets/textedit.cpp
    widgets/webview.cpp
)

#NEPOMUK_GENERATE_FROM_ONTOLOGY(
#   nwc.nrl
#   ${metadata_test_BINARY_DIR}
#   TEST_HEADERS
#   TEST_SOURCES
#   TEST_INCLUDES
#)

if(QT_QTOPENGL_FOUND AND OPENGL_FOUND)
MESSAGE(STATUS "Adding support for OpenGL applets to libplasma")
set(plasma_LIB_SRCS
    ${plasma_LIB_SRCS}
    glapplet.cpp)
endif(QT_QTOPENGL_FOUND AND OPENGL_FOUND)

kde4_add_library(plasma SHARED ${plasma_LIB_SRCS})

target_link_libraries(plasma ${KDE4_KIO_LIBS} ${KDE4_KFILE_LIBS} ${KDE4_KNEWSTUFF2_LIBS}
                             ${QT_QTUITOOLS_LIBRARY} ${QT_QTWEBKIT_LIBRARY}
                             ${KDE4_THREADWEAVER_LIBRARIES} ${KDE4_SOLID_LIBS} ${X11_LIBRARIES})
if(DL_LIBRARY)
target_link_libraries(plasma ${DL_LIBRARY})
endif(DL_LIBRARY)

if(QT_QTOPENGL_FOUND AND OPENGL_FOUND)
    target_link_libraries(plasma ${QT_QTOPENGL_LIBRARY} ${OPENGL_gl_LIBRARY})
endif(QT_QTOPENGL_FOUND AND OPENGL_FOUND)

set_target_properties(plasma PROPERTIES
                             VERSION 3.0.0
                             SOVERSION 3
                             ${KDE4_DISABLE_PROPERTY_}LINK_INTERFACE_LIBRARIES  "${KDE4_KDEUI_LIBS}"
                      )

install(TARGETS plasma ${INSTALL_TARGETS_DEFAULT_ARGS})


########### install files ###############

set(plasmagik_HEADERS
    packagemetadata.h
    packagestructure.h
    package.h
    )

install(FILES ${plasmagik_HEADERS} DESTINATION ${INCLUDE_INSTALL_DIR}/plasma/ COMPONENT Devel)

set(plasma_LIB_INCLUDES
    abstractrunner.h
    animationdriver.h
    animator.h
    applet.h
    configloader.h
    containment.h
    context.h
    corona.h
    datacontainer.h
    dataengine.h
    dataenginemanager.h
    delegate.h
    dialog.h
    extender.h
    extenderitem.h
    paintutils.h
    framesvg.h
    plasma.h
    plasma_export.h
    popupapplet.h
    querymatch.h
    runnercontext.h
    runnermanager.h
    service.h
    servicejob.h
    svg.h
    theme.h
    tooltipmanager.h
    uiloader.h
    tooltipmanager.h
    version.h
    view.h
    wallpaper.h)

if(QT_QTOPENGL_FOUND AND OPENGL_FOUND)
set(plasma_LIB_INCLUDES
    ${plasma_LIB_INCLUDES}
    glapplet.h)
endif(QT_QTOPENGL_FOUND AND OPENGL_FOUND)

install(FILES
        ${plasma_LIB_INCLUDES}
        DESTINATION ${INCLUDE_INSTALL_DIR}/plasma COMPONENT Devel)

install(FILES
    widgets/checkbox.h
    widgets/combobox.h
    widgets/flashinglabel.h
    widgets/frame.h
    widgets/groupbox.h
    widgets/iconwidget.h
    widgets/label.h
    widgets/lineedit.h
    widgets/meter.h
    widgets/pushbutton.h
    widgets/radiobutton.h
    widgets/scrollbar.h
    widgets/signalplotter.h
    widgets/slider.h
    widgets/svgwidget.h
    widgets/tabbar.h
    widgets/treeview.h
    widgets/textedit.h
    widgets/webview.h
    DESTINATION ${INCLUDE_INSTALL_DIR}/plasma/widgets COMPONENT Devel)

install(FILES
    scripting/appletscript.h
    scripting/dataenginescript.h
    scripting/runnerscript.h
    scripting/scriptengine.h
    DESTINATION ${INCLUDE_INSTALL_DIR}/plasma/scripting COMPONENT Devel)


install(FILES
includes/AbstractRunner
includes/AnimationDriver
includes/Animator
includes/Applet
includes/AppletScript
includes/CheckBox
includes/ComboBox
includes/ConfigLoader
includes/Containment
includes/Context
includes/Corona
includes/DataContainer
includes/DataEngine
includes/DataEngineManager
includes/DataEngineScript
includes/Delegate
includes/Dialog
includes/Extender
includes/ExtenderItem
includes/FlashingLabel
includes/Frame
includes/FrameSvg
includes/GroupBox
includes/IconWidget
includes/Label
includes/LineEdit
includes/Meter
includes/Package
includes/PackageMetadata
includes/PackageStructure
includes/PaintUtils
includes/Plasma
includes/PopupApplet
includes/PushButton
includes/QueryMatch
includes/RadioButton
includes/RunnerContext
includes/RunnerManager
includes/RunnerScript
includes/ScriptEngine
includes/ScrollBar
includes/Service
includes/ServiceJob
includes/SignalPlotter
includes/Slider
includes/Svg
includes/SvgWidget
includes/TabBar
includes/TextEdit
includes/ToolTipManager
includes/Theme
includes/TreeView
includes/UiLoader
includes/View
includes/Version
includes/Wallpaper
includes/WebView
DESTINATION ${INCLUDE_INSTALL_DIR}/KDE/Plasma COMPONENT Devel)

if(QT_QTOPENGL_FOUND AND OPENGL_FOUND)
   install(FILES
      includes/GLApplet
      DESTINATION ${INCLUDE_INSTALL_DIR}/KDE/Plasma COMPONENT Devel)
endif(QT_QTOPENGL_FOUND AND OPENGL_FOUND)

install(FILES
   servicetypes/plasma-animator.desktop
   servicetypes/plasma-applet.desktop
   servicetypes/plasma-containment.desktop
   servicetypes/plasma-dataengine.desktop
   servicetypes/plasma-packagestructure.desktop
   servicetypes/plasma-runner.desktop
   servicetypes/plasma-scriptengine.desktop
   servicetypes/plasma-wallpaper.desktop
   DESTINATION ${SERVICETYPES_INSTALL_DIR})

install(FILES
   servicetypes/plasma-applet-extenderapplet.desktop
   DESTINATION ${SERVICES_INSTALL_DIR})

install(FILES scripting/plasmoids.knsrc DESTINATION  ${CONFIG_INSTALL_DIR})


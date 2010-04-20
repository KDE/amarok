set(simple_javascript_engine_SRCS
    simplejavascriptapplet.cpp
    appletinterface.cpp
    uiloader.cpp
    qtgui/font.cpp
    qtgui/graphicsitem.cpp
    qtgui/linearlayout.cpp
    qtgui/painter.cpp
    qtgui/point.cpp
    qtgui/rect.cpp
    qtgui/size.cpp
    qtgui/timer.cpp
    qtgui/url.cpp
)

include_directories( ../../..  # PaletteHandler.h
	            ${PHONON_INCLUDES})


kde4_add_plugin(amarok_appletscript_simple_javascript
	        ${simple_javascript_engine_SRCS})

target_link_libraries(amarok_appletscript_simple_javascript
                       ${KDE4_KDECORE_LIBS}
                       ${KDE4_PLASMA_LIBS}
                       ${QT_QTSCRIPT_LIBRARY}
                       ${QT_QTUITOOLS_LIBRARY}
                       ${QT_QTXML_LIBRARY}
		               amarokcore
                       amaroklib
                       )
 
install(TARGETS amarok_appletscript_simple_javascript DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES amarok-scriptengine-applet-simple-javascript.desktop DESTINATION ${SERVICES_INSTALL_DIR} )

set(javascript_runner_engine_SRCS
    javascriptrunner.cpp 
)

kde4_add_plugin(amarok_runnerscript_javascript ${javascript_runner_engine_SRCS})

target_link_libraries(amarok_runnerscript_javascript
                       ${KDE4_KDECORE_LIBS}
                       ${KDE4_PLASMA_LIBS}
                       ${QT_QTSCRIPT_LIBRARY}
                       ${QT_QTUITOOLS_LIBRARY}
                       ${QT_QTXML_LIBRARY}
                       )

install(TARGETS amarok_runnerscript_javascript DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES amarok-scriptengine-runner-javascript.desktop DESTINATION ${SERVICES_INSTALL_DIR})

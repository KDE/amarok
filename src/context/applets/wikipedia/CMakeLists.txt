project(context-currenttrack)

set( wiki_SRCS WikipediaApplet.cpp )

include_directories(${Amarok_SOURCE_DIR}/src
                    ${Amarok_SOURCE_DIR}/src/context
                    ${Amarok_SOURCE_DIR}/src/network
                    )

ki18n_wrap_ui( wiki_SRCS wikipediaGeneralSettings.ui wikipediaLanguageSettings.ui )

add_library(amarok_context_applet_wikipedia MODULE ${wiki_SRCS})
if(APPLE)
   set_target_properties(amarok_context_applet_wikipedia PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif()
target_link_libraries(amarok_context_applet_wikipedia
    amarokcore
    amaroklib
    KF5::Plasma
    KF5::KIOCore
    ${KDE4_KDEWEBKIT_LIBS}
    Qt5::WebKitWidgets
)

install(TARGETS amarok_context_applet_wikipedia DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES amarok-context-applet-wikipedia.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES amarok-wikipedia.svg amarok-wikipediaheader.svg DESTINATION ${DATA_INSTALL_DIR}/desktoptheme/default/widgets/ )
install(FILES WikipediaCustomStyle.css bullet.gif DESTINATION ${DATA_INSTALL_DIR}/amarok/data )

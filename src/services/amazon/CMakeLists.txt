include_directories(
    ../
    ../../
    ../../core-impl/collections
    ../../core-impl/collections/support
    ../../statusbar
    ${CMAKE_CURRENT_BINARY_DIR}/../../..
    ${KDE4_INCLUDE_DIR}
    ${QT_INCLUDES} )

add_subdirectory( images )

########### next target ###############

set( amarok_service_amazonstore_PART_SRCS
    AmazonCollection.cpp
    AmazonConfig.cpp
    AmazonInfoParser.cpp
    AmazonItemTreeModel.cpp
    AmazonItemTreeView.cpp
    AmazonParser.cpp
    AmazonMeta.cpp
    AmazonShoppingCart.cpp
    AmazonShoppingCartDialog.cpp
    AmazonShoppingCartItem.cpp
    AmazonShoppingCartModel.cpp
    AmazonShoppingCartView.cpp
    AmazonStore.cpp
    AmazonUrlRunner.cpp
    AmazonWantCountryWidget.cpp
)

kde4_add_ui_files( amarok_service_amazonstore_PART_SRCS
    AmazonConfigWidget.ui
    AmazonShoppingCartDialog.ui
    AmazonWantCountryWidget.ui
)

kde4_add_plugin( amarok_service_amazonstore ${amarok_service_amazonstore_PART_SRCS} )

target_link_libraries( amarok_service_amazonstore
    amarokcore
    amaroklib
    amarokpud
    ${KDE4_KDECORE_LIBS}
    ${KDE4_KDEUI_LIBS}
    ${KDE4_KUTILS_LIBS}
    ${KDE4_KIO_LIBS}
    ${KDE4_THREADWEAVER_LIBRARIES}
)

install(TARGETS amarok_service_amazonstore DESTINATION ${PLUGIN_INSTALL_DIR} )

########### next target ###############

set(kcm_amarok_service_amazonstore_PART_SRCSS
    AmazonSettingsModule.cpp
    AmazonConfig.cpp
)

kde4_add_ui_files( kcm_amarok_service_amazonstore_PART_SRCSS AmazonConfigWidget.ui )

kde4_add_plugin( kcm_amarok_service_amazonstore ${kcm_amarok_service_amazonstore_PART_SRCSS} )

target_link_libraries( kcm_amarok_service_amazonstore ${KDE4_KUTILS_LIBS} ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${KDE4_KDECORE_LIBRARY} ${KDE4_KDEUI_LIBS} )

install(TARGETS kcm_amarok_service_amazonstore DESTINATION ${PLUGIN_INSTALL_DIR})

########### install files ###############

install( FILES amarok_service_amazonstore.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install( FILES amarok_service_amazonstore_config.desktop DESTINATION ${SERVICES_INSTALL_DIR})

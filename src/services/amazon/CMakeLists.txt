include_directories(
    ../
    ../../
    ../../core-impl/collections
    ../../core-impl/collections/support
    ../../statusbar
    ${CMAKE_CURRENT_BINARY_DIR}/../../..
    
     )

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

ki18n_wrap_ui( amarok_service_amazonstore_PART_SRCS
    AmazonConfigWidget.ui
    AmazonShoppingCartDialog.ui
    AmazonWantCountryWidget.ui
)

add_library(amarok_service_amazonstore MODULE ${amarok_service_amazonstore_PART_SRCS} )

target_link_libraries( amarok_service_amazonstore
    amarokcore
    amaroklib
    amarokpud
    KF5::KDELibs4Support
    
    
    KF5::KIOCore
    KF5::ThreadWeaver
)

install(TARGETS amarok_service_amazonstore DESTINATION ${PLUGIN_INSTALL_DIR} )

########### next target ###############

set(kcm_amarok_service_amazonstore_PART_SRCSS
    AmazonSettingsModule.cpp
    AmazonConfig.cpp
)

ki18n_wrap_ui( kcm_amarok_service_amazonstore_PART_SRCSS AmazonConfigWidget.ui )

add_library(kcm_amarok_service_amazonstore MODULE ${kcm_amarok_service_amazonstore_PART_SRCSS} )

target_link_libraries( kcm_amarok_service_amazonstore  Qt5::Core Qt5::Gui   )

install(TARGETS kcm_amarok_service_amazonstore DESTINATION ${PLUGIN_INSTALL_DIR})

########### install files ###############

install( FILES amarok_service_amazonstore.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install( FILES amarok_service_amazonstore_config.desktop DESTINATION ${SERVICES_INSTALL_DIR})

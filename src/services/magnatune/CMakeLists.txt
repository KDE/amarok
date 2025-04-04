include_directories(
    ../
    ../../
    ../../core-impl/collections
    ../../statusbar
    ${CMAKE_CURRENT_BINARY_DIR}/../../..
)

add_subdirectory( images )

########### next target ###############

set(amarok_service_magnatune_PART_SRCS
    MagnatuneActions.cpp
    MagnatuneAlbumDownloader.cpp
    MagnatuneCollectionLocation.cpp
    MagnatuneConfig.cpp
    MagnatuneDatabaseHandler.cpp
    MagnatuneDatabaseWorker.cpp
    MagnatuneDownloadDialog.cpp
    MagnatuneDownloadInfo.cpp
    MagnatuneInfoParser.cpp
    MagnatuneMeta.cpp
    MagnatuneNeedUpdateWidget.cpp
    MagnatuneDownloadHandler.cpp
    MagnatuneRedownloadDialog.cpp
    MagnatuneRedownloadHandler.cpp
    MagnatuneSqlCollection.cpp
    MagnatuneStore.cpp
    MagnatuneUrlRunner.cpp
    MagnatuneXmlParser.cpp

)

ki18n_wrap_ui( amarok_service_magnatune_PART_SRCS
    MagnatuneDownloadDialogBase.ui
    MagnatuneNeedUpdateWidget.ui
    MagnatuneRedownloadDialogBase.ui
    MagnatuneSignupDialogBase.ui
)

add_library(amarok_service_magnatunestore MODULE ${amarok_service_magnatune_PART_SRCS})

target_link_libraries(amarok_service_magnatunestore
    amarokcore
    amaroklib
    KF${KF_MAJOR_VERSION}::ConfigWidgets
    KF${KF_MAJOR_VERSION}::KIOCore
    KF${KF_MAJOR_VERSION}::ThreadWeaver
    Qt::Core
    Qt::Widgets
)

install(TARGETS amarok_service_magnatunestore DESTINATION ${KDE_INSTALL_PLUGINDIR} )

########### next target ###############

set(kcm_amarok_service_magnatune_PART_SRCSS
    MagnatuneSettingsModule.cpp
    MagnatuneConfig.cpp
)

ki18n_wrap_ui( kcm_amarok_service_magnatune_PART_SRCSS MagnatuneConfigWidget.ui )


add_library(kcm_amarok_service_magnatunestore MODULE ${kcm_amarok_service_magnatune_PART_SRCSS} )


target_link_libraries(kcm_amarok_service_magnatunestore
    amarokcore
    amaroklib
    KF${KF_MAJOR_VERSION}::ConfigWidgets
    KF${KF_MAJOR_VERSION}::KCMUtils
    KF${KF_MAJOR_VERSION}::ThreadWeaver
    Qt::Core
    Qt::Widgets
)

install(TARGETS kcm_amarok_service_magnatunestore DESTINATION ${KDE_INSTALL_PLUGINDIR})

########### install files ###############


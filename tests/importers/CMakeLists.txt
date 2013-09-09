add_custom_target( importers_files
    ${CMAKE_COMMAND} -E copy_directory "${AMAROK_TEST_TREE}/importers/files" "${CMAKE_BINARY_DIR}/tests/importers_files"
    VERBATIM
)

set( testimporterscommon_LIBS
    amaroklib
    amarokcore
    ${KDE4_KDEUI_LIBS}
    ${KDE4_KIO_LIBS}
    ${KDE4_THREADWEAVER_LIBRARIES}
    ${QT_QTSQL_LIBRARY}
    ${QT_QTTEST_LIBRARY}
    ${GOOGLEMOCK_LIBRARIES}
)

#------------------------ Test ImporterManager ----------------------------

set( testimportermanager_SRCS
    TestImporterManager.cpp
    ImporterMocks.cpp
)

kde4_add_unit_test( testimportermanager ${testimportermanager_SRCS} )
target_link_libraries( testimportermanager ${testimporterscommon_LIBS} )

#------------------------ Test ImporterProvider ---------------------------

set( testimporterprovider_SRCS
    TestImporterProvider.cpp
    ImporterMocks.cpp
)

kde4_add_unit_test( testimporterprovider ${testimporterprovider_SRCS} )
target_link_libraries( testimporterprovider ${testimporterscommon_LIBS} )

#------------------------ Test ImporterSqlProvider ------------------------

set( testimportersqlprovider_SRCS TestImporterSqlProvider.cpp )
kde4_add_unit_test( testimportersqlprovider ${testimportersqlprovider_SRCS} )
target_link_libraries( testimportersqlprovider ${testimporterscommon_LIBS} )

#------------------------ Test SimpleImporterConfigWidget -----------------

set( testsimpleimporterconfigwidget_SRCS TestSimpleImporterConfigWidget.cpp )
kde4_add_unit_test( testsimpleimporterconfigwidget
                    ${testsimpleimporterconfigwidget_SRCS} )
target_link_libraries( testsimpleimporterconfigwidget ${testimporterscommon_LIBS} )

#------------------------ Test AmarokImporter -----------------------------

set( testamarokimporter_SRCS
    TestImporterBase.cpp
    TestAmarokImporter.cpp
    ${AMAROK_SOURCE_TREE}/importers/amarok/AmarokConfigWidget.cpp
    ${AMAROK_SOURCE_TREE}/importers/amarok/AmarokManager.cpp
    ${AMAROK_SOURCE_TREE}/importers/amarok/AmarokProvider.cpp
    ${AMAROK_SOURCE_TREE}/importers/amarok/AmarokProviderEmbedded.cpp
    ${AMAROK_SOURCE_TREE}/importers/amarok/AmarokTrack.cpp
)
kde4_add_ui_files( testamarokimporter_SRCS
    ${AMAROK_SOURCE_TREE}/importers/amarok/AmarokConfigWidget.ui
)

kde4_add_unit_test( testamarokimporter ${testamarokimporter_SRCS} )
add_dependencies( testamarokimporter importers_files )
target_link_libraries( testamarokimporter
    amarok_importer-amarok
    ${testimporterscommon_LIBS}
)

#------------------------ Test BansheeImporter -------------------------------

set( testbansheeimporter_SRCS
    TestImporterBase.cpp
    TestBansheeImporter.cpp
    ${AMAROK_SOURCE_TREE}/importers/banshee/BansheeConfigWidget.cpp
    ${AMAROK_SOURCE_TREE}/importers/banshee/BansheeManager.cpp
    ${AMAROK_SOURCE_TREE}/importers/banshee/BansheeProvider.cpp
    ${AMAROK_SOURCE_TREE}/importers/banshee/BansheeTrack.cpp
)

kde4_add_unit_test( testbansheeimporter ${testbansheeimporter_SRCS} )
add_dependencies( testbansheeimporter importers_files )
target_link_libraries( testbansheeimporter
    amarok_importer-banshee
    ${testimporterscommon_LIBS}
)

#------------------------ Test ClementineImporter ----------------------------

set( testclementineimporter_SRCS
    TestImporterBase.cpp
    TestClementineImporter.cpp
    ${AMAROK_SOURCE_TREE}/importers/clementine/ClementineConfigWidget.cpp
    ${AMAROK_SOURCE_TREE}/importers/clementine/ClementineManager.cpp
    ${AMAROK_SOURCE_TREE}/importers/clementine/ClementineProvider.cpp
    ${AMAROK_SOURCE_TREE}/importers/clementine/ClementineTrack.cpp
)

kde4_add_unit_test( testclementineimporter ${testclementineimporter_SRCS} )
add_dependencies( testclementineimporter importers_files )
target_link_libraries( testclementineimporter
    amarok_importer-clementine
    ${testimporterscommon_LIBS}
)

#------------------------ Test FastForwardImporter ------------------------

set( testfastforwardimporter_SRCS
    TestImporterBase.cpp
    TestFastForwardImporter.cpp
    ${AMAROK_SOURCE_TREE}/importers/fastforward/FastForwardConfigWidget.cpp
    ${AMAROK_SOURCE_TREE}/importers/fastforward/FastForwardManager.cpp
    ${AMAROK_SOURCE_TREE}/importers/fastforward/FastForwardProvider.cpp
    ${AMAROK_SOURCE_TREE}/importers/fastforward/FastForwardTrack.cpp
)
kde4_add_ui_files( testfastforwardimporter_SRCS
    ${AMAROK_SOURCE_TREE}/importers/fastforward/FastForwardConfigWidget.ui
)

kde4_add_unit_test( testfastforwardimporter ${testfastforwardimporter_SRCS} )
add_dependencies( testfastforwardimporter importers_files )
target_link_libraries( testfastforwardimporter
    amarok_importer-fastforward
    ${testimporterscommon_LIBS}
)

#------------------------ Test ITunesImporter -----------------------------

set( testitunesimporter_SRCS
    TestImporterBase.cpp
    TestITunesImporter.cpp
    ${AMAROK_SOURCE_TREE}/importers/itunes/ITunesConfigWidget.cpp
    ${AMAROK_SOURCE_TREE}/importers/itunes/ITunesManager.cpp
    ${AMAROK_SOURCE_TREE}/importers/itunes/ITunesProvider.cpp
    ${AMAROK_SOURCE_TREE}/importers/itunes/ITunesTrack.cpp
)

kde4_add_unit_test( testitunesimporter ${testitunesimporter_SRCS} )
add_dependencies( testitunesimporter importers_files )
target_link_libraries( testitunesimporter
    amarok_importer-itunes
    ${testimporterscommon_LIBS}
)

#------------------------ Test RhythmboxImporter -----------------------------

set( testrhytmboximporter_SRCS
    TestImporterBase.cpp
    TestRhythmboxImporter.cpp
    ${AMAROK_SOURCE_TREE}/importers/rhythmbox/RhythmboxConfigWidget.cpp
    ${AMAROK_SOURCE_TREE}/importers/rhythmbox/RhythmboxManager.cpp
    ${AMAROK_SOURCE_TREE}/importers/rhythmbox/RhythmboxProvider.cpp
    ${AMAROK_SOURCE_TREE}/importers/rhythmbox/RhythmboxTrack.cpp
)

kde4_add_unit_test( testrhytmboximporter ${testrhytmboximporter_SRCS} )
add_dependencies( testrhytmboximporter importers_files )
target_link_libraries( testrhytmboximporter
    amarok_importer-rhythmbox
    ${testimporterscommon_LIBS}
)

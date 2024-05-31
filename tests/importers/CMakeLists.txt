add_custom_target( importers_files
    ${CMAKE_COMMAND} -E copy_directory "${AMAROK_TEST_TREE}/importers/files" "${CMAKE_BINARY_DIR}/tests/importers_files"
    VERBATIM
)

set( testimporterscommon_LIBS
    amaroklib
    amarokcore
    
    KF${KF_MAJOR_VERSION}::KIOCore
    KF${KF_MAJOR_VERSION}::ThreadWeaver
    Qt::Sql
    Qt::Test
    ${GOOGLEMOCK_LIBRARIES}
)

#------------------------ Test ImporterManager ----------------------------

set( testimportermanager_SRCS
    TestImporterManager.cpp
    ImporterMocks.cpp
    ${GOOGLEMOCK_SRCS}
)

add_executable( testimportermanager ${testimportermanager_SRCS} )
add_test(NAME testimportermanager COMMAND $<TARGET_FILE:testimportermanager>)
ecm_mark_as_test(testimportermanager)
target_link_libraries( testimportermanager ${testimporterscommon_LIBS} )

#------------------------ Test ImporterProvider ---------------------------

set( testimporterprovider_SRCS
    TestImporterProvider.cpp
    ImporterMocks.cpp
    ${GOOGLEMOCK_SRCS}
)

add_executable( testimporterprovider ${testimporterprovider_SRCS} )
add_test(NAME testimporterprovider COMMAND $<TARGET_FILE:testimporterprovider>)
ecm_mark_as_test(testimporterprovider)
target_link_libraries( testimporterprovider ${testimporterscommon_LIBS} )

#------------------------ Test SimpleImporterConfigWidget -----------------

set( testsimpleimporterconfigwidget_SRCS TestSimpleImporterConfigWidget.cpp )
add_executable( testsimpleimporterconfigwidget ${testsimpleimporterconfigwidget_SRCS} )
add_test(NAME testsimpleimporterconfigwidget COMMAND $<TARGET_FILE:testsimpleimporterconfigwidget>)
ecm_mark_as_test(testsimpleimporterconfigwidget)
target_link_libraries( testsimpleimporterconfigwidget ${testimporterscommon_LIBS} )

#------------------------ Test AmarokImporter -----------------------------

set( testamarokimporter_SRCS
    TestImporterBase.cpp
    TestAmarokImporter.cpp
    ${AMAROK_SOURCE_TREE}/importers/amarok/AmarokConfigWidget.cpp
    ${AMAROK_SOURCE_TREE}/importers/amarok/AmarokManager.cpp
    ${AMAROK_SOURCE_TREE}/importers/amarok/AmarokProvider.cpp
    ${AMAROK_SOURCE_TREE}/importers/amarok/AmarokEmbeddedSqlConnection.cpp
    ${AMAROK_SOURCE_TREE}/importers/amarok/AmarokTrack.cpp
)
ki18n_wrap_ui( testamarokimporter_SRCS
    ${AMAROK_SOURCE_TREE}/importers/amarok/AmarokConfigWidget.ui
)

add_executable( testamarokimporter ${testamarokimporter_SRCS} )
add_test(NAME testamarokimporter COMMAND $<TARGET_FILE:testamarokimporter>)
ecm_mark_as_test(testamarokimporter)
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

add_executable( testbansheeimporter ${testbansheeimporter_SRCS} )
add_test(NAME testbansheeimporter COMMAND $<TARGET_FILE:testbansheeimporter>)
ecm_mark_as_test(testbansheeimporter)
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

add_executable( testclementineimporter ${testclementineimporter_SRCS} )
add_test(NAME testclementineimporter COMMAND $<TARGET_FILE:testclementineimporter>)
ecm_mark_as_test(testclementineimporter)
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
ki18n_wrap_ui( testfastforwardimporter_SRCS
    ${AMAROK_SOURCE_TREE}/importers/fastforward/FastForwardConfigWidget.ui
)

add_executable( testfastforwardimporter ${testfastforwardimporter_SRCS} )
add_test(NAME testfastforwardimporter COMMAND $<TARGET_FILE:testfastforwardimporter>)
ecm_mark_as_test(testfastforwardimporter)
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

add_executable( testitunesimporter ${testitunesimporter_SRCS} )
add_test(NAME testitunesimporter COMMAND $<TARGET_FILE:testitunesimporter>)
ecm_mark_as_test(testitunesimporter)
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

add_executable( testrhytmboximporter ${testrhytmboximporter_SRCS} )
add_test(NAME testrhytmboximporter COMMAND $<TARGET_FILE:testrhytmboximporter>)
ecm_mark_as_test(testrhytmboximporter)
add_dependencies( testrhytmboximporter importers_files )
target_link_libraries( testrhytmboximporter
    amarok_importer-rhythmbox
    ${testimporterscommon_LIBS}
)

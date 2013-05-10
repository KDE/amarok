#------------------------ Test TrackLoader -----------------------------

kde4_add_unit_test( testtrackloader TestTrackLoader.cpp )

target_link_libraries( testtrackloader
    amarokcore
    amaroklib
    ${KDE4_KDECORE_LIBS}
    ${QT_QTTEST_LIBRARY}
    ${QT_QTCORE_LIBRARY}
    ${KDE4_THREADWEAVER_LIBRARIES}
)

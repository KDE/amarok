#------------------------ Test MetaCueTrack -----------------------------

set( testmetacuetrack_SRCS TestMetaCueTrack.cpp
    ${AMAROK_SOURCE_TREE}/core/meta/impl/cue/CueFileSupport.cpp
    ${AMAROK_SOURCE_TREE}/core/meta/impl/timecode/TimecodeMeta.cpp
    ${AMAROK_SOURCE_TREE}/core/capabilities/BoundedPlaybackCapability.cpp
    ${AMAROK_SOURCE_TREE}/core/capabilities/impl/timecode/TimecodeBoundedPlaybackCapability.cpp
    )
kde4_add_unit_test( testmetacuetrack ${testmetacuetrack_SRCS} )
target_link_libraries( testmetacuetrack ${QT_QTTEST_LIBRARY} amaroklib )

#------------------------ Test MetaCueCueFileItem -----------------------------

set( testmetacuecuefileitem_SRCS TestMetaCueCueFileItem.cpp )
kde4_add_unit_test( testmetacuecuefileitem ${testmetacuecuefileitem_SRCS} )
target_link_libraries( testmetacuecuefileitem ${QT_QTTEST_LIBRARY} ${KDE4_KDECORE_LIBS} )


#------------------------ Test CueFileSupport -----------------------------

include_directories(
  ${AMAROK_TEST_TREE}
  ${AMAROK_SOURCE_TREE}
  ${AMAROK_UTILITY_TREE}
  ${CMAKE_BINARY_DIR}/src
  ${CMAKE_BINARY_DIR}/tests
  ${AMAROK_COLLECTION_SUPPORT_DIR}
  ${KDE4_INCLUDE_DIR}
  ${QT_INCLUDES}
  ${GOOGLEMOCK_INCLUDE_DIR}
  )


set( testcuefilesupport_SRCS
        TestCueFileSupport.cpp
        ${AMAROK_SOURCE_TREE}/core/capabilities/BoundedPlaybackCapability.cpp
        ${AMAROK_SOURCE_TREE}/core/meta/impl/cue/CueFileSupport.cpp
        ${AMAROK_SOURCE_TREE}/core/meta/impl/timecode/TimecodeMeta.cpp
        ${AMAROK_SOURCE_TREE}/core/capabilities/impl/timecode/TimecodeBoundedPlaybackCapability.cpp
    )

kde4_add_unit_test( testcuefilesupport ${testcuefilesupport_SRCS} )

add_dependencies( testcuefilesupport amaroklib)

target_link_libraries( testcuefilesupport
    amaroklib
    ${KDE4_KDECORE_LIBS}
    ${KDE4_KDEUI_LIBS}
    ${QT_QTTEST_LIBRARY}
    ${QT_QTCORE_LIBRARY}
    ${GOOGLEMOCK_LIBRARIES}
    ${CMAKE_DL_LIBS})


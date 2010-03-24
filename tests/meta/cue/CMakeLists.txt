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

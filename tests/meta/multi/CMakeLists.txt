#------------------------ Test MetaMultiTrack -----------------------------

set( testmetamultitrack_SRCS TestMetaMultiTrack.cpp )
kde4_add_unit_test( testmetamultitrack
    ${testmetamultitrack_SRCS}
    ${AMAROK_SOURCE_TREE}/core/meta/impl/multi/MultiTrack.cpp
    ${AMAROK_SOURCE_TREE}/core/capabilities/MultiSourceCapability.cpp
    ${AMAROK_SOURCE_TREE}/core/capabilities/impl/multisource/MultiSourceCapabilityImpl.cpp
    )
target_link_libraries( testmetamultitrack ${QT_QTTEST_LIBRARY} amaroklib )

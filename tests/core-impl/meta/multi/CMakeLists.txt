include_directories(
  ${AMAROK_TEST_TREE}
  ${AMAROK_SOURCE_TREE}
  ${AMAROK_UTILITY_TREE}
  ${CMAKE_BINARY_DIR}/src
  ${CMAKE_BINARY_DIR}/tests
  ${AMAROK_COLLECTION_SUPPORT_DIR}
)

include_directories(SYSTEM
  ${GOOGLEMOCK_INCLUDE_DIR}
  )

#------------------------ Test MetaMultiTrack -----------------------------

set( testmetamultitrack_SRCS
    TestMetaMultiTrack.cpp
    ${AMAROK_SOURCE_TREE}/EngineController.cpp
    ${AMAROK_SOURCE_TREE}/core/capabilities/BoundedPlaybackCapability.cpp
    ${AMAROK_SOURCE_TREE}/core/capabilities/MultiSourceCapability.cpp
    ${AMAROK_SOURCE_TREE}/core-impl/meta/multi/MultiTrack.cpp
    ${AMAROK_SOURCE_TREE}/core-impl/capabilities/multisource/MultiSourceCapabilityImpl.cpp
    ${AMAROK_SOURCE_TREE}/playback/PowerManager.cpp
        ${GOOGLEMOCK_SRCS}
    )


add_executable( testmetamultitrack
    ${testmetamultitrack_SRCS}
    )
add_test(NAME testmetamultitrack COMMAND $<TARGET_FILE:testmetamultitrack>)
ecm_mark_as_test(testmetamultitrack)

add_dependencies( testmetamultitrack amarokcore )
add_dependencies( testmetamultitrack amaroklib )

target_link_libraries( testmetamultitrack
    amarokcore
    amaroklib
    KF5::KIOCore
    Phonon::phonon4qt5

    KF5::ThreadWeaver
    Qt::Test
    Qt::Core
    ${GOOGLEMOCK_LIBRARIES}
    ${CMAKE_DL_LIBS})

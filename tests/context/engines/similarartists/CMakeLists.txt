include_directories(
  ${AMAROK_SOURCE_TREE}/context/engines/similarartists
  ${AMAROK_SOURCE_TREE}/context/applets/similarartists
  ${AMAROK_SOURCE_TREE}/network
  ${AMAROK_TEST_TREE}/context/engines
  ${AMAROK_COLLECTION_SUPPORT_DIR}


)


set( testsimilarartistsengine_SRCS
    TestSimilarArtistsEngine.cpp
    ${AMAROK_TEST_TREE}/context/engines/ContextView.cpp
    ${AMAROK_TEST_TREE}/context/engines/TestDataEngine.cpp
    ${AMAROK_SOURCE_TREE}/context/ContextObserver.cpp
    ${AMAROK_SOURCE_TREE}/context/engines/similarartists/SimilarArtistsEngine.cpp
    ${AMAROK_SOURCE_TREE}/context/applets/similarartists/SimilarArtist.cpp
)

add_executable( testsimilarartistsengine ${testsimilarartistsengine_SRCS} )
add_test(NAME testsimilarartistsengine COMMAND $<TARGET_FILE:testsimilarartistsengine>)
ecm_mark_as_test(testsimilarartistsengine)

add_dependencies( testsimilarartistsengine amaroklib)

target_link_libraries( testsimilarartistsengine amarokcore amaroklib KF${KF_MAJOR_VERSION}::KIOCore KF${KF_MAJOR_VERSION}::Plasma ${LIBLASTFM_LIBRARY} Qt::Test)

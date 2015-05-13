include_directories( .
  ${AMAROK_TEST_TREE}
  ${CMAKE_BINARY_DIR}/tests
)

#------------------------ Test MetaAlbumKey -----------------------------

set( testmetaalbumkey_SRCS TestMetaAlbumKey.cpp )
add_executable( testmetaalbumkey ${testmetaalbumkey_SRCS} )
add_test(testmetaalbumkey testmetaalbumkey)
ecm_mark_as_test(testmetaalbumkey)
target_link_libraries( testmetaalbumkey KF5::KDELibs4Support Qt5::Test amarokcore amaroklib )

#------------------------ Test MetaTrackKey -----------------------------

set( testmetatrackkey_SRCS TestMetaTrackKey.cpp )
add_executable( testmetatrackkey ${testmetatrackkey_SRCS} )
add_test(testmetatrackkey testmetatrackkey)
ecm_mark_as_test(testmetatrackkey)
target_link_libraries( testmetatrackkey KF5::KDELibs4Support Qt5::Test amarokcore amaroklib )

#------------------------ Test PrivateMetaRegistry -----------------------------

set( testprivatemetaregistry_SRCS TestPrivateMetaRegistry.cpp )
add_executable( testprivatemetaregistry ${testprivatemetaregistry_SRCS} )
add_test(testprivatemetaregistry testprivatemetaregistry)
ecm_mark_as_test(testprivatemetaregistry)
target_link_libraries( testprivatemetaregistry KF5::KDELibs4Support Qt5::Test amarokcore amaroklib )

#------------------------ Test MetaConstants -----------------------------

set( testmetaconstants_SRCS TestMetaConstants.cpp ../../../../shared/FileType.cpp )
add_executable( testmetaconstants ${testmetaconstants_SRCS} )
add_test(testmetaconstants testmetaconstants)
ecm_mark_as_test(testmetaconstants)
target_link_libraries( testmetaconstants KF5::KDELibs4Support Qt5::Gui Qt5::Test amarokcore amaroklib )

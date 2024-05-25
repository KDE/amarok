include_directories( .
  ${AMAROK_TEST_TREE}
  ${CMAKE_BINARY_DIR}/tests
)

#------------------------ Test MetaAlbumKey -----------------------------

set( testmetaalbumkey_SRCS TestMetaAlbumKey.cpp )
add_executable( testmetaalbumkey ${testmetaalbumkey_SRCS} )
add_test(NAME testmetaalbumkey COMMAND $<TARGET_FILE:testmetaalbumkey>)
ecm_mark_as_test(testmetaalbumkey)
target_link_libraries( testmetaalbumkey Qt::Test amarokcore amaroklib )

#------------------------ Test MetaTrackKey -----------------------------

set( testmetatrackkey_SRCS TestMetaTrackKey.cpp )
add_executable( testmetatrackkey ${testmetatrackkey_SRCS} )
add_test(NAME testmetatrackkey COMMAND $<TARGET_FILE:testmetatrackkey>)
ecm_mark_as_test(testmetatrackkey)
target_link_libraries( testmetatrackkey Qt::Test amarokcore amaroklib )

#------------------------ Test PrivateMetaRegistry -----------------------------

set( testprivatemetaregistry_SRCS TestPrivateMetaRegistry.cpp )
add_executable( testprivatemetaregistry ${testprivatemetaregistry_SRCS} )
add_test(NAME testprivatemetaregistry COMMAND $<TARGET_FILE:testprivatemetaregistry>)
ecm_mark_as_test(testprivatemetaregistry)
target_link_libraries( testprivatemetaregistry Qt::Test amarokcore amaroklib )

#------------------------ Test MetaConstants -----------------------------

set( testmetaconstants_SRCS TestMetaConstants.cpp ../../../../shared/FileType.cpp )
add_executable( testmetaconstants ${testmetaconstants_SRCS} )
add_test(NAME testmetaconstants COMMAND $<TARGET_FILE:testmetaconstants>)
ecm_mark_as_test(testmetaconstants)
target_link_libraries( testmetaconstants Qt::Gui Qt::Test amarokcore amaroklib )

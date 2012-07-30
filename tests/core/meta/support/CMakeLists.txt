include_directories( .
  ${AMAROK_TEST_TREE}
  ${CMAKE_BINARY_DIR}/tests
)

#------------------------ Test MetaAlbumKey -----------------------------

set( testmetaalbumkey_SRCS TestMetaAlbumKey.cpp )
kde4_add_unit_test( testmetaalbumkey ${testmetaalbumkey_SRCS} )
target_link_libraries( testmetaalbumkey ${KDE4_KDECORE_LIBS} ${QT_QTTEST_LIBRARY} amarokcore amaroklib )

#------------------------ Test MetaTrackKey -----------------------------

set( testmetatrackkey_SRCS TestMetaTrackKey.cpp )
kde4_add_unit_test( testmetatrackkey ${testmetatrackkey_SRCS} )
target_link_libraries( testmetatrackkey ${KDE4_KDECORE_LIBS} ${QT_QTTEST_LIBRARY} amarokcore amaroklib )

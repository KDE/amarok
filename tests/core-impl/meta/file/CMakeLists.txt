#------------------------ Test MetaFileTrack -----------------------------

set( testmetafiletrack_SRCS TestMetaFileTrack.cpp )
kde4_add_unit_test( testmetafiletrack ${testmetafiletrack_SRCS} )
target_link_libraries( testmetafiletrack ${QT_QTTEST_LIBRARY} amaroklib )

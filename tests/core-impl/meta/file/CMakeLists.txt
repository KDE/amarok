#------------------------ Test MetaFileTrack -----------------------------

set( testmetafiletrack_SRCS TestMetaFileTrack.cpp )
add_executable( testmetafiletrack ${testmetafiletrack_SRCS} )
add_test(testmetafiletrack testmetafiletrack)
ecm_mark_as_test(testmetafiletrack)
target_link_libraries( testmetafiletrack Qt5::Test KF5::KDELibs4Support Qt5::Core amarokcore amaroklib )

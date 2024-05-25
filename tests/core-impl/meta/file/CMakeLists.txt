#------------------------ Test MetaFileTrack -----------------------------

set( testmetafiletrack_SRCS TestMetaFileTrack.cpp )
add_executable( testmetafiletrack ${testmetafiletrack_SRCS} )
add_test(NAME testmetafiletrack COMMAND $<TARGET_FILE:testmetafiletrack>)
ecm_mark_as_test(testmetafiletrack)
target_link_libraries( testmetafiletrack Qt::Test Qt::Core amarokcore amaroklib )

add_subdirectory( support )

#------------------------ Test MetaTrack -----------------------------

set( testmetatrack_SRCS TestMetaTrack.cpp )
add_executable( testmetatrack ${testmetatrack_SRCS} )
add_test(NAME testmetatrack COMMAND $<TARGET_FILE:testmetatrack>)
ecm_mark_as_test(testmetatrack)
target_link_libraries( testmetatrack Qt::Gui Qt::Test amarokcore amaroklib )

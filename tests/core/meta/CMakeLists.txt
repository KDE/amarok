add_subdirectory( support )

#------------------------ Test MetaTrack -----------------------------

set( testmetatrack_SRCS TestMetaTrack.cpp )
kde4_add_unit_test( testmetatrack ${testmetatrack_SRCS} )
target_link_libraries( testmetatrack ${KDE4_KDECORE_LIBS} ${QT_QTGUI_LIBRARY} ${QT_QTTEST_LIBRARY} amarokcore amaroklib )

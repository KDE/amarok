
add_subdirectory( void )
if(XINE_FOUND)
add_subdirectory( xine )
else(XINE_FOUND)
message(STATUS "${CMAKE_CURRENT_SOURCE_DIR}: skipped subdir $(XINE_ENGINE_SUBDIR)")
endif(XINE_FOUND)

message(STATUS "${CMAKE_CURRENT_SOURCE_DIR}: skipped subdir $(NMM_ENGINE_SUBDIR)")
message(STATUS "${CMAKE_CURRENT_SOURCE_DIR}: skipped subdir $(HELIX_ENGINE_SUBDIR)")
message(STATUS "${CMAKE_CURRENT_SOURCE_DIR}: skipped subdir $(GST10_ENGINE_SUBDIR)")
include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )


########### install files ###############




#original Makefile.am contents follow:

#if with_gst10
#    GST10_ENGINE_SUBDIR = gst10
#endif
#
#if with_nmm
#    NMM_ENGINE_SUBDIR = nmm
#endif
#
#if with_xine
#    XINE_ENGINE_SUBDIR = xine
#endif
#
#if with_helix
#    HELIX_ENGINE_SUBDIR = helix
#endif
#
#
#SUBDIRS = . \
#    void \
#    $(XINE_ENGINE_SUBDIR) \
#    $(NMM_ENGINE_SUBDIR) \
#    $(HELIX_ENGINE_SUBDIR) \
#    $(GST10_ENGINE_SUBDIR)
#

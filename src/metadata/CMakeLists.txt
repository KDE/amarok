ADD_SUBDIRECTORY( speex ) 
ADD_SUBDIRECTORY( wavpack ) 
ADD_SUBDIRECTORY( trueaudio ) 
ADD_SUBDIRECTORY( wma ) 
ADD_SUBDIRECTORY( audible ) 
ADD_SUBDIRECTORY( rmff ) 
ADD_SUBDIRECTORY( aac ) 
ADD_SUBDIRECTORY( wav ) 

MESSAGE(STATUS "${CMAKE_CURRENT_SOURCE_DIR}: skipped subdir $(MP4_SUBDIR)")
INCLUDE_DIRECTORIES( ${CMAKE_SOURCE_DIR}/amarok/src ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${KDE3_INCLUDE_DIR} ${QT_INCLUDE_DIR}  )


########### next target ###############

SET(metadata_STAT_SRCS
tplugins.cpp
)

kde4_automoc(${metadata_STAT_SRCS})

ADD_LIBRARY(metadata STATIC ${metadata_STAT_SRCS})


########### install files ###############




#original Makefile.am contents follow:

#if with_mp4v2
#MP4_SUBDIR = mp4
#MP4_LDADD = mp4/libtagmp4.la
#else
#MP4_SUBDIR = m4a
#MP4_LDADD = m4a/libtagm4a.la
#endif
#
#SUBDIRS = speex wavpack trueaudio wma audible rmff $(MP4_SUBDIR) aac wav
#
#
#INCLUDES           = -I$(top_srcdir)/amarok/src $(all_includes) $(taglib_includes)
#METASOURCES        = AUTO
#libmetadata_la_LDFLAGS  = $(all_libraries)
#noinst_LTLIBRARIES = libmetadata.la
#
#libmetadata_la_SOURCES =  \
#                     tplugins.cpp
#
#noinst_HEADERS = \
#                     tplugins.h
#
#libmetadata_la_LIBADD = \
#                     speex/libtagspeex.la \
#                     trueaudio/libtagtrueaudio.la \
#                     wavpack/libtagwavpack.la \
#                     wma/libtagwma.la \
#                     wav/libtagwav.la \
#                     rmff/libtagrealmedia.la \
#                     $(MP4_LDADD) \
#                     aac/libtagaac.la \
#                     audible/libtagaudible.la

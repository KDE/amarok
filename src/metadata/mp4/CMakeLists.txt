INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${KDE3_INCLUDE_DIR} ${QT_INCLUDE_DIR}  )


########### next target ###############

SET(tagmp4_STAT_SRCS
mp4properties.cpp
mp4tag.cpp
mp4file.cpp
taglib_mp4filetyperesolver.cpp
)

kde4_automoc(${tagmp4_STAT_SRCS})

ADD_LIBRARY(tagmp4 STATIC ${tagmp4_STAT_SRCS})


########### install files ###############




#original Makefile.am contents follow:

#SUBDIRS =
#
#INCLUDES           = $(all_includes) $(MP4V2_INCLUDES) $(taglib_includes)
#METASOURCES        = AUTO
#libtagmp4_la_LDFLAGS  = $(all_libraries) $(MP4V2_LIBS)
#noinst_LTLIBRARIES = libtagmp4.la
#
#libtagmp4_la_SOURCES =  \
#                     mp4properties.cpp \
#                     mp4tag.cpp \
#                     mp4file.cpp \
#                     taglib_mp4filetyperesolver.cpp
#
#noinst_HEADERS = \
#                     mp4properties.h \
#                     mp4tag.h \
#                     mp4file.h \
#                     taglib_mp4filetyperesolver.h

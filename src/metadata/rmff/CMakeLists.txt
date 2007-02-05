INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${KDE3_INCLUDE_DIR} ${QT_INCLUDE_DIR}  )


########### next target ###############

SET(tagrealmedia_STAT_SRCS
rmff.cpp
taglib_realmediafile.cpp
taglib_realmediafiletyperesolver.cpp
)

kde4_automoc(${tagrealmedia_STAT_SRCS})

ADD_LIBRARY(tagrealmedia STATIC ${tagrealmedia_STAT_SRCS})


########### install files ###############




#original Makefile.am contents follow:

#SUBDIRS =
#
#INCLUDES           = $(all_includes) $(taglib_includes)
#METASOURCES        = AUTO
#libtagrealmedia_la_LDFLAGS  = $(all_libraries)
#noinst_LTLIBRARIES = libtagrealmedia.la
#
#libtagrealmedia_la_SOURCES =  rmff.cpp \
#                     taglib_realmediafile.cpp \
#                     taglib_realmediafiletyperesolver.cpp
#
#noinst_HEADERS =     rmff.h \
#                     taglib_realmediafile.h \
#                     taglib_realmediafiletyperesolver.h
#

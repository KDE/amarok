INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${KDE3_INCLUDE_DIR} ${QT_INCLUDE_DIR}  )


########### next target ###############

SET(tagaudible_STAT_SRCS
audibleproperties.cpp
audibletag.cpp
taglib_audiblefile.cpp
taglib_audiblefiletyperesolver.cpp
)

kde4_automoc(${tagaudible_STAT_SRCS})

ADD_LIBRARY(tagaudible STATIC ${tagaudible_STAT_SRCS})


########### install files ###############




#original Makefile.am contents follow:

#SUBDIRS =
#
#INCLUDES           = $(all_includes) $(taglib_includes)
#METASOURCES        = AUTO
#libtagaudible_la_LDFLAGS  = $(all_libraries)
#noinst_LTLIBRARIES = libtagaudible.la
#
#libtagaudible_la_SOURCES =  audibleproperties.cpp \
#                     audibletag.cpp \
#                     taglib_audiblefile.cpp \
#                     taglib_audiblefiletyperesolver.cpp
#
#noinst_HEADERS =     audibleproperties.h \
#                     audibletag.h \
#                     taglib_audiblefile.h \
#                     taglib_audiblefiletyperesolver.h
#

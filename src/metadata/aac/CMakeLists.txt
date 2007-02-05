INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${KDE3_INCLUDE_DIR} ${QT_INCLUDE_DIR}  )


########### next target ###############

SET(tagaac_STAT_SRCS
aacfiletyperesolver.cpp
)

kde4_automoc(${tagaac_STAT_SRCS})

ADD_LIBRARY(tagaac STATIC ${tagaac_STAT_SRCS})


########### install files ###############




#original Makefile.am contents follow:

#SUBDIRS =
#METASOURCES        = AUTO
#INCLUDES = $(all_includes) $(taglib_includes)
#
#libtagaac_la_LDFLAGS  = $(all_libraries)
#noinst_LTLIBRARIES = libtagaac.la
#
#libtagaac_la_SOURCES = \
#                       aacfiletyperesolver.cpp
#
#noinst_HEADERS = \
#                         aacfiletyperesolver.h

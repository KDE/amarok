INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${KDE3_INCLUDE_DIR} ${QT_INCLUDE_DIR}  )


########### next target ###############

SET(tagwma_STAT_SRCS
wmaattribute.cpp
wmaproperties.cpp
wmatag.cpp
wmafile.cpp
taglib_wmafiletyperesolver.cpp
)

kde4_automoc(${tagwma_STAT_SRCS})

ADD_LIBRARY(tagwma STATIC ${tagwma_STAT_SRCS})


########### install files ###############




#original Makefile.am contents follow:

#SUBDIRS =
#
#INCLUDES           = $(all_includes) $(taglib_includes)
#METASOURCES        = AUTO
#libtagwma_la_LDFLAGS  = $(all_libraries)
#noinst_LTLIBRARIES = libtagwma.la
#
#libtagwma_la_SOURCES =  \
#                     wmaattribute.cpp \
#                     wmaproperties.cpp \
#                     wmatag.cpp \
#                     wmafile.cpp \
#                     taglib_wmafiletyperesolver.cpp
#
#noinst_HEADERS = \
#                     wmaattribute.h \
#                     wmaproperties.h \
#                     wmatag.h \
#                     wmafile.h \
#                     taglib_wmafiletyperesolver.h

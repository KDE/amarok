
include_directories( ${CMAKE_SOURCE_DIR}/amarok/src ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )


########### next target ###############

#set(amarokcollectionscanner_SRCS main.cpp collectionscanner.cpp )

#kde4_automoc(${amarokcollectionscanner_SRCS})

#kde4_add_executable(amarokcollectionscanner ${amarokcollectionscanner_SRCS})

#target_link_libraries(amarokcollectionscanner  ${KDE4_KDECORE_LIBS} metadata amarok )

#install_targets(/bin amarokcollectionscanner )


########### install files ###############




#original Makefile.am contents follow:

#bin_PROGRAMS = amarokcollectionscanner
#
#METASOURCES = AUTO
#
#INCLUDES = \
#    -I$(top_srcdir)/amarok/src \
#    $(taglib_includes) \
#    $(all_includes)
#
#amarokcollectionscanner_SOURCES = \
#    main.cpp \
#    collectionscanner.cpp
#
#amarokcollectionscanner_LDADD = \
#    $(top_builddir)/amarok/src/metadata/libmetadata.la \
#    $(top_builddir)/amarok/src/libamarok.la \
#    $(LIB_QT) \
#    $(LIB_KDECORE) \
#    $(taglib_libs)
#
#amarokcollectionscanner_LDFLAGS = \
#    $(all_libraries) \
#    $(KDE_RPATH)

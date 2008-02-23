bin_PROGRAMS = amarokcollectionscanner

METASOURCES = AUTO

INCLUDES = \
    -I$(top_srcdir)/amarok/src \
    $(TAGLIB_CFLAGS) \
    $(all_includes)

noinst_HEADERS = \
    collectionscannerdcophandler.h

amarokcollectionscanner_SOURCES = \
    main.cpp \
    collectionscannerdcopiface.skel \
    collectionscannerdcophandler.cpp \
    collectionscanner.cpp

amarokcollectionscanner_LDADD = \
    $(top_builddir)/amarok/src/metadata/libmetadata.la \
    $(top_builddir)/amarok/src/libamarok.la \
    $(LIB_QT) \
    $(LIB_KDECORE) \
    $(TAGLIB_LIBS)

amarokcollectionscanner_LDFLAGS = \
    $(all_libraries) \
    $(KDE_RPATH)

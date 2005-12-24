bin_PROGRAMS = amarokcollectionscanner

METASOURCES = AUTO

INCLUDES = \
    -I$(top_srcdir)/amarok/src \
    $(taglib_includes) \
    $(all_includes)

amarokcollectionscanner_SOURCES = \
    main.cpp \
    collectionscanner.cpp

amarokcollectionscanner_LDADD = \
    $(top_builddir)/amarok/src/metadata/libmetadata.la \
    $(top_builddir)/amarok/src/libamarok.la \
    $(LIB_QT) \
    $(LIB_KDECORE) \
    $(taglib_libs)

amarokcollectionscanner_LDFLAGS = \
    $(all_libraries) \
    $(KDE_RPATH)

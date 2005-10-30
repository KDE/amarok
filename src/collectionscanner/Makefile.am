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
    $(LIB_QT) \
    $(LIB_KDECORE) \
    $(taglib_libs)

amarokcollectionscanner_LDFLAGS = \
    $(all_libraries) \
    $(KDE_RPATH)

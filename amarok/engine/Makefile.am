INCLUDES = \
    -I$(top_builddir)/amarok/amarok/amarokarts \
    -I$(kde_includes)/arts \
    $(CFLAGS_GST) \
    $(CFLAGS_NMM) \
    $(all_includes)


#####################
# NMM_engine stuff
#####################
#if link_lib_NMM

#PLEASE FIXME!!
#this if construct doesn't work, I don't understand why! :-(
#NMM requries exceptions to compile..

KDE_CXXFLAGS = $(USE_EXCEPTIONS)
#endif
#####################

libengine_a_SOURCES = \
    enginebase.cpp \
    artsengine.cpp \
    gstengine.cpp \
    nmm_engine.cpp

noinst_HEADERS = \
    enginebase.h \
    artsengine.h \
    gstengine.h \
    nmm_engine.h

METASOURCES = AUTO

noinst_LIBRARIES =  libengine.a

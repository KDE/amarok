INCLUDES = \
    -I$(top_builddir)/amarok/amarok/amarokarts \
    -I$(kde_includes)/arts \
    $(CFLAGS_GST) \
    $(all_includes)
    
libengine_a_SOURCES = \
    enginebase.cpp \
    artsengine.cpp \
    gstengine.cpp

noinst_HEADERS = \
    enginebase.h \
    artsengine.h \
    gstengine.h

METASOURCES = AUTO

noinst_LIBRARIES =  libengine.a



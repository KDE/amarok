INCLUDES = \
    -I$(top_srcdir)/amarok/amarok \
    $(all_includes)

if with_arts
    ARTSENGINE_BUILD = arts
endif

if with_gst
    GSTENGINE_BUILD = gst
endif

if with_nmm
    NMMENGINE_BUILD = nmm
endif

libengine_a_SOURCES = \
    enginebase.cpp

noinst_HEADERS = \
    enginebase.h

METASOURCES = AUTO

noinst_LIBRARIES = libengine.a

SUBDIRS = . $(ARTSENGINE_BUILD) $(GSTENGINE_BUILD) $(NMMENGINE_BUILD)


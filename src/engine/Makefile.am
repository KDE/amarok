noinst_LTLIBRARIES = libengine.la

INCLUDES = \
    -I$(top_srcdir)/amarok/src/plugin \
    -I$(top_srcdir)/amarok/src \
    $(all_includes)

if with_gst10
    GST10_ENGINE_SUBDIR = gst10
endif

if with_nmm
    NMM_ENGINE_SUBDIR = nmm
endif

if with_xine
    XINE_ENGINE_SUBDIR = xine
endif

if with_akode
    AKODE_ENGINE_SUBDIR = akode
endif

if with_mas
    MAS_ENGINE_SUBDIR = mas
endif

if with_helix
    HELIX_ENGINE_SUBDIR = helix
endif

libengine_la_SOURCES = \
    enginebase.cpp

noinst_HEADERS = enginebase.h

METASOURCES = \
    AUTO

SUBDIRS = . \
    void \
    $(GST10_ENGINE_SUBDIR) \
    $(XINE_ENGINE_SUBDIR) \
    $(AKODE_ENGINE_SUBDIR) \
    $(MAS_ENGINE_SUBDIR) \
    $(NMM_ENGINE_SUBDIR) \
    $(HELIX_ENGINE_SUBDIR)


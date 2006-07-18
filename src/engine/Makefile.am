#if with_gst10
#    GST10_ENGINE_SUBDIR = gst10
#endif

if with_nmm
    NMM_ENGINE_SUBDIR = nmm
endif

if with_xine
    XINE_ENGINE_SUBDIR = xine
endif

if with_helix
    HELIX_ENGINE_SUBDIR = helix
endif


SUBDIRS = . \
    void \
    $(XINE_ENGINE_SUBDIR) \
    $(NMM_ENGINE_SUBDIR) \
    $(HELIX_ENGINE_SUBDIR)


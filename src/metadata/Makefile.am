if with_mp4v2
MP4_SUBDIR = mp4
MP4_LDADD = mp4/libtagmp4.la
else
MP4_SUBDIR = m4a
MP4_LDADD = m4a/libtagm4a.la
endif

SUBDIRS = speex wavpack trueaudio asf audible rmff $(MP4_SUBDIR) aac wav


INCLUDES           = -I$(top_srcdir)/amarok/src $(all_includes) $(taglib_includes)
METASOURCES        = AUTO
libmetadata_la_LDFLAGS  = $(all_libraries)
noinst_LTLIBRARIES = libmetadata.la

libmetadata_la_SOURCES =  \
                     tplugins.cpp

noinst_HEADERS = \
                     tplugins.h

libmetadata_la_LIBADD = \
                     speex/libtagspeex.la \
                     trueaudio/libtagtrueaudio.la \
                     wavpack/libtagwavpack.la \
                     asf/libtagasf.la \
                     wav/libtagwav.la \
                     rmff/libtagrealmedia.la \
                     $(MP4_LDADD) \
                     aac/libtagaac.la \
                     audible/libtagaudible.la

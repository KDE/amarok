if with_mp4v2
MP4_SUBDIR = mp4
MP4_LDADD = mp4/libtagmp4.la
endif

SUBDIRS = wma audible $(MP4_SUBDIR)


INCLUDES           = $(all_includes) $(taglib_includes)
METASOURCES        = AUTO
libmetadata_la_LDFLAGS  = $(all_libraries)
noinst_LTLIBRARIES = libmetadata.la

libmetadata_la_SOURCES =  \
                     tplugins.cpp

noinst_HEADERS = \
                     tplugins.h

libmetadata_la_LIBADD = \
                     wma/libtagwma.la \
                     $(MP4_LDADD) \
                     audible/libtagaudible.la

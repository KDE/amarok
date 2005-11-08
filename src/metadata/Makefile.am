SUBDIRS = wma mp4 audible

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
                     mp4/libtagmp4.la \
                     audible/libtagaudible.la

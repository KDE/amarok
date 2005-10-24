SUBDIRS =

INCLUDES           = $(all_includes) $(MP4V2_INCLUDES) $(taglib_includes)
METASOURCES        = AUTO
libmetadata_la_LDFLAGS  = $(all_libraries) $(MP4V2_LIBS)
noinst_LTLIBRARIES = libmetadata.la

libmetadata_la_SOURCES =  \
                     tplugins.cpp \
                     \
                     wma/asf.cpp \
                     wma/wmaproperties.cpp \
                     wma/wmatag.cpp \
                     wma/wmafile.cpp \
                     wma/taglib_wmafiletyperesolver.cpp \
                     \
                     mp4/mp4properties.cpp \
                     mp4/mp4tag.cpp \
                     mp4/taglib_mp4file.cpp \
                     mp4/taglib_mp4filetyperesolver.cpp

noinst_HEADERS = \
                     tplugins.h \
                     \
                     wma/asf.h \
                     wma/wmaproperties.h \
                     wma/wmatag.h \
                     wma/wmafile.h \
                     wma/taglib_wmafiletyperesolver.h \
                     \
                     mp4/mp4properties.h \
                     mp4/mp4tag.h \
                     mp4/taglib_mp4file.h \
                     mp4/taglib_mp4filetyperesolver.h

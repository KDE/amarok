SUBDIRS =

INCLUDES           = $(all_includes) $(MP4V2_INCLUDES) $(TAGLIB_CFLAGS)
METASOURCES        = AUTO
libtagmp4_la_LDFLAGS  = $(all_libraries) $(MP4V2_LIBS)
noinst_LTLIBRARIES = libtagmp4.la

libtagmp4_la_SOURCES =  \
                     mp4properties.cpp \
                     mp4tag.cpp \
                     mp4file.cpp \
                     taglib_mp4filetyperesolver.cpp

noinst_HEADERS = \
                     mp4properties.h \
                     mp4tag.h \
                     mp4file.h \
                     taglib_mp4filetyperesolver.h

plugin_LTLIBRARIES = \
    libgstkio.la

#location where plug-in will be installed
plugindir = \
    $(libdir)/gstreamer-@GST_MAJORMINOR@
#    /usr/local/lib/gstreamer-@GST_MAJORMINOR@
    
INCLUDES = \
    $(CFLAGS_GST) \
    $(all_includes)

libgstkio_la_SOURCES = \
    gstkiosrc.cpp
#    kioreceiver.cpp

libgstkio_la_LDFLAGS = \
    -module \
    -avoid-version \
    $(all_libraries) 

libgstkio_la_LIBADD = \
    $(LIB_QT) \
    $(LIB_KIO) \
    $(LIB_KDECORE)

AM_CXXFLAGS = \
    $(CFLAGS_GST) 

 METASOURCES = \
    AUTO
   
    

        
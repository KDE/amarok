bin_PROGRAMS = \
    amarok_xmmswrapper

INCLUDES = \
    $(xmms_cflags) \
    $(gtk_cflags ) \
    $(all_includes)

amarok_xmmswrapper_SOURCES = \
    xmmswrapper.cpp
              
amarok_xmmswrapper_LDADD = \
    $(LIB_QT) \
    $(LIB_KDECORE)
    
amarok_xmmswrapper_LDFLAGS = \
    $(xmms_libs) \
    $(gtk_libs) \
    $(KDE_RPATH) \
    $(all_libraries) 
    
    

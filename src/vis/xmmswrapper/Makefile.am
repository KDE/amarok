bin_PROGRAMS = \
    amarok_xmmswrapper

INCLUDES = \
    $(xmms_cflags) \
    $(all_includes)

amarok_xmmswrapper_SOURCES = \
    xmmswrapper.cpp
              
amarok_xmmswrapper_LDADD = \
    $(LIB_QT) \
    $(LIB_KDECORE)
    
amarok_xmmswrapper_LDFLAGS = \
    $(all_libraries) $(KDE_RPATH) \
    $(xmms_libs)


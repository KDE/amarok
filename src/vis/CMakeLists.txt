
message(STATUS "${CMAKE_CURRENT_SOURCE_DIR}: skipped subdir $(XMMSWRAPPER_BUILD)")
message(STATUS "${CMAKE_CURRENT_SOURCE_DIR}: skipped subdir $(LIBVISUAL_BUILD)")


########### install files ###############




#original Makefile.am contents follow:

#if with_xmmswrapper
#    XMMSWRAPPER_BUILD = xmmswrapper
#endif
#
#if with_libvisual
#    LIBVISUAL_BUILD = libvisual
#endif
#
#SUBDIRS = \
#    $(XMMSWRAPPER_BUILD) \
#    $(LIBVISUAL_BUILD)

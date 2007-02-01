
include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )


########### next target ###############


########### install files ###############

install_files( ${KCFG_INSTALL_DIR} FILES  helixconfig.kcfg )



#original Makefile.am contents follow:

#noinst_LTLIBRARIES = \
#    libhelixconfig.la
#
#libhelixconfig_la_SOURCES = \
#    dummy.cpp \
#    helixconfig.kcfgc
#    
#INCLUDES = \
#    $(all_includes)
#
#kde_kcfg_DATA = \
#    helixconfig.kcfg
#
#
#  


include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )


########### next target ###############


########### install files ###############

install_files( ${KCFG_INSTALL_DIR} FILES  gstconfig.kcfg )



#original Makefile.am contents follow:

#noinst_LTLIBRARIES = \
#    libgstconfig.la
#
#libgstconfig_la_SOURCES = \
#    dummy.cpp \
#    gstconfig.kcfgc
#    
#INCLUDES = \
#    $(all_includes)
#
#kde_kcfg_DATA = \
#    gstconfig.kcfg
#
#
#  
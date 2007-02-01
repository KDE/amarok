
include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )


########### install files ###############

install_files( ${DATA_INSTALL_DIR}/amarok/scripts/common FILES  Zeroconf.py Publisher.py )



#original Makefile.am contents follow:

#commondir = $(kde_datadir)/amarok/scripts/common
#
#common_DATA = Zeroconf.py Publisher.py
#
#

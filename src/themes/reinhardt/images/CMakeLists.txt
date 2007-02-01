
include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )


########### install files ###############

install_files( ${DATA_INSTALL_DIR}/amarok/themes/reinhardt/images FILES  background.png transparency.png )



#original Makefile.am contents follow:

#reinhardtimagesdir = $(kde_datadir)/amarok/themes/reinhardt/images
#
#reinhardtimages_DATA = background.png transparency.png

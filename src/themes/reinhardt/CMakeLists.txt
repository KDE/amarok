
add_subdirectory( images )

########### install files ###############

install_files( ${DATA_INSTALL_DIR}/amarok/themes/reinhardt FILES  stylesheet.css )



#original Makefile.am contents follow:

#SUBDIRS = images
#
#reinhardtdir = $(kde_datadir)/amarok/themes/reinhardt
#
#reinhardt_DATA = stylesheet.css


include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )


########### install files ###############

install_files( ${DATA_INSTALL_DIR}/amarok/scripts/score_impulsive FILES  score_impulsive.rb COPYING      README      score_impulsive.spec )



#original Makefile.am contents follow:

#scoreimpulsivedir = \
#    $(kde_datadir)/amarok/scripts/score_impulsive
#
#scoreimpulsive_SCRIPTS = \
#    score_impulsive.rb
#
#scoreimpulsive_DATA = \
#    COPYING \
#    README \
#    score_impulsive.spec


include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )


########### install files ###############

install_files( ${DATA_INSTALL_DIR}/amarok/scripts/score_default FILES  score_default.rb COPYING      README      score_default.spec )



#original Makefile.am contents follow:

#scoredefaultdir = \
#    $(kde_datadir)/amarok/scripts/score_default
#
#scoredefault_SCRIPTS = \
#    score_default.rb
#
#scoredefault_DATA = \
#    COPYING \
#    README \
#    score_default.spec

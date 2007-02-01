
include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )


########### install files ###############

install_files( ${DATA_INSTALL_DIR}/amarok/scripts/ruby_debug FILES  debug.rb )



#original Makefile.am contents follow:

#rubydebugdir = \
#    $(kde_datadir)/amarok/scripts/ruby_debug
#
#rubydebug_DATA = \
#    debug.rb

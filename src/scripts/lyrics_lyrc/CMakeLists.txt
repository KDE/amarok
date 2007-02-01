
include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )


########### install files ###############

install_files( ${DATA_INSTALL_DIR}/amarok/scripts/lyrics_lyrc FILES  lyrics_lyrc.rb COPYING      README      lyrics_lyrc.spec )



#original Makefile.am contents follow:

#lyricslyrcdir = \
#    $(kde_datadir)/amarok/scripts/lyrics_lyrc
#
#lyricslyrc_SCRIPTS = \
#    lyrics_lyrc.rb
#
#lyricslyrc_DATA = \
#    COPYING \
#    README \
#    lyrics_lyrc.spec

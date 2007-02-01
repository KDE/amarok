
include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )


########### install files ###############

install_files( ${DATA_INSTALL_DIR}/amarok/scripts/lyrics_astraweb FILES  COPYING      README      lyrics_astraweb.spec )

install_programs( ${DATA_INSTALL_DIR}/amarok/scripts/lyrics_astraweb FILES  lyrics_astraweb.rb )


#original Makefile.am contents follow:

#lyricsastrawebdir = \
#    $(kde_datadir)/amarok/scripts/lyrics_astraweb
#
#lyricsastraweb_SCRIPTS = \
#    lyrics_astraweb.rb
#
#lyricsastraweb_DATA = \
#    COPYING \
#    README \
#    lyrics_astraweb.spec

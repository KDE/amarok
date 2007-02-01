
include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )


########### install files ###############

install_files( ${DATA_INSTALL_DIR}/amarok/scripts/playlist2html FILES  playlist2html.py PlaylistServer.py Playlist.py README )



#original Makefile.am contents follow:

#playlistexporterdir = $(kde_datadir)/amarok/scripts/playlist2html
#
#playlistexporter_SCRIPTS = playlist2html.py PlaylistServer.py
#
#playlistexporter_DATA = Playlist.py README
#

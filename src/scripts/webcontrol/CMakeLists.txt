
include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )


########### install files ###############

install_files( ${DATA_INSTALL_DIR}/amarok/scripts/webcontrol FILES  Globals.py      Playlist.py      README      RequestHandler.py      WebControl.spec      WebPublisher.py      amarok_cut.png      controlbackground.png      main.css      main.js      player_end.png      player_pause.png      player_play.png      player_start.png      player_stop.png      template.thtml      vol_speaker.png      star.png      smallstar.png )

install_programs(${DATA_INSTALL_DIR}/amarok/scripts/webcontrol FILES WebControl.py)


#original Makefile.am contents follow:

#webcontroldir = \
#    $(kde_datadir)/amarok/scripts/webcontrol
#
#webcontrol_SCRIPTS = \
#    WebControl.py
#
#webcontrol_DATA = \
#    Globals.py \
#    Playlist.py \
#    README \
#    RequestHandler.py \
#    WebControl.spec \
#    WebPublisher.py \
#    amarok_cut.png \
#    controlbackground.png \
#    main.css \
#    main.js \
#    player_end.png \
#    player_pause.png \
#    player_play.png \
#    player_start.png \
#    player_stop.png \
#    template.thtml \
#    vol_speaker.png \
#    star.png \
#    smallstar.png

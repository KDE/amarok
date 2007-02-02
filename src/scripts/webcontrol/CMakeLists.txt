install( FILES  Globals.py      Playlist.py      README      RequestHandler.py      WebControl.spec      WebPublisher.py      amarok_cut.png      controlbackground.png      main.css      main.js      player_end.png      player_pause.png      player_play.png      player_start.png      player_stop.png      template.thtml      vol_speaker.png      star.png      smallstar.png DESTINATION ${DATA_INSTALL_DIR}/amarok/scripts/webcontrol)

install(PROGRAMS WebControl.py DESTINATION ${DATA_INSTALL_DIR}/amarok/scripts/webcontrol)

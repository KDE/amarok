#ifndef _UADE_XMMS_SLAVE_MSG_H_
#define _UADE_XMMS_SLAVE_MSG_H_

#include <stdlib.h>
#include <unistd.h>

struct uade_msgstruct {
  /* If this bit is set UADE will not write any sound data into shared mem */
  int dontwritebit;

  int touaemsgtype;

  char playername[256];
  char modulename[256];
  char scorename[256];

  int force_by_default;

  int set_subsong;
  int subsong;
  int min_subsong;
  int max_subsong;

  /* this is non-zero if emulator runs in ntsc state (not PAL) */
  int ntscbit;

  /* non-zero if filter emulation is used */
  int use_filter;

  /* it this is non-zero emulated deliplayer can force song end to occure,
     otherwise not */
  int songendpossible;
  /* if a song ends, this is set non-zero */
  int song_end;

  /* if this is non-zero uade_prerun() loads needed information from this
     structure and starts playing */
  int loadnewsongboolean;

  int plugin_pause_boolean;
  int sbuf_writeoffset;
  int sbuf_readoffset;

  /* pid of xmms plugin */
  pid_t masterpid;

  /* this is set 1 after uade executable has inited itself (and starts
     to wait commands in uade_prerun() */
  int uade_inited_boolean;

  char score_playername[256];
  char score_playerauthor[256];	// still useless
  char score_modulename[256];
  char score_formatname[256];

  char soundbuffer[32768];

};

#endif

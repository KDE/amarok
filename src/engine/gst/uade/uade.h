#ifndef _UADE_MAIN_H_
#define _UADE_MAIN_H_

#include "uademsg.h"
#include "playlist.h"

struct uade_song {
  char playername[PATH_MAX];       /* filename of eagleplayer */
  char modulename[PATH_MAX];       /* filename of song */
  char scorename[PATH_MAX];        /* filename of score file */

  int set_subsong;
  int subsong;
  int force_by_default;
  int use_ntsc;
  int song_end_possible;
  int use_filter;

  int min_subsong;
  int max_subsong;
  int cur_subsong;
};

struct uade_command {
  int type;
  void *par;
  void *ret;
};

struct uade_slave {
  /* called first. slave may parse options from argc/argv, and set
     specific variables in uade_song */
  int (*setup)(struct uade_song *uade_song, int argc, char **argv);

  int (*list_empty)(void);

  /* get_next waits for new song titles to be played, and returns the
     relevant information in 'uade_song'. in case of xmms plugin it
     must also signal the xmms to proceed with playing */
  int (*get_next)(struct uade_song *uade_song);

  /* this is called if an error happened after get_next() in uade_prerun()
   */
  void (*skip_to_next_song)(void);

  /* called after new song (and player) have been loaded into memory, and
     just before the song starts to play. */
  void (*post_init)(void);

  /* emulator sends sound data to the slave */
  void (*write)(void *sndbuffer, int bytes);

  /* uade calls this when song ends for any reason */
  void (*song_end)(struct uade_song *song, char *reason, int kill_it);

  /* flush sound buffers */
  void (*flush_sound)(void);

  /* synchronous interaction with the slave. slave becomes the master. */
  int (*interaction)(struct uade_command *cmd, int wait_for);

  /* slave reactions to messges sent by amiga software */
  void (*subsinfo)(struct uade_song *song, int mins, int maxs, int curs);
  void (*got_playername)(char *playername);
  void (*got_modulename)(char *modulename);
  void (*got_formatname)(char *formatname);

  int timeout;		/* default timeout infinite */
  int subsong_timeout;	/* default per subsong timeout infinite */
  int silence_timeout;	/* default silence timeout */
};

void uade_audxdat(int nr, unsigned int v);
void uade_audxlch(int nr, unsigned int v);
void uade_audxlcl(int nr, unsigned int v);
void uade_audxlen(int nr, unsigned int v);
void uade_audxper(int nr, unsigned int v);
void uade_audxvol(int nr, unsigned int v);
void uade_aud_dma(unsigned int v);
void uade_aud_strike(int nr, unsigned int v);

void uade_change_subsong(int subs);
void uade_get_amiga_message(void);
void uade_option(int, char**); /* handles command line parameters */
void uade_prerun(void);
void uade_send_amiga_message(int msgtype);
void uade_set_automatic_song_end(int song_end_possible);
void uade_set_ntsc(int usentsc);
void uade_song_end(char *reason, int kill_it);
void uade_swap_buffer_bytes(void *data, int bytes);
void uade_test_sound_block(void *buf, int size); /* for silence detection */
void uade_vsync_handler(void);
int uade_check_sound_buffers(void *sndbuffer, int sndbufsize, int bytes_per_sample);

void uade_print_help(int problemcode);

extern struct uade_slave slave;

extern int uade_local_sound;
extern char *uade_unix_sound_device;

extern int uade_using_outpipe;

extern int uade_swap_output_bytes;

extern int uade_reboot;

extern int uade_time_critical;

extern int uade_debug;
extern int uade_filter_debug; /* just for debugging */
extern int uade_filter_counter; /* just for debugging */
extern int uade_filter_overruns; /* just for debugging */

#endif

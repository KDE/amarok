//Maintainer: Max Howell <max.howell@methylblue.com>, (C) 2004
//Copyright:  See COPYING file that comes with this distribution


#ifndef XMMSWRAPPER_H
#define XMMSWRAPPER_H


//BEGIN fft.h

typedef short int sound_sample; //should be an signed 16 bit value

typedef struct _struct_fft_state fft_state;
fft_state *fft_init (void);
void fft_perform (const sound_sample *input, float *output, fft_state *state);
void fft_close (fft_state *state);

#define FFT_BUFFER_SIZE_LOG 9
#define FFT_BUFFER_SIZE (1 << FFT_BUFFER_SIZE_LOG)

//END fft.h


//BEGIN XmmsWrapper

#include <xmms/plugin.h> //VisPlugin struct
#include <string>
#include <gtk/gtk.h> //gint16, gtk_init(), gtk_rgb_init()

class XmmsWrapper
{
    VisPlugin *m_vis;

public:
    XmmsWrapper( const std::string& );
    ~XmmsWrapper();

    bool renderPCM() const { return vis()->render_pcm  && vis()->num_pcm_chs_wanted > 0; }
    bool renderFFT() const { return vis()->render_freq && vis()->num_freq_chs_wanted > 0; }

    void render( gint16 pcm_data[2][512] );

    VisPlugin *vis() { return m_vis; }
    const VisPlugin *vis() const { return m_vis; }
    void configure();
};

//END XmmsWrapper



//BEGIN required symbols

extern "C"
{
    //bloody ogl_scope requires this block of functions
    //libxmms/xmmsctrl.h
    gint xmms_connect_to_session(gint /*session*/) { return 0; }
    void xmms_remote_playlist(gint /*session*/, gchar ** /*list*/, gint /*num*/, gboolean /*enqueue*/) {}
    gint xmms_remote_get_version(gint /*session*/) { return 0; }
    void xmms_remote_playlist_add(gint /*session*/, GList * /*list*/) {}
    void xmms_remote_playlist_delete(gint /*session*/, gint /*pos*/) {}
    void xmms_remote_play(gint /*session*/) {}
    void xmms_remote_pause(gint /*session*/) {}
    void xmms_remote_stop(gint /*session*/) {}
    gboolean xmms_remote_is_playing(gint /*session*/) { return false; }
    gboolean xmms_remote_is_paused(gint /*session*/) { return false; }
    gint xmms_remote_get_playlist_pos(gint /*session*/) { return 0; }
    void xmms_remote_set_playlist_pos(gint /*session*/, gint /*pos*/) {}
    gint xmms_remote_get_playlist_length(gint /*session*/) { return 0; }
    void xmms_remote_playlist_clear(gint /*session*/) {}
    gint xmms_remote_get_output_time(gint /*session*/) { return 0; }
    void xmms_remote_jump_to_time(gint /*session*/, gint /*pos*/) {}
    void xmms_remote_get_volume(gint /*session*/, gint * /*vl*/, gint * /*vr*/) {}
    gint xmms_remote_get_main_volume(gint /*session*/) { return 0;}
    gint xmms_remote_get_balance(gint /*session*/) { return 0;}
    void xmms_remote_set_volume(gint /*session*/, gint /*vl*/, gint /*vr*/) {}
    void xmms_remote_set_main_volume(gint /*session*/, gint /*v*/) {}
    void xmms_remote_set_balance(gint /*session*/, gint /*b*/) {}
    gchar *xmms_remote_get_skin(gint /*session*/) { return 0; }
    void xmms_remote_set_skin(gint /*session*/, gchar * /*skinfile*/) {}
    gchar *xmms_remote_get_playlist_file(gint /*session*/, gint /*pos*/) { return 0; }
    gchar *xmms_remote_get_playlist_title(gint /*session*/, gint /*pos*/) { return 0; }
    gint xmms_remote_get_playlist_time(gint /*session*/, gint /*pos*/) { return 0; }
    void xmms_remote_get_info(gint /*session*/, gint * /*rate*/, gint * /*freq*/, gint * /*nch*/) {}
    void xmms_remote_main_win_toggle(gint /*session*/, gboolean /*show*/) {}
    void xmms_remote_pl_win_toggle(gint /*session*/, gboolean /*show*/) {}
    void xmms_remote_eq_win_toggle(gint /*session*/, gboolean /*show*/) {}
    gboolean xmms_remote_is_main_win(gint /*session*/) { return false; }
    gboolean xmms_remote_is_pl_win(gint /*session*/) { return false; }
    gboolean xmms_remote_is_eq_win(gint /*session*/) { return false; }
    void xmms_remote_show_prefs_box(gint /*session*/) {}
    void xmms_remote_toggle_aot(gint /*session*/, gboolean /*ontop*/) {}
    void xmms_remote_eject(gint /*session*/) {}
    void xmms_remote_playlist_prev(gint /*session*/) {}
    void xmms_remote_playlist_next(gint /*session*/) {}
    void xmms_remote_playlist_add_url_string(gint /*session*/, gchar * /*string*/) {}
    gboolean xmms_remote_is_running(gint /*session*/) { return false; }
    void xmms_remote_toggle_repeat(gint /*session*/) {}
    void xmms_remote_toggle_shuffle(gint /*session*/) {}
    gboolean xmms_remote_is_repeat(gint /*session*/) { return false; }
    gboolean xmms_remote_is_shuffle(gint /*session*/) { return false; }
    void xmms_remote_get_eq(gint /*session*/, gfloat */*preamp*/, gfloat **/*bands*/) {}
    gfloat xmms_remote_get_eq_preamp(gint /*session*/) { return 0;}
    gfloat xmms_remote_get_eq_band(gint /*session*/, gint /*band*/) { return 0;}
    void xmms_remote_set_eq(gint /*session*/, gfloat /*preamp*/, gfloat */*bands*/) {}
    void xmms_remote_set_eq_preamp(gint /*session*/, gfloat /*preamp*/) {}
    void xmms_remote_set_eq_band(gint /*session*/, gint /*band*/, gfloat /*value*/) {}
    /* Added in XMMS 1.2.1 */
    void xmms_remote_quit(gint /*session*/) {}
    /* Added in XMMS 1.2.6 */
    void xmms_remote_play_pause(gint /*session*/) {}
    void xmms_remote_playlist_ins_url_string(gint /*session*/, gchar * /*string*/, gint /*pos*/) {}

    //libxmms/util.h
    GtkWidget *xmms_show_message(gchar *, gchar *, gchar *, gboolean, GtkSignalFunc, gpointer) { return 0;}
    gboolean   xmms_check_realtime_priority(void) { return false; }
    void       xmms_usleep(gint usec) { usleep( usec ); }

    ConfigFile *xmms_cfg_new( void ) { return 0; }
    ConfigFile *xmms_cfg_open_file( gchar * ) { return 0; }
    gboolean xmms_cfg_write_file( ConfigFile *, gchar * ) { return 0;}
    void xmms_cfg_free( ConfigFile * ) {}
    ConfigFile *xmms_cfg_open_default_file( void ) { return 0;}
    gboolean xmms_cfg_write_default_file( ConfigFile * ) { return 0;}
    gboolean xmms_cfg_read_string( ConfigFile *, gchar *, gchar *, gchar ** ) { return 0;}
    gboolean xmms_cfg_read_int( ConfigFile *, gchar *, gchar *, gint * ) { return 0;}
    gboolean xmms_cfg_read_boolean( ConfigFile *, gchar *, gchar * , gboolean * ) { return 0;}
    gboolean xmms_cfg_read_float( ConfigFile *, gchar *, gchar *, gfloat * ) { return 0;}
    gboolean xmms_cfg_read_double( ConfigFile *, gchar *, gchar *, gdouble * ) { return 0; }
    void xmms_cfg_write_string( ConfigFile *, gchar *, gchar *, gchar * ) {}
    void xmms_cfg_write_int( ConfigFile *, gchar *, gchar *, gint ) {}
    void xmms_cfg_write_boolean( ConfigFile *, gchar *, gchar *, gboolean ) {}
    void xmms_cfg_write_float( ConfigFile *, gchar *, gchar *, gfloat ) {}
    void xmms_cfg_write_double( ConfigFile *, gchar *, gchar *, gdouble ) {}
    void xmms_cfg_remove_key( ConfigFile *, gchar *, gchar * ) {}
}

#include <gdk/gdkx.h> //Display Struct

extern "C" {
    //xmms/fullscreen.h
    gboolean xmms_fullscreen_available(Display*);
    gboolean xmms_fullscreen_init(GtkWidget *win);
    gboolean xmms_fullscreen_enter(GtkWidget *win, gint *w, gint *h);
    void     xmms_fullscreen_leave(GtkWidget *win);
    gboolean xmms_fullscreen_in(GtkWidget *win);
    gboolean xmms_fullscreen_mark(GtkWidget *win);
    void     xmms_fullscreen_unmark(GtkWidget *win);
    void     xmms_fullscreen_cleanup(GtkWidget *win);
    GSList  *xmms_fullscreen_modelist(GtkWidget *win);
    void     xmms_fullscreen_modelist_free(GSList *modes);


    //xmms/util.h
    gchar *find_file_recursively(const char*, const char*) { return 0; }
    void del_directory(const char *) {}
    GdkImage *create_dblsize_image(GdkImage *) { return 0; }
    char *read_ini_string(const char *, const char *, const char *) { return 0; }
    char *read_ini_string_no_comment(const char *, const char *, const char *) { return 0; }
    GArray *read_ini_array(const gchar *, const gchar *, const gchar *) { return 0; }
    GArray *string_to_garray(const gchar *) { return 0; }
    void glist_movedown(GList *) {}
    void glist_moveup(GList *) {}
    void util_item_factory_popup(GtkItemFactory *, guint, guint, guint, guint32) {}
    void util_item_factory_popup_with_data(GtkItemFactory *, gpointer, GtkDestroyNotify, guint, guint, guint, guint32) {}
    GtkWidget *util_create_add_url_window(gchar *, GtkSignalFunc, GtkSignalFunc) { return 0;}
    GtkWidget *util_create_filebrowser(gboolean) { return 0; }
    gboolean util_filebrowser_is_dir(GtkFileSelection *) { return false; }
    GdkFont *util_font_load(gchar *) { return 0; }
    void util_set_cursor(GtkWidget *) {}
    void util_dump_menu_rc(void) {}
    void util_read_menu_rc(void) {}
}

//END required symbols

#endif

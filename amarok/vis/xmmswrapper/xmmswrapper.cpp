//Maintainer: Max Howell <max.howell@methylblue.com>, (C) 2004
//Copyright:  See COPYING file that comes with this distribution

#include <dlfcn.h>   //dlopen etc.
#include <gtk/gtk.h> //gtk_init()
#include <iostream>
#include <stdlib.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <xmms/configfile.h> //visplugins use this stuff, see extern "C" block
#include <xmms/plugin.h>     //VisPlugin struct

//#define SHARED_LIB_EXT ".so"



//TODO pure c would give a smaller binary
//TODO keep socket open




#define FFT_BUFFER_SIZE_LOG 9
#define FFT_BUFFER_SIZE (1 << FFT_BUFFER_SIZE_LOG)

/* sound sample - should be an signed 16 bit value */
typedef short int sound_sample;

typedef struct _struct_fft_state fft_state;
fft_state *fft_init (void);
void fft_perform (const sound_sample *input, float *output, fft_state *state);
void fft_close (fft_state *state);

#include "fft.c" //FIXME tmp




class XmmsWrapper
{
    VisPlugin *m_vis;

public:
    XmmsWrapper( const std::string& );
    ~XmmsWrapper();

    bool renderPCM() const { return vis()->render_pcm  && vis()->num_pcm_chs_wanted > 0; }
    bool renderFFT() const { return vis()->render_freq && vis()->num_freq_chs_wanted > 0; }

    VisPlugin *vis() { return m_vis; }
    const VisPlugin *vis() const { return m_vis; }
};

int tryConnect();


std::string socketpath; //global


int
main( int argc, char** argv )
{
/*
    if( argc == 1 )
    {
        //scan plugins


        char *filename, *ext;
        DIR *dir;
        struct dirent *ent;
        struct stat statbuf;

        dir = opendir(dirname);
        if (!dir)
                return;

        while( (ent = readdir( path )) != NULL )
        {
                filename = g_strdup_printf("%s/%s", dirname, ent->d_name);
                if (!stat(filename, &statbuf) && S_ISREG(statbuf.st_mode) &&
                    (ext = strrchr(ent->d_name, '.')) != NULL)
                        if (!strcmp(ext, SHARED_LIB_EXT))
                                add_plugin(filename);
                g_free(filename);
        }
        closedir(dir);
    }
*/



    socketpath  = getenv( "HOME" );
    socketpath += "/.kde/share/apps/amarok"; //TODO get this with "kconfig --type data"
    socketpath += "/visualization_socket";

    gtk_init( &argc, &argv ); //xmms plugins require this
    gdk_rgb_init();
    XmmsWrapper wrap( argc > 1 ? argv[1] : "" );

    int    sockfd;
    int    nbytes = 0;
    const int nch = 1; //no of channels?

    while( (sockfd = tryConnect()) != -1 )
    {
        gtk_main_iteration_do( FALSE );
        usleep( 20 * 1000 );

        if( wrap.renderPCM() )
        {
            gint16 pcm_data[2][512];
            memset( pcm_data, 0, 1024 );

            send( sockfd, "PCM", 4, 0 );
            nbytes = recv( sockfd, pcm_data[0], 512, 0 );
            close( sockfd );

            wrap.vis()->render_pcm( pcm_data );
/*
            if (wrap.vis()->num_pcm_chs_wanted == 1)
            {
                gint16 mono_pcm[2][512];
                calc_mono_pcm(mono_pcm, pcm_data, nch);
                wrap.vis()->render_pcm( mono_pcm );
            }
            else //nch == 2
            {
                gint16 stereo_pcm[2][512];
                calc_stereo_pcm(stereo_pcm, pcm_data, nch);
                wrap.vis()->render_pcm( stereo_pcm );
            }
*/
        }

        if( wrap.renderFFT() ) //NOTE xmms has no else
        {
            gint16 fft_data[2][256];
            memset( fft_data, 0, 512 );

/*
            send( sockfd, "FFT", 4, 0 );
            nbytes = recv( sockfd, fft_data[0], 256, 0 );
            close( sockfd );
*/
        gint16 pcm_data[2][512];
        memset( pcm_data, 0, 1024 );

        send( sockfd, "PCM", 4, 0 );
        nbytes = recv( sockfd, pcm_data[0], 512, 0 );
        close( sockfd );

        static fft_state *state = NULL;
        gfloat tmp_out[257];
        gint i;

        if( !state ) state = fft_init();

        fft_perform( pcm_data[0], tmp_out, state );

        for(i = 0; i < 256; i++) fft_data[0][i] = ((gint)sqrt(tmp_out[i + 1])) >> 8;


            wrap.vis()->render_freq( fft_data );
/*
            if (wrap.vis()->num_freq_chs_wanted == 1)
            {
                gint16 mono_freq[2][256];
                calc_mono_freq(mono_freq, pcm_data, nch);
                wrap.vis()->render_freq(mono_freq);
            }
            else
            {
                gint16 stereo_freq[2][256];
                calc_stereo_freq(stereo_freq, pcm_data, nch);
                wrap.vis()->render_freq(stereo_freq);
            }
*/
        }
    }
}

int
tryConnect()
{
    //try to connect to the LoaderServer
    int fd = socket( AF_UNIX, SOCK_STREAM, 0 );

    if( fd != -1 )
    {

        struct sockaddr_un local;

        strcpy( &local.sun_path[0], socketpath.c_str() );
        local.sun_family = AF_UNIX;

        if( connect( fd, (struct sockaddr*) &local, sizeof( local ) ) == -1 )
        {
            close ( fd );
            fd = -1;
        }
    }

    return fd;
}


void vis_disable_plugin( VisPlugin *vp ) {} //seems to be a required function (see loadVis() )

extern "C"
{
    //bloody ogl_scope requires this block of functions
    //libxmms/xmmsctrl.h
    gint xmms_connect_to_session(gint session) { return 0; }
    void xmms_remote_playlist(gint session, gchar ** list, gint num, gboolean enqueue) {}
    gint xmms_remote_get_version(gint session) { return 0; }
    void xmms_remote_playlist_add(gint session, GList * list) {}
    void xmms_remote_playlist_delete(gint session, gint pos) {}
    void xmms_remote_play(gint session) {}
    void xmms_remote_pause(gint session) {}
    void xmms_remote_stop(gint session) {}
    gboolean xmms_remote_is_playing(gint session) { return false; }
    gboolean xmms_remote_is_paused(gint session) { return false; }
    gint xmms_remote_get_playlist_pos(gint session) { return 0; }
    void xmms_remote_set_playlist_pos(gint session, gint pos) {}
    gint xmms_remote_get_playlist_length(gint session) {}
    void xmms_remote_playlist_clear(gint session) {}
    gint xmms_remote_get_output_time(gint session) {}
    void xmms_remote_jump_to_time(gint session, gint pos) {}
    void xmms_remote_get_volume(gint session, gint * vl, gint * vr) {}
    gint xmms_remote_get_main_volume(gint session) {}
    gint xmms_remote_get_balance(gint session) {}
    void xmms_remote_set_volume(gint session, gint vl, gint vr) {}
    void xmms_remote_set_main_volume(gint session, gint v) {}
    void xmms_remote_set_balance(gint session, gint b) {}
    gchar *xmms_remote_get_skin(gint session) {}
    void xmms_remote_set_skin(gint session, gchar * skinfile) {}
    gchar *xmms_remote_get_playlist_file(gint session, gint pos) {}
    gchar *xmms_remote_get_playlist_title(gint session, gint pos) {}
    gint xmms_remote_get_playlist_time(gint session, gint pos) {}
    void xmms_remote_get_info(gint session, gint * rate, gint * freq, gint * nch) {}
    void xmms_remote_main_win_toggle(gint session, gboolean show) {}
    void xmms_remote_pl_win_toggle(gint session, gboolean show) {}
    void xmms_remote_eq_win_toggle(gint session, gboolean show) {}
    gboolean xmms_remote_is_main_win(gint session) {}
    gboolean xmms_remote_is_pl_win(gint session) {}
    gboolean xmms_remote_is_eq_win(gint session) {}
    void xmms_remote_show_prefs_box(gint session) {}
    void xmms_remote_toggle_aot(gint session, gboolean ontop) {}
    void xmms_remote_eject(gint session) {}
    void xmms_remote_playlist_prev(gint session) {}
    void xmms_remote_playlist_next(gint session) {}
    void xmms_remote_playlist_add_url_string(gint session, gchar * string) {}
    gboolean xmms_remote_is_running(gint session) {}
    void xmms_remote_toggle_repeat(gint session) {}
    void xmms_remote_toggle_shuffle(gint session) {}
    gboolean xmms_remote_is_repeat(gint session) {}
    gboolean xmms_remote_is_shuffle(gint session) {}
    void xmms_remote_get_eq(gint session, gfloat *preamp, gfloat **bands) {}
    gfloat xmms_remote_get_eq_preamp(gint session) {}
    gfloat xmms_remote_get_eq_band(gint session, gint band) {}
    void xmms_remote_set_eq(gint session, gfloat preamp, gfloat *bands) {}
    void xmms_remote_set_eq_preamp(gint session, gfloat preamp) {}
    void xmms_remote_set_eq_band(gint session, gint band, gfloat value) {}
    /* Added in XMMS 1.2.1 */
    void xmms_remote_quit(gint session) {}
    /* Added in XMMS 1.2.6 */
    void xmms_remote_play_pause(gint session) {}
    void xmms_remote_playlist_ins_url_string(gint session, gchar * string, gint pos) {}


    //these are the actual functions that vis plugins all use
    //xmms/pluginenum.h
    /*
    ConfigFile *xmms_cfg_new(void) {}
    ConfigFile *xmms_cfg_open_file(gchar * filename) { return 0; }
    gboolean    xmms_cfg_write_file(ConfigFile * cfg, gchar * filename) { return false; }
    void        xmms_cfg_free(ConfigFile * cfg) {}
    ConfigFile *xmms_cfg_open_default_file(void) { return 0; }
    gboolean    xmms_cfg_write_default_file(ConfigFile * cfg) { return false; }
    gboolean xmms_cfg_read_string(ConfigFile * cfg, gchar * section, gchar * key, gchar ** value) { return false; }
    gboolean xmms_cfg_read_int(ConfigFile * cfg, gchar * section, gchar * key, gint * value) { return false; }
    gboolean xmms_cfg_read_boolean(ConfigFile * cfg, gchar * section, gchar * key, gboolean * value) { return false; }
    gboolean xmms_cfg_read_float(ConfigFile * cfg, gchar * section, gchar * key, gfloat * value) { return false; }
    gboolean xmms_cfg_read_double(ConfigFile * cfg, gchar * section, gchar * key, gdouble * value) { return false; }
    void xmms_cfg_write_string(ConfigFile * cfg, gchar * section, gchar * key, gchar * value) {}
    void xmms_cfg_write_int(ConfigFile * cfg, gchar * section, gchar * key, gint value) {}
    void xmms_cfg_write_boolean(ConfigFile * cfg, gchar * section, gchar * key, gboolean value) {}
    void xmms_cfg_write_float(ConfigFile * cfg, gchar * section, gchar * key, gfloat value) {}
    void xmms_cfg_write_double(ConfigFile * cfg, gchar * section, gchar * key, gdouble value) {}
    void xmms_cfg_remove_key(ConfigFile * cfg, gchar * section, gchar * key) {}
    */

    //libxmms/util.h
    GtkWidget *xmms_show_message(gchar * title, gchar * text, gchar * button_text, gboolean modal, GtkSignalFunc button_action, gpointer action_data) {}
    gboolean   xmms_check_realtime_priority(void) { return false; }
    void       xmms_usleep(gint usec) { usleep( usec ); }


}

#include <gdk/gdkx.h> //Display Struct

extern "C" {
    //xmms/fullscreen.h
    gboolean xmms_fullscreen_available(Display *) { return false; }
    gboolean xmms_fullscreen_init(GtkWidget *win) { return false; }
    gboolean xmms_fullscreen_enter(GtkWidget *win, gint *w, gint *h) { return false; }
    void     xmms_fullscreen_leave(GtkWidget *win) {}
    gboolean xmms_fullscreen_in(GtkWidget *win) { return false; }
    gboolean xmms_fullscreen_mark(GtkWidget *win) { return false; }
    void     xmms_fullscreen_unmark(GtkWidget *win) {}
    void     xmms_fullscreen_cleanup(GtkWidget *win) {}
    GSList  *xmms_fullscreen_modelist(GtkWidget *win) { return 0; }
    void     xmms_fullscreen_modelist_free(GSList *modes) {}


    //xmms/util.h
    gchar *find_file_recursively(const char *dirname, const char *file) {}
    void del_directory(const char *dirname) {}
    GdkImage *create_dblsize_image(GdkImage * img) {}
    char *read_ini_string(const char *filename, const char *section, const char *key) {}
    char *read_ini_string_no_comment(const char *filename, const char *section, const char *key) {}
    GArray *read_ini_array(const gchar * filename, const gchar * section, const gchar * key) {}
    GArray *string_to_garray(const gchar * str) {}
    void glist_movedown(GList * list) {}
    void glist_moveup(GList * list) {}
    void util_item_factory_popup(GtkItemFactory * ifactory, guint x, guint y, guint mouse_button, guint32 time) {}
    void util_item_factory_popup_with_data(GtkItemFactory * ifactory, gpointer data, GtkDestroyNotify destroy, guint x, guint y, guint mouse_button, guint32 time) {}
    GtkWidget *util_create_add_url_window(gchar *caption, GtkSignalFunc ok_func, GtkSignalFunc enqueue_func) {}
    GtkWidget *util_create_filebrowser(gboolean clear_pl_on_ok) {}
    gboolean util_filebrowser_is_dir(GtkFileSelection * filesel) {}
    GdkFont *util_font_load(gchar *name) {}
    void util_set_cursor(GtkWidget *window) {}
    void util_dump_menu_rc(void) {}
    void util_read_menu_rc(void) {}
}




XmmsWrapper::XmmsWrapper( const std::string &plugin )
{
    std::string path = plugin;

    //const char *filename = "/usr/lib/xmms/Visualization/libbscope.so";
    //const char *filename = "/usr/lib/xmms/Visualization/libfinespectrum.so";
    //const char *filename = "/usr/lib/xmms/Visualization/libogl_spectrum.so";
    //const char *filename = "/usr/lib/xmms/Visualization/libsanalyzer.so";
    //const char *filename = "/usr/lib/xmms/Visualization/liblava.so";
    //const char *filename = "/usr/lib/xmms/Visualization/libbumpscope.so";
    //const char *filename = "/usr/lib/xmms/Visualization/libbezier.so";

    if( path.empty() ) path = "libblursk.so";

    path.insert( 0, "/usr/lib/xmms/Visualization/" );

    std::cout << "[amK] loading xmms plugin: " << path << '\n';

  /*{
        //scan plugins

        char *filename, *ext;
        DIR *dir;
        struct dirent *ent;
        struct stat statbuf;

        dir = opendir(dirname);
        if (!dir)
                return;

        while ((ent = readdir(dir)) != NULL)
        {
                filename = g_strdup_printf("%s/%s", dirname, ent->d_name);
                if (!stat(filename, &statbuf) && S_ISREG(statbuf.st_mode) &&
                    (ext = strrchr(ent->d_name, '.')) != NULL)
                        if (!strcmp(ext, SHARED_LIB_EXT))
                                add_plugin(filename);
                g_free(filename);
        }
        closedir(dir);
    }*/

    {
        //add plugin

        void *h;
        void *(*gpi) (void);

        if ((h = dlopen( path.c_str(), RTLD_NOW )) == NULL)
        {
                std::cout << dlerror() << "\n";
                return;
        }

        if ((gpi = (void*(*)())dlsym(h, "get_vplugin_info")) != NULL)
        {
                VisPlugin *p = (VisPlugin*)gpi();
                p->handle = h;
                p->filename = strdup( path.c_str() );
                p->xmms_session = 0;//ctrlsocket_get_session_id(); //FIXME
                p->disable_plugin = vis_disable_plugin; //FIXME what is this symbol?

                m_vis = p;
        }
        else { dlclose(h); return; }
    }

    if(m_vis->init) { std::cout << "[amK] init()\n"; m_vis->init(); }
    if(m_vis->playback_start)  { std::cout << "[amK] start()\n"; m_vis->playback_start(); }
}


XmmsWrapper::~XmmsWrapper()
{
    dlclose( m_vis );

    std::cout << "[amK] ~\n";
}






//NOTE as yet, these functions are a little mysterious to me

static void calc_stereo_pcm(gint16 dest[2][512], gint16 src[2][512], gint nch)
{
        memcpy(dest[0], src[0], 512 * sizeof(gint16));
        if(nch == 1)
                memcpy(dest[1], src[0], 512 * sizeof(gint16));
        else
                memcpy(dest[1], src[1], 512 * sizeof(gint16));
}

static void calc_mono_pcm(gint16 dest[2][512], gint16 src[2][512], gint nch)
{
        gint i;
        gint16 *d, *sl, *sr;

        if(nch == 1)
                memcpy(dest[0], src[0], 512 * sizeof(gint16));
        else
        {
                d = dest[0];
                sl = src[0];
                sr = src[1];
                for(i = 0; i < 512; i++)
                {
                        *(d++) = (*(sl++) + *(sr++)) >> 1;
                }
        }
}

static void calc_freq(gint16 *dest, gint16 *src)
{
/* FIXME
        static fft_state *state = NULL;
        gfloat tmp_out[257];
        gint i;

        if(!state)
                state = fft_init();

        fft_perform(src,tmp_out,state);

        for(i = 0; i < 256; i++)
                dest[i] = ((gint)sqrt(tmp_out[i + 1])) >> 8;
*/
}

static void calc_mono_freq(gint16 dest[2][256], gint16 src[2][512], gint nch)
{
/* FIXME
        gint i;
        gint16 *d, *sl, *sr, tmp[512];

        if(nch == 1)
                calc_freq(dest[0], src[0]);
        else
        {
                d = tmp;
                sl = src[0];
                sr = src[1];
                for(i = 0; i < 512; i++)
                {
                        *(d++) = (*(sl++) + *(sr++)) >> 1;
                }
                calc_freq(dest[0], tmp);
        }
*/
}

static void calc_stereo_freq(gint16 dest[2][256], gint16 src[2][512], gint nch)
{
/*
FIXME
        calc_freq(dest[0], src[0]);

        if(nch == 2)
                calc_freq(dest[1], src[1]);
        else
                memcpy(dest[1], dest[0], 256 * sizeof(gint16));
*/
}

//Maintainer: Max Howell <max.howell@methylblue.com>, (C) 2004
//Copyright:  See COPYING file that comes with this distribution


#include "config.h"

#include <dirent.h>
#include <dlfcn.h>    //dlopen etc.
#include <gtk/gtk.h>  //gtk_init(), gtk_rgb_init()
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <xmms/configfile.h> //visplugins use this stuff, see extern "C" block
#include "xmmswrapper.h"

#include <iostream>
#include <list> 
#include <vector>

#define SHARED_LIB_EXT ".so"

//TODO pure c would give a smaller binary <markey> get real, max! :)
//TODO keep socket open

#include "fft.c"

// This increases our little wrapper a lot, but unless Max implements
// auto search for proper place of visualization_socket, I'll do it KDE way.
//    -- berkus
#include <qstring.h>
#include <kinstance.h>
#include <kstandarddirs.h>


GtkWidget dummy;
GtkWidget *equalizerwin = &dummy; //required by msa visplugin
GtkWidget *playlistwin = &dummy; //required by msa visplugin
GtkWidget *mainwin = &dummy; //required by msa visplugin


int tryConnect();
void vis_disable_plugin( VisPlugin *vp ) {} //seems to be a required function (see loadVis() )

QString socketpath; //global
KInstance *inst;

int
main( int argc, char** argv )
{
    //TODO fork after connection?

    inst = new KInstance( "xmmswrapper" );
    std::string plugin;

    if ( argc == 1 ) {
        std::list<std::string> list;

        //scan plugins
        std::string dirname = XMMS_PLUGIN_PATH;
        dirname.append( "/" );
        DIR *dir;
        struct dirent *ent;
        struct stat statbuf;

        dir = opendir( dirname.c_str() );
        if ( !dir ) { std::cerr << "Please edit the PLUGIN_PATH in xmmswrapper.cpp\n"; exit( 1 ); }

        while ( ent = readdir( dir ) ) {
            std::string filename = ent->d_name;
            int index = filename.find_last_of( '.' );
            if ( index == std::string::npos ) continue;
            std::string extension = filename.substr( index );
            std::string fullpath = dirname + filename;

            if ( !stat( fullpath.c_str(), &statbuf )
                    && S_ISREG( statbuf.st_mode )
                    && extension == SHARED_LIB_EXT )
                list.push_back( filename );
        }
        closedir( dir );

        std::cout << "Please select a plugin: \n";

        std::vector<std::string> v( list.size() );
        std::copy( list.begin(), list.end(), v.begin() );

        for ( uint n = 0; n < v.size(); ++n ) {
            std::cout << n + 1 << ": " << v[ n ] << '\n';
        }
        char c[ 10 ];
        std::cin >> c;
        uint selection = atoi( c );
        if ( selection > v.size() ) return 1;

        plugin = v[ selection - 1 ];
    } else
        plugin = argv[ 1 ];

    socketpath = ::locateLocal( "socket", "amarok.visualization_socket", inst );

    gtk_init( &argc, &argv ); //xmms plugins require this
    gdk_rgb_init();

    XmmsWrapper wrap( plugin );

    int sockfd;
    int nbytes = 0;
    const int nch = 1; //no of channels?

    sockfd = tryConnect();

    while ( ( sockfd ) != -1 ) {
        gtk_main_iteration_do( FALSE );
        usleep( 20 * 1000 );

        if ( wrap.renderPCM() ) {
            float float_data[ 512 ];
            gint16 pcm_data[ 2 ][ 512 ];
            memset( pcm_data, 0, 1024 );

            send( sockfd, "PCM", 4, 0 );
            nbytes = recv( sockfd, float_data, 512 * sizeof( float ), 0 );

            //NOTE we times by 1<<14 rather than 1<<15 (maximum value of signed 16bit)
            //     this is because values of pcm data tend to range 0-2 (although there
            //     is no theoretical maximum.

            for ( uint x = 0; x < 512; ++x ) {
                pcm_data[ 0 ][ x ] = gint16( float_data[ x ] * ( 1 << 14 ) );
            }

            wrap.vis() ->render_pcm( pcm_data );
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

        if ( wrap.renderFFT() )   //NOTE xmms has no else
        {
            //TODO check float and gfloat are the same thing!
            //TODO do second channel as mirror for the moment

            float float_data[ 512 ];
            gint16 pcm_data[ 2 ][ 512 ];
            gint16 fft_data[ 2 ][ 256 ];
            memset( fft_data, 0, 512 );

            send( sockfd, "PCM", 4, 0 );
            nbytes = recv( sockfd, float_data, 512 * sizeof( float ), 0 );

            for ( uint x = 0; x < 512; ++x ) {
                pcm_data[ 0 ][ x ] = gint16( float_data[ x ] * ( 1 << 14 ) );
            }

            static fft_state *state = NULL;
            gfloat tmp_out[ 257 ];

            if ( !state ) state = fft_init();

            fft_perform( pcm_data[ 0 ], tmp_out, state );

            for ( uint i = 0; i < 256; i++ ) {
                fft_data[ 0 ][ i ] = fft_data[ 1 ][ i ] = ( ( gint ) sqrt( tmp_out[ i + 1 ] ) ) >> 8;
            }

            wrap.vis() ->render_freq( fft_data );
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

    close( sockfd );
}


int
tryConnect()
{
    //try to connect to the LoaderServer
    int fd = socket( AF_UNIX, SOCK_STREAM, 0 );

    if ( fd != -1 ) {

        struct sockaddr_un local;

        strcpy( &local.sun_path[ 0 ], socketpath.local8Bit() );
        local.sun_family = AF_UNIX;

        std::cout << "[amK] Connecting to " << socketpath.local8Bit() << '\n';

        if ( connect( fd, ( struct sockaddr* ) & local, sizeof( local ) ) == -1 ) {
            close ( fd );
            fd = -1;

            std::cerr << "[amK] tryConnect() failed\n";
        }
    }

    return fd;
}


/////////////////////////////////////////////////////////////////////////////////////////
// CLASS XmmsWrapper
/////////////////////////////////////////////////////////////////////////////////////////

XmmsWrapper::XmmsWrapper( const std::string &plugin )
{
    std::cout << "[amK] loading xmms plugin: " << plugin << '\n';

    std::string
    path = XMMS_PLUGIN_PATH;
    path += "/";
    path += plugin;

    { //<load plugin>

        void *h;
        void *( *gpi ) ( void );

        if ( ( h = dlopen( path.c_str(), RTLD_NOW ) ) == NULL ) {
            std::cout << dlerror() << "\n";
            return ;
        }

        if ( ( gpi = ( void * ( * ) () ) dlsym( h, "get_vplugin_info" ) ) != NULL ) {
            VisPlugin * p = ( VisPlugin* ) gpi();
            p->handle = h;
            p->filename = strdup( path.c_str() );
            p->xmms_session = 0; //ctrlsocket_get_session_id(); //FIXME
            p->disable_plugin = vis_disable_plugin; //FIXME what is this symbol?

            m_vis = p;
        } else { dlclose( h ); return ; }

    } //</load plugin>

    if ( m_vis->init ) { std::cout << "[amK] init()\n"; m_vis->init(); }
    if ( m_vis->playback_start ) { std::cout << "[amK] start()\n"; m_vis->playback_start(); }
}


XmmsWrapper::~XmmsWrapper()
{
    dlclose( m_vis );

    std::cout << "[amK] ~\n";
}




//NOTE as yet, these functions are a little mysterious to me

static void calc_stereo_pcm( gint16 dest[ 2 ][ 512 ], gint16 src[ 2 ][ 512 ], gint nch )
{
    memcpy( dest[ 0 ], src[ 0 ], 512 * sizeof( gint16 ) );
    if ( nch == 1 )
        memcpy( dest[ 1 ], src[ 0 ], 512 * sizeof( gint16 ) );
    else
        memcpy( dest[ 1 ], src[ 1 ], 512 * sizeof( gint16 ) );
}

static void calc_mono_pcm( gint16 dest[ 2 ][ 512 ], gint16 src[ 2 ][ 512 ], gint nch )
{
    gint i;
    gint16 *d, *sl, *sr;

    if ( nch == 1 )
        memcpy( dest[ 0 ], src[ 0 ], 512 * sizeof( gint16 ) );
    else {
        d = dest[ 0 ];
        sl = src[ 0 ];
        sr = src[ 1 ];
        for ( i = 0; i < 512; i++ ) {
            *( d++ ) = ( *( sl++ ) + *( sr++ ) ) >> 1;
        }
    }
}


static void calc_freq( gint16 *dest, gint16 *src )
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


static void calc_mono_freq( gint16 dest[ 2 ][ 256 ], gint16 src[ 2 ][ 512 ], gint nch )
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


static void calc_stereo_freq( gint16 dest[ 2 ][ 256 ], gint16 src[ 2 ][ 512 ], gint nch )
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



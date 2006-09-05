/***************************************************************************
 *   Copyright (C) 2004, 2005 Max Howell <max.howell@methylblue.com>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

extern "C"
{
    #include <sys/types.h>  //this must be _before_ sys/socket on freebsd
    #include <sys/socket.h>
    #include <sys/un.h>
    #include <unistd.h>
}

#include <cstdlib>
#include <cstring>
#include <iostream>
#include "libvisual.h"


int
main( int argc, char** argv )
{
    if( argc <= 1 || std::strcmp( argv[1], "--list" ) == 0 )
    {
        visual_init( &argc, &argv );

        #if 0
        VisList *list = visual_actor_get_list();

        for( VisListEntry *entry = list->head->next; entry != list->tail; entry = entry->next )
        {
            VisPluginInfo *info = static_cast<VisActor*>(entry->data)->plugin->ref->info;

            std::cout << info->name << '|' << info->about << std::endl;
        }
        #endif

        const char *plugin = 0;

        while( (plugin = visual_actor_get_next_by_name( plugin )) )
            std::cout << plugin << '\n';

        std::exit( 0 );
    }
    else if( argc == 3 )
        Vis::plugin = argv[2];


    //connect to socket
    const int sockfd = tryConnect( argv[1] );

    //register fd/pid combo with Amarok
    {
        pid_t pid = ::getpid();
        char  buf[32] = "REG";
        *(pid_t*)&buf[4] = pid;

        ::send( sockfd, buf, 4 + sizeof(pid_t), 0 );
    }

    //init
    SDL::init();
    Vis::init( argc, argv );


    //main loop
    // 1. we sleep for a bit, listening for messages from Amarok
    // 2. render a frame

    timeval tv;
    fd_set  fds;
    int     nbytes = 0;
    uint    render_time = 0;

    while( nbytes != -1 && SDL::event_handler() )
    {
        //set the time to wait
        tv.tv_sec  = 0;
        tv.tv_usec = render_time > 16 ? 0 : (16 - render_time) * 1000; //60Hz

        //get select to watch the right file descriptor
        FD_ZERO( &fds );
        FD_SET( sockfd, &fds );

        ::select( sockfd+1, &fds, 0, 0, &tv );

        if( FD_ISSET( sockfd, &fds) ) {
            //Amarok sent us some data

            char command[16];
            ::recv( sockfd, command, 16, 0 );

            if( std::strcmp( command, "fullscreen" ) == 0 )
                SDL::toggleFullScreen();
        }

        //request pcm data
        ::send( sockfd, "PCM", 4, 0 );
        nbytes = ::recv( sockfd, Vis::pcm_data, 1024 * sizeof( int16_t ), 0 );

        render_time = LibVisual::render();
    }

    ::close( sockfd );

    return 0;
}

static int
tryConnect( const char *path )
{
    const int fd = ::socket( AF_UNIX, SOCK_STREAM, 0 );

    if( fd != -1 )
    {
        struct sockaddr_un local;

        std::strcpy( &local.sun_path[ 0 ], path );
        local.sun_family = AF_UNIX;

        std::cout << "[Amk] Connecting to: " << path << '\n';

        if( connect( fd, (struct sockaddr*)&local, sizeof(local) ) == -1 )
        {
            ::close( fd );

            std::cerr << "[Amk] Could not connect\n";
            std::exit( -1 );
        }
    }

    return fd;
}


namespace SDL
{
    static inline void
    init()
    {
        if( SDL_Init( SDL_INIT_VIDEO ) )
        {
            std::cerr << "Could not initialize SDL: " << SDL_GetError() << std::endl;
            std::exit( -2 );
        }

        std::atexit( SDL::quit );
    }

    static void
    quit()
    {
        //FIXME crashes!
        //visual_bin_destroy( Vis::bin );
        //visual_quit();

        SDL_FreeSurface( screen );
        SDL_Quit();
    }

    static inline void
    set_pal()
    {
        if (Vis::pal)
            for( int i = 0; i < 256; i ++ )
            {
                SDL::pal[i].r = Vis::pal->colors[i].r;
                SDL::pal[i].g = Vis::pal->colors[i].g;
                SDL::pal[i].b = Vis::pal->colors[i].b;
            }

        SDL_SetColors( screen, SDL::pal, 0, 256 );
    }

    static void
    create( int width, int height )
    {
        SDL_FreeSurface( screen );

        if( Vis::pluginIsGL )
        {
            const SDL_VideoInfo *videoinfo = SDL_GetVideoInfo();

            if( videoinfo == NULL )
            {
                std::cerr << "CRITICAL: Could not get video info\n";
                std::exit( -2 );
            }

            int
            videoflags  = SDL_OPENGL | SDL_GL_DOUBLEBUFFER | SDL_HWPALETTE | SDL_RESIZABLE;
            videoflags |= videoinfo->hw_available ? SDL_HWSURFACE : SDL_SWSURFACE;

            if( videoinfo->blit_hw ) videoflags |= SDL_HWACCEL;

            SDL_GL_SetAttribute (SDL_GL_DOUBLEBUFFER, 1);

            screen = SDL_SetVideoMode( width, height, 16, videoflags );
        }
        else screen = SDL_SetVideoMode( width, height, Vis::video->bpp * 8, SDL_RESIZABLE );

        visual_video_set_buffer( Vis::video, screen->pixels );
        visual_video_set_pitch( Vis::video, screen->pitch );
    }

    static bool
    event_handler()
    {
        SDL_Event event;
        VisEventQueue *vevent;

        while( SDL_PollEvent( &event ) )
        {
            vevent = visual_plugin_get_eventqueue( visual_actor_get_plugin( visual_bin_get_actor( Vis::bin ) ) );

            switch( event.type )
            {
            case SDL_KEYUP:
                visual_event_queue_add_keyboard( vevent, (VisKey)event.key.keysym.sym, event.key.keysym.mod, VISUAL_KEY_UP );
                break;

            case SDL_KEYDOWN:
                visual_event_queue_add_keyboard (vevent, (VisKey)event.key.keysym.sym, event.key.keysym.mod, VISUAL_KEY_DOWN);

                switch( event.key.keysym.sym )
                {
                //PLUGIN CONTROLS
                case SDLK_F11:
                case SDLK_TAB:
                    SDL::toggleFullScreen();
                    break;

                case SDLK_ESCAPE:
                    if( SDL::isFullScreen() )
                        SDL::toggleFullScreen();
                    break;

                case SDLK_LEFT:
                    Vis::prevActor();
                    goto morph;

                case SDLK_RIGHT:
                    Vis::nextActor();

                morph:
                    SDL::lock();
                      visual_bin_set_morph_by_name( Vis::bin, (char*)"alphablend" );
                      visual_bin_switch_actor_by_name( Vis::bin, (char*)Vis::plugin );
                    SDL::unlock();

                    SDL_WM_SetCaption( Vis::plugin, 0 );

                    break;

                default:
                    ;
                }
                break;

            case SDL_VIDEORESIZE:
                Vis::resize( event.resize.w, event.resize.h );
                break;

            case SDL_MOUSEMOTION:
                visual_event_queue_add_mousemotion (vevent, event.motion.x, event.motion.y);
                break;

            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_RIGHT)
                {
                    SDL::toggleFullScreen();
                    break;
		}
                visual_event_queue_add_mousebutton (vevent, event.button.button, VISUAL_MOUSE_DOWN, 0, 0);
                break;

            case SDL_MOUSEBUTTONUP:
                visual_event_queue_add_mousebutton (vevent, event.button.button, VISUAL_MOUSE_UP, 0, 0);
                break;

            case SDL_QUIT:
                return false;

            default:
                ;
            }
        }

        return true;
    }
} //namespace SDL


namespace LibVisual
{
    static int
    upload_callback( VisInput*, VisAudio *audio, void* )
    {
        VisBuffer buf;

        visual_buffer_init( &buf, pcm_data, 1024, 0 );
        visual_audio_samplepool_input( audio->samplepool, &buf, VISUAL_AUDIO_SAMPLE_RATE_44100,
            VISUAL_AUDIO_SAMPLE_FORMAT_S16, VISUAL_AUDIO_SAMPLE_CHANNEL_STEREO );

        return 0;
    }

    static void
    resize( int width, int height )
    {
        visual_video_set_dimension( video, width, height );

        SDL::create( width, height );

        visual_bin_sync( bin, false );
    }

    static void
    init( int &argc, char **&argv )
    {
        VisVideoDepth depth;

        visual_init( &argc, &argv );

        bin    = visual_bin_new ();
        depth  = visual_video_depth_enum_from_value( 24 );

        if( !plugin ) plugin = visual_actor_get_next_by_name( 0 );
        if( !plugin ) exit( "Actor plugin not found!" );

        visual_bin_set_supported_depth( bin, VISUAL_VIDEO_DEPTH_ALL );

        if( NULL == (video = visual_video_new()) )       exit( "Cannot create a video surface" );
        if( visual_video_set_depth( video, depth ) < 0 ) exit( "Cannot set video depth" );

        visual_video_set_dimension( video, 320, 200 );

        if( visual_bin_set_video( bin, video ) ) exit( "Cannot set video" );

        visual_bin_connect_by_names( bin, (char*)plugin, 0 );

        if( visual_bin_get_depth( bin ) == VISUAL_VIDEO_DEPTH_GL )
        {
            visual_video_set_depth( video, VISUAL_VIDEO_DEPTH_GL );
            pluginIsGL = true;
        }

        SDL::create( 320, 200 );

        SDL_WM_SetCaption( plugin, 0 );

        /* Called so the flag is set to false, seen we create the initial environment here */
        visual_bin_depth_changed( bin );

        VisInput *input = visual_bin_get_input( bin );
        if( visual_input_set_callback( input, upload_callback, NULL ) < 0 ) exit( "Cannot set input plugin callback" );

        visual_bin_switch_set_style( bin, VISUAL_SWITCH_STYLE_MORPH );
        visual_bin_switch_set_automatic( bin, true );
        visual_bin_switch_set_steps( bin, 100 );

        visual_bin_realize( bin );
        visual_bin_sync( bin, false );

        std::cout << "[Amk] Libvisual version " << visual_get_version() << '\n';
        std::cout << "[Amk] bpp: " << video->bpp << std::endl;
        std::cout << "[Amk]  GL: "  << (pluginIsGL ? "true\n" : "false\n");
    }

    static uint
    render()
    {
        /* On depth change */
        if( visual_bin_depth_changed( bin ) )
        {
            SDL::lock();

            pluginIsGL = (visual_bin_get_depth( bin ) == VISUAL_VIDEO_DEPTH_GL);

            SDL::create( SDL::screen->w, SDL::screen->h );
            visual_bin_sync( bin, true );

            SDL::unlock();
        }

        long ticks = -SDL_GetTicks();

        if( pluginIsGL )
        {
            visual_bin_run( bin );

            SDL_GL_SwapBuffers();

        } else {

            SDL::lock();

            visual_video_set_buffer( video, SDL::screen->pixels );
            visual_bin_run( bin );

            SDL::unlock();

            Vis::pal = visual_bin_get_palette( bin );
            SDL::set_pal();

            SDL_Flip( SDL::screen );
        }

        ticks += SDL_GetTicks();
        return ticks;
    }
} //namespace LibVisual

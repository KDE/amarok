//Maintainer: Max Howell <max.howell@methylblue.com>, (C) 2004
//Copyright:  See COPYING file that comes with this distribution

#ifndef LIBVISUAL_H
#define LIBVISUAL_H

#include <libvisual/libvisual.h>
#include <SDL/SDL.h>


static int tryConnect();

namespace SDL
{
    static SDL_Surface *screen = NULL;
    static SDL_Color    pal[256];

    static void init();
    static bool event_handler();
    static void quit();

    static inline void   lock() { if( SDL_MUSTLOCK( screen ) == SDL_TRUE ) SDL_LockSurface( screen ); }
    static inline void unlock() { if( SDL_MUSTLOCK( screen ) == SDL_TRUE ) SDL_UnlockSurface( screen ); }

    static inline void
    toggleFullscreen()
    {
        SDL_WM_ToggleFullScreen( screen );
        SDL_ShowCursor( (screen->flags & SDL_FULLSCREEN) > 0 ? SDL_DISABLE : SDL_ENABLE );
    }
}

namespace LibVisual
{
    static VisVideo   *video;
    static VisPalette *pal;
    static VisBin     *bin;
    static char       *plugin;
    static bool        pluginIsGL = false;
    static int16_t     pcm_data[512];

    static void init( int&, char**& );
    static uint render();
    static void resize( int, int );

    static inline void
    nextActor()
    {
        plugin = visual_actor_get_next_by_name( plugin );

        if( plugin == NULL ) plugin = visual_actor_get_next_by_name( NULL );
    }

    static inline void
    prevActor()
    {
        plugin = visual_actor_get_prev_by_name( plugin );

        if( plugin == NULL ) plugin = visual_actor_get_prev_by_name( NULL );
    }

    static inline void
    exit( const char *msg )
    {
        std::cerr << msg << std::endl;
        std::exit( -3 );
    }
}

namespace Vis = LibVisual;

#endif

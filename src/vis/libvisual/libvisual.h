/***************************************************************************
 *   Copyright (C) 2004, 2005 Max Howell <max.howell@methylblue.com>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef LIBVISUAL_H
#define LIBVISUAL_H

extern "C"
{
    #include <libvisual/libvisual.h>
    #include <SDL.h>
}


namespace SDL
{
    static SDL_Surface *screen = 0;
    static SDL_Color    pal[256];

    static void init();
    static bool event_handler();
    static void quit();

    static inline void   lock() { if( SDL_MUSTLOCK( screen ) == SDL_TRUE ) SDL_LockSurface( screen ); }
    static inline void unlock() { if( SDL_MUSTLOCK( screen ) == SDL_TRUE ) SDL_UnlockSurface( screen ); }

    static inline bool
    isFullScreen()
    {
        return (screen->flags & SDL_FULLSCREEN) > 0;
    }

    static inline void
    toggleFullScreen()
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
    static const char *plugin;
    static bool        pluginIsGL = false;
    static int16_t     pcm_data[1024];

    static void init( int&, char**& );
    static uint render();
    static void resize( int, int );

    static inline void
    nextActor()
    {
        plugin = visual_actor_get_next_by_name( plugin );

        if( plugin == 0 ) plugin = visual_actor_get_next_by_name( 0 );
    }

    static inline void
    prevActor()
    {
        plugin = visual_actor_get_prev_by_name( plugin );

        if( plugin == 0 ) plugin = visual_actor_get_prev_by_name( 0 );
    }

    static inline void
    exit( const char *msg )
    {
        std::cerr << msg << std::endl;
        std::exit( -3 );
    }
}

namespace Vis = LibVisual;

static int tryConnect( const char *path );

#endif

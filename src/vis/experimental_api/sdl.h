// Author: Max Howell <max.howell@methylblue.com>, (C) 2004
// Copyright: See COPYING file that comes with this distribution
//

#ifndef AMK_VIS_SDL_H
#define AMK_VIS_SDL_H

#include "./base.hpp"
#include <SDL/SDL.h> //for convenience

namespace amaroK {
namespace Vis {
namespace SDL {


class Basic : public Vis::Implementation<SDL_Surface>
{
public:
    Basic( DataType = FFT, bool notify = false, uint fps = 0 );

    virtual void init( uint w = 640, uint h = 480, Uint32 flags = SDL_HWSURFACE|SDL_DOUBLEBUF );

    void drawPixel( SDL_Surface *, int x, int y, Uint8 R, Uint8 G, Uint8 B );

    int width()  const { return m_surface->w; } //warning do not call before exec()
    int height() const { return m_surface->h; } //warning do not call before exec()
};


class Convenience : public Vis::SDL::Basic
{
public:
    Convenience( DataType = FFT, bool notification = false, uint fps = 0 );

    virtual void render( SDL_Surface* );
    virtual void doRender( SDL_Surface* ) = 0;
};


//convenience functions

inline SDL_Rect
Rect( Sint16 x, Sint16 y, Uint16 w, Uint16 h )
{
    SDL_Rect rect;
    rect.x = x; rect.y = y;
    rect.w = w; rect.h = h;

    return rect;
}

}
}
}

#endif

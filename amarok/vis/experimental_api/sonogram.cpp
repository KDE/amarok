// Author: Max Howell <max.howell@methylblue.com>, (C) 2004
// Copyright: See COPYING file that comes with this distribution
//

#include "fht.h"
#include <iostream>
#include <math.h>
#include "./sdl.h"


using namespace amaroK::Vis;


class Sonogram : public SDL::Basic
{
public:
    Sonogram() : Basic( SDL_HWSURFACE, PCM, false, 30 ), m_fht( 8 ), m_data( 256, 0 ) {}

    virtual void render( SDL_Surface* );

private:
    FHT m_fht;
    std::vector<float> m_data;
};


void
Sonogram::render( SDL_Surface *screen )
{
    for( uint x = 0; x < 256; ++x ) m_data[x] = (float)left( x ) / (1 << 15);

    float *front = static_cast<float*>(&m_data.front());
    m_fht.power(front);
    m_fht.scale(front, 1.0 / 64);

    int x = width() - 1;
    int r = 0, g = 0, b = 0;
    const int h = height() - 1;

    //lock surface
    if( SDL_MUSTLOCK(screen) && SDL_LockSurface(screen) < 0 ) return;

    std::vector<float>::const_iterator it  = m_data.begin();
    std::vector<float>::const_iterator end = m_data.end();

    for( int y = h; y && it < end; ++it )
    {
        if( *it < .005 );
        else if( *it < .05 ) { r = 255; g = 20;  b = 20;  }// c.setHsv(95, 255, 255 - int(*it * 4000.0));
        else if( *it < 1.0 ) { r = 20;  g = 255; b = 20;  }// c.setHsv(95 - int(*it * 90.0), 255, 255);
        else                 { r = 20;  g = 20;  b = 255; }
        drawPixel( screen, x, y-- , r, g, b );
    }

    //unlock, blit and flip
    if( SDL_MUSTLOCK(screen) ) SDL_UnlockSurface(screen);

    SDL_Rect rect( SDL::Rect(1, 0, x, height()) );
    SDL_BlitSurface( screen, &rect, screen, NULL );

    //SDL_Flip(screen);
}

int main()
{
    Sonogram vis;

    return vis.exec();
}

// Author: Max Howell <max.howell@methylblue.com>, (C) 2004
// Copyright: See COPYING file that comes with this distribution
//

#include "fht.h"
#include <iostream>
#include <math.h>
#include "./sdl.h"


using namespace amaroK::Vis;

//from http://www.cs.rit.edu/~ncs/color/t_convert.html
static void HSVtoRGB( int &_r, int &_g, int &_b, float h, float s, float v ) //values of 0-255, bar h = 0-360
{
        float R,G,B;
        float *r = &R, *g = &G, *b = &B;

        int i;
        float f, p, q, t;
        if( s == 0 ) {
                // achromatic (grey)
                *r = *g = *b = v;
                return;
        }
        h /= 60;                        // sector 0 to 5
        i = floor( h );
        f = h - i;                      // factorial part of h
        p = v * ( 1 - s );
        q = v * ( 1 - s * f );
        t = v * ( 1 - s * ( 1 - f ) );
        switch( i ) {
                case 0:
                        *r = v;
                        *g = t;
                        *b = p;
                        break;
                case 1:
                        *r = q;
                        *g = v;
                        *b = p;
                        break;
                case 2:
                        *r = p;
                        *g = v;
                        *b = t;
                        break;
                case 3:
                        *r = p;
                        *g = q;
                        *b = v;
                        break;
                case 4:
                        *r = t;
                        *g = p;
                        *b = v;
                        break;
                default:                // case 5:
                        *r = v;
                        *g = p;
                        *b = q;
                        break;
        }

        _r = 255*R; _g = 255*G; _b = 255*B;

        //std::cout << _r << std::endl;
}

class Sonogram : public SDL::Basic
{
public:
    Sonogram() : Basic( 0/*SDL_HWSURFACE*/, PCM, false, 100 ), m_fht( 9 ), m_data( 512, 0 ) {}

    virtual void render( SDL_Surface* );

private:
    FHT   m_fht;
    Scope m_data;
};


void
Sonogram::render( SDL_Surface *screen )
{


    std::copy( left().begin(), left().end(), m_data.begin() );

    //for( uint x=0; x<200; x+=20) std::cout << m_data[x] << " " << left( x ) << std::endl;

    float *front = static_cast<float*>(&m_data.front());
    m_fht.power(front);
    m_fht.scale(front, 1.0 / 64);

    int r, g, b;
    const int h = height() - 1;
    const int x = width()  - 1;

    //lock surface
    if( SDL_MUSTLOCK(screen) && SDL_LockSurface(screen) < 0 ) return;

    for( int y = 0; y < 256; ++y )
    {
        float f = m_data[255 - y];

//        static float max = 0, min = 100;
//        if( f > max ) { max = f; std::cout << "max: " << f << std::endl; }
//        if( f < min ) { min = f; std::cout << "min: " << f << std::endl; }

        if( f < .005 ) { r = g = b = 0; }
        else if( f < .05 ) { HSVtoRGB( r, g, b, 95, 0.6+f*0.2, f*16.0 ); }
        else if( f < 1.0 ) { HSVtoRGB( r, g, b, 95 - f*90.0, 0.8, 0.8 ); }
        else               { HSVtoRGB( r, g, b, 5, 0.8, 0.9 ); }

        drawPixel( screen, x, y, r, g, b );
    }

    //unlock, blit and flip
    if( SDL_MUSTLOCK(screen) ) SDL_UnlockSurface(screen);

    SDL_Rect rect( SDL::Rect(1, 0, x, height()) );
    SDL_BlitSurface( screen, &rect, screen, NULL );

    SDL_Flip(screen);
}

int main()
{
    Sonogram vis;

    return vis.exec();
}

// Author: Max Howell <max.howell@methylblue.com>, (C) 2004
// Copyright: See COPYING file that comes with this distribution
//

#include "base.cpp"
#include "sdl.cpp"
#include "convolve.c"

#include <iostream>
#include <math.h>



using namespace amaroK::Vis;


class Monoscope : public SDL::Basic
{
public:
    Monoscope() : Basic( PCM, false, 50 )
    {
        colors[0] = 0;

        for (uint i = 1; i < 32; ++i)
        {
            colors[i] = (i*8 << 16) +(255 << 8);
            colors[i+31] = (255 << 16) + (((31 - i) * 8) << 8);
        }

        colors[63] = (40 << 16) + (75 << 8);
    }

    virtual void render( SDL_Surface* );

private:
    int colors[65];
};


void
Monoscope::render( SDL_Surface *screen )
{
    static int foo;
    static int bar;
    static int i, h;


    static Uint8 bits[ 257 * 129];
    static Uint8 *loc;

    static convolve_state *state = NULL;

    if( state == NULL ) state = convolve_init();


    static short newEq[CONVOLVE_BIG]; /* latest block of 512 samples. */
    static short copyEq[CONVOLVE_BIG];
    static int avgEq[CONVOLVE_SMALL];    /* a running average of the last few. */
    static int avgMax; /* running average of max sample. */


    int factor;
    int val;
    int max = 1;
    short * thisEq;


    //convert to the 16 bit ints the routine expects
    for( uint x = 0; x < 512; ++x )
    {
        newEq[x] = short(double(left( x )) * (1 << 15));
    }


    memcpy( copyEq, newEq, sizeof (short) * CONVOLVE_BIG );
    thisEq = copyEq;

    val = convolve_match (avgEq, copyEq, state);
    thisEq += val;

    memset(bits, 0, 256 * 128);

    static int phoo = -(1 << 30);

    for (i=0; i < 256; i++)
    {
        if( thisEq[i] > phoo ) { phoo = thisEq[i]; std::cout << phoo << std::endl; }

        foo = thisEq[i] + (avgEq[i] >> 1);

        avgEq[i] = foo;

        if (foo < 0)   foo = -foo;
        if (foo > max) max = foo;
    }

    avgMax += max - (avgMax >> 8);
    if (avgMax < max) avgMax = max; /* Avoid overflow */

    factor = 0x7fffffff / avgMax;


    /* Keep the scaling sensible. */
    if (factor > (1 << 18)) factor = 1 << 18;
    if (factor < (1 << 8))  factor = 1 << 8;

    for (i=0; i < 256; i++)
    {
        foo = avgEq[i] * factor;
        foo >>= 18;

        if (foo > 63) foo = 63;
        if (foo < -64) foo = -64;

        val = (i + ((foo+64) << 8));
        bar = val;

        if ((bar > 0) && (bar < (256 * 128)))
        {
            loc = bits + bar;
            if (foo < 0)
            {
                for (h = 0; h <= (-foo); h++)
                {
                    *loc = h;
                    loc+=256;
                }

            } else {

                for (h = 0; h <= foo; h++)
                {
                    *loc = h;
                    loc-=256;
                }
            }
        }
    }

    for (i=16; i < 128; i+=16)
    {
        for (h = 0; h < 256; h+=2)
        {
            bits[(i << 8) + h] = 63;
            if (i == 64) bits[(i << 8) + h + 1] = 63;
        }
    }

    for (i = 16; i < 256; i+=16)
    {
        for (h = 0; h < 128; h+=2)
        {
            bits[i + (h << 8)] = 63;
        }
    }

    //SDL outputing follows
    //FIXME expects a 15 or 16bit display

    for( uint x = 0; x < 257; ++x ) {
        for( uint y = 0; y < 129; ++y )
        {
            uint c = bits[ y * 256 + x ];

            Uint16 *bufp;
            bufp = (Uint16 *)screen->pixels + y*screen->pitch/2 + x;
            *bufp = colors[c];
        }
    }

    SDL_Flip(screen);
}

int main()
{
    Monoscope vis;

    vis.init( 257, 129 );
    return vis.exec();
}

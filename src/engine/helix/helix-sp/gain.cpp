/* ***** BEGIN LICENSE BLOCK *****
 *
 * This software is released under the provisions of the GPL version 2.
 * see file "COPYING".  If that file is not available, the full statement
 * of the license can be found at
 *
 * http://www.fsf.org/licensing/licenses/gpl.txt
 *
 * Portions Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved.
 * Copyright (c) 2005 Paul Cifarelli All Rights Reserved.
 *
 *
 * This file is part of the Helix DNA Technology. RealNetworks is the
 * developer of the Original Code and owns the copyrights in the
 * portions it created.
 *
 * This file, and the files included with this file, is distributed
 * and made available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS
 * ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET
 * ENJOYMENT OR NON-INFRINGEMENT.
 *
 * Technology Compatibility Kit Test Suite(s) Location:
 *    http://www.helixcommunity.org/content/tck
 *
 * ***** END LICENSE BLOCK ***** */

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <iostream>
#include "hxassert.h"

#include "gain.h"

using namespace std;

#define INT8_CEILING  255
#define INT16_CEILING 32767
#define INT32_CEILING 65535

struct GAIN_STATE
{
   int sampleRate;
   int nChannels;
   int bytesPerSample;
   bool  isMute;
   float instGain; /* gain applied right now */
   float tgtGain;  /* in a smooth gain change, the gain we are aiming for */
   float decay;
};

GAIN_STATE* gainInit(int sampleRate, int nChannels, int bytesPerSample)
{
    GAIN_STATE* g = (GAIN_STATE*) calloc(1,sizeof(GAIN_STATE)) ;
    if (g)
    {
        g->sampleRate = sampleRate;
        g->nChannels = nChannels;
        g->bytesPerSample = bytesPerSample;
        gainSetTimeConstant(0.1f, g);
    }

    return g ;
}

void gainFree(GAIN_STATE* g)
{
    if (g) free(g) ;
}

float gainSetSmoothdB(float dB, GAIN_STATE* g)
{
    float gain = pow(10.0, 0.05*dB) ;

    if (g)
    {  
       g->isMute = false;  
       g->tgtGain = gain ;
    }

    return dB ;
}

float gainSetImmediatedB(float dB, GAIN_STATE* g)
{
    dB = gainSetSmoothdB(dB, g) ;

    if (g)
       g->instGain = g->tgtGain ; // make it instantaneous

    return dB ;
}

float gainSetSmooth(float percent, GAIN_STATE* g)
{
    float gaintop = pow(10.0, 0.05*GAIN_MAX_dB) ;
    float gainbottom = pow(10.0, 0.05*GAIN_MIN_dB) ;
    float gain = percent * (gaintop - gainbottom) + gainbottom;

    if (g)
    {
       g->isMute = false;
       g->tgtGain = gain ;
    }

    return gain;
}

float gainSetImmediate(float percent, GAIN_STATE* g)
{
    float gain = gainSetSmooth(percent, g) ;

    if (g)
       g->instGain = g->tgtGain ; // make it instantaneous

    return gain;
}

void gainSetMute(GAIN_STATE* g)
{
   if (g)
   {
      g->isMute = true;
      g->instGain = g->tgtGain = 0.0; // mute is immediate
   }
}

int gainSetTimeConstant(float millis, GAIN_STATE* g)
{
   if (!g)
      return 0;

    // we define the time constant millis so that the signal has decayed to 1/2 (-6dB) after
    // millis milliseconds have elapsed.
    // Let T[sec] = millis/1000 = time constant in units of seconds
    //
    // => (1-2^-s)^(T[sec]*sr) = 1/2
    // => 1-2^-s = (1/2)^(1/(T[sec]*sr))
    // => 2^-s = 1 - (1/2)^(1/(T[sec]*sr))
    // => s = -log2(1 - (1/2)^(1 / (T[sec]*sr)))

    // first 0.5 is rounding constant
   int shift;
   shift = (int)(0.5 - 1.0/log(2.0)*log(1.0 - pow(0.5, 1000.0/(millis * g->sampleRate)))) ;
   if (shift < 1)
      shift = 1 ;
   if (shift > 31)
      shift = 31 ;

   g->decay = ::pow(2.0, (float) shift);

   return 1 ; // OK
}

static void gainFeedMono(unsigned char* signal, unsigned char *outsignal, int len, GAIN_STATE *g)
{
   if (!g)
      return;

   float tgtGain = g->tgtGain ;
   float gain = g->instGain ;
   unsigned char *bufferEnd = signal + len;

   if (gain == tgtGain)
   { // steady state
      while (signal < bufferEnd)
      {
         switch (g->bytesPerSample)
         {
            case 1:
            {
               short int res;
               char *s = (char *) signal;
               char *o = (char *) outsignal;
               res = (short int) (*s * gain);
               *o = (char) (res > INT8_CEILING ? INT8_CEILING : res);
            }
            break;
            case 2:
            {
               long res;
               short int *s = (short int *) signal;
               short int *o = (short int *) outsignal;
               res = (long) (*s * gain);
               *o = (short int) (res > INT16_CEILING ? INT16_CEILING : res);
            }
            break;
            case 4:
            {
               long long res;
               long *s = (long *) signal;
               long *o = (long *) outsignal;
               res = (long long) (*s * gain);
               *o = (long) (res > INT32_CEILING ? INT32_CEILING : res);
            }
            break;
            default:
               return;
         }
         signal += g->bytesPerSample;
         outsignal += g->bytesPerSample;
      }
   }
   else
   { // while we are still ramping the gain
      while (signal < bufferEnd)
      {
         switch (g->bytesPerSample)
         {
            case 1:
            {
               short int res;
               char *s = (char *) signal;
               char *o = (char *) outsignal;
               res = (short int) (*s * gain);
               *o = (char) (res > INT8_CEILING ? INT8_CEILING : res);
            }
            break;
            case 2:
            {
               long res;
               short int *s = (short int *) signal;
               short int *o = (short int *) outsignal;
               res = (long) (*s * gain);
               *o = (short int) (res > INT16_CEILING ? INT16_CEILING : res);
            }
            break;
            case 4:
            {
               long long res;
               long *s = (long *) signal;
               long *o = (long *) outsignal;
               res = (long long) (*s * gain);
               *o = (long) (res > INT32_CEILING ? INT32_CEILING : res);
            }
            break;
            default:
               return;
         }
         signal += g->bytesPerSample;
         outsignal += g->bytesPerSample;
         gain += ((tgtGain-gain) / g->decay);
      }
      g->instGain = gain ;
   }
}

static void gainFeedStereo(unsigned char* signal, unsigned char *outsignal, int len, GAIN_STATE *g)
{
   if (!g)
      return;

   float tgtGain = g->tgtGain ;
   float gain = g->instGain ;
   unsigned char *bufferEnd = signal + len;

   if (gain == tgtGain)
   { // steady state
      while (signal < bufferEnd)
      {
         switch (g->bytesPerSample)
         {
            case 1:
            {
               short int res;
               char *s = (char *) signal;
               char *o = (char *) outsignal;
               res = (short int) (*s * gain);
               *o = (char) (res > INT8_CEILING ? INT8_CEILING : res);
               s++;
               o++;
               res = (short int) (*s * gain);
               *o = (char) (res > INT8_CEILING ? INT8_CEILING : res);
            }
            break;
            case 2:
            {
               long res;
               short int *s = (short int *) signal;
               short int *o = (short int *) outsignal;
               res = (long) (*s * gain);
               *o = (short int) (res > INT16_CEILING ? INT16_CEILING : res);
               s++;
               o++;
               res = (long) (*s * gain);
               *o = (short int) (res > INT16_CEILING ? INT16_CEILING : res);
            }
            break;
            case 4:
            {
               long long res;
               long *s = (long *) signal;
               long *o = (long *) outsignal;
               res = (long long) (*s * gain);
               *o = (long) (res > INT32_CEILING ? INT32_CEILING : res);
               s++;
               o++;
               res = (long long) (*s * gain);
               *o = (long) (res > INT32_CEILING ? INT32_CEILING : res);
            }
            break;
            default:
               return;
         }
         signal += 2 * g->bytesPerSample;
         outsignal += 2 * g->bytesPerSample;
      }
   }
   else
   { // while we are still ramping the gain
      while (signal < bufferEnd)
      {
         switch (g->bytesPerSample)
         {
            case 1:
            {
               short int res;
               char *s = (char *) signal;
               char *o = (char *) outsignal;
               res = (short int) (*s * gain);
               *o = (char) (res > INT8_CEILING ? INT8_CEILING : res);
               s++;
               o++;
               res = (short int) (*s * gain);
               *o = (char) (res > INT8_CEILING ? INT8_CEILING : res);
            }
            break;
            case 2:
            {
               long res;
               short int *s = (short int *) signal;
               short int *o = (short int *) outsignal;
               res = (long) (*s * gain);
               *o = (short int) (res > INT16_CEILING ? INT16_CEILING : res);
               s++;
               o++;
               res = (long) (*s * gain);
               *o = (short int) (res > INT16_CEILING ? INT16_CEILING : res);
            }
            break;
            case 4:
            {
               long long res;
               long *s = (long *) signal;
               long *o = (long *) outsignal;
               res = (long long) (*s * gain);
               *o = (long) (res > INT32_CEILING ? INT32_CEILING : res);
               s++;
               o++;
               res = (long long) (*s * gain);
               *o = (long) (res > INT32_CEILING ? INT32_CEILING : res);
            }
            break;
            default:
               return;
         }
         signal += 2 * g->bytesPerSample;
         outsignal += 2 * g->bytesPerSample;
         gain += ((tgtGain-gain) / g->decay);
      }
      g->instGain = gain ;
   }
}

static void gainFeedMulti(unsigned char* signal, unsigned char *outsignal, int len, GAIN_STATE *g)
{
   if (!g)
      return;

    float tgtGain = g->tgtGain ;
    float gain = g->instGain ;
    unsigned char *bufferEnd = signal + len;

    if (gain == tgtGain)
    { // steady state
       while (signal < bufferEnd)
       {
          switch (g->bytesPerSample)
          {
             case 1:
             {
                short int res;
                int i ;
                char *s = (char *) signal;
                char *o = (char *) outsignal;
                for (i = 0 ; i < g->nChannels ; i++)
                {
                   res = (short int) (*s * gain);
                   *o = (char) (res > INT8_CEILING ? INT8_CEILING : res);
                   s++;
                   o++;
                }
             }
             break;
             case 2:
             {
                long res;
                int i ;
                short int *s = (short int *) signal;
                short int *o = (short int *) outsignal;
                for (i = 0 ; i < g->nChannels ; i++)
                {
                   res = (long) (*s * gain);
                   *o = (short int) (res > INT16_CEILING ? INT16_CEILING : res);
                   s++;
                   o++;
                }
             }
             break;
             case 4:
             {
                long long res;
                int i ;
                long *s = (long *) signal;
                long *o = (long *) outsignal;
                for (i = 0 ; i < g->nChannels ; i++)
                {
                   res = (long long) (*s * gain);
                   *o = (long) (res > INT32_CEILING ? INT32_CEILING : res);
                   s++;
                   o++;
                }
             }
             break;
             default:
                return;
          }
          signal += g->nChannels * g->bytesPerSample;
          outsignal += g->nChannels * g->bytesPerSample;
       }
    }
    else
    { // while we are still ramping the gain
       while (signal < bufferEnd)
       {
          int i ;

          switch (g->bytesPerSample)
          {
             case 1:
             {
                short int res;
                char *s = (char *) signal;
                char *o = (char *) outsignal;
                for (i = 0 ; i < g->nChannels ; i++)
                {
                   res = (short int) (*s * gain);
                   *o = (char) (res > INT8_CEILING ? INT8_CEILING : res);
                   s++;
                   o++;
                }
             }
             break;
             case 2:
             {
                long res;
                short int *s = (short int *) signal;
                short int *o = (short int *) outsignal;
                for (i = 0 ; i < g->nChannels ; i++)
                {
                   res = (long) (*s * gain);
                   *o = (short int) (res > INT16_CEILING ? INT16_CEILING : res);
                   s++;
                   o++;
                }
             }
             case 4:
             {
                long long res;
                long *s = (long *) signal;
                long *o = (long *) outsignal;
                for (i = 0 ; i < g->nChannels ; i++)
                {
                   res = (long long) (*s * gain);
                   *o = (long) (res > INT32_CEILING ? INT32_CEILING : res);
                   s++;
                   o++;
                }
             }
             break;
          }
          signal += g->nChannels * g->bytesPerSample;
          outsignal += g->nChannels * g->bytesPerSample;
          gain += ((tgtGain-gain) / g->decay);
       }
       g->instGain = gain ;
    }
}

void gainFeed(unsigned char* signal, unsigned char *outsignal, int len, GAIN_STATE* g)
{
   if (!g)
      return;

    /* if the gain is 0dB, and we are not currently ramping, shortcut. */
    if (g->instGain == 1.0 && g->instGain == g->tgtGain)
    {
       if (signal != outsignal)
          memcpy(outsignal, signal, len);

        return ;
    }
    switch (g->nChannels)
    {
    case 1:
        gainFeedMono(signal, outsignal, len, g) ;
        break ;
    case 2:
        gainFeedStereo(signal, outsignal, len, g) ;
        break ;
    default:
        gainFeedMulti(signal, outsignal, len, g) ;
        break ;
    }
}

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

GainTool::GainTool(int sampleRate, int nChannels, int bytesPerSample) : m_g(0)
{
   m_g = gainInit(sampleRate, nChannels, bytesPerSample);
   gainSetTimeConstant(0.1f);
}

GainTool::~GainTool()
{
   gainFree(m_g);
}

GAIN_STATE* GainTool::gainInit(int sampleRate, int nChannels, int bytesPerSample)
{
    GAIN_STATE* g = (GAIN_STATE*) calloc(1,sizeof(GAIN_STATE)) ;
    if (g)
    {
        g->sampleRate = sampleRate;
        g->nChannels = nChannels;
        g->bytesPerSample = bytesPerSample;
    }

    return g ;
}

void GainTool::gainFree(GAIN_STATE* g)
{
    if (g) free(g) ;
}

float GainTool::gainSetSmoothdB(float dB)
{
    float gain = pow(10.0, 0.05*dB) ;

    m_g->isMute = false;

    m_g->tgtGain = gain ;

    return dB ;
}

float GainTool::gainSetImmediatedB(float dB)
{
    dB = gainSetSmoothdB(dB);

    m_g->instGain = m_g->tgtGain ; // make it instantaneous

    return dB ;
}

float GainTool::gainSetSmooth(float percent)
{
    float gaintop = pow(10.0, 0.05*GAIN_MAX_dB) ;
    float gainbottom = pow(10.0, 0.05*GAIN_MIN_dB) ;
    float gain = percent * (gaintop - gainbottom) + gainbottom;

    m_g->isMute = false;

    m_g->tgtGain = gain ;

    return gain;
}

float GainTool::gainSetImmediate(float percent)
{
    float gain = gainSetSmooth(percent) ;

    m_g->instGain = m_g->tgtGain ; // make it instantaneous

    return gain;
}

void GainTool::gainSetMute()
{
   m_g->isMute = true;
   m_g->instGain = m_g->tgtGain = 0.0; // mute is immediate
}

int GainTool::gainSetTimeConstant(float millis)
{
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
   shift = (int)(0.5 - 1.0/log(2.0)*log(1.0 - pow(0.5, 1000.0/(millis * m_g->sampleRate)))) ;
   if (shift < 1)
      shift = 1 ;
   if (shift > 31)
      shift = 31 ;

   m_g->decay = ::pow(2.0, (float) shift);

   return 1 ; // OK
}

void GainTool::gainFeedMono(unsigned char* signal, unsigned char *outsignal, int len)
{
   float tgtGain = m_g->tgtGain ;
   float gain = m_g->instGain ;
   unsigned char *bufferEnd = signal + len;

   if (gain == tgtGain)
   { // steady state
      while (signal < bufferEnd)
      {
         switch (m_g->bytesPerSample)
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
         signal += m_g->bytesPerSample;
         outsignal += m_g->bytesPerSample;
      }
   }
   else
   { // while we are still ramping the gain
      while (signal < bufferEnd)
      {
         switch (m_g->bytesPerSample)
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
         signal += m_g->bytesPerSample;
         outsignal += m_g->bytesPerSample;
         gain += ((tgtGain-gain) / m_g->decay);
      }
      m_g->instGain = gain ;
   }
}

void GainTool::gainFeedStereo(unsigned char* signal, unsigned char *outsignal, int len)
{
   float tgtGain = m_g->tgtGain ;
   float gain = m_g->instGain ;
   unsigned char *bufferEnd = signal + len;

   if (gain == tgtGain)
   { // steady state
      while (signal < bufferEnd)
      {
         switch (m_g->bytesPerSample)
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
         signal += 2 * m_g->bytesPerSample;
         outsignal += 2 * m_g->bytesPerSample;
      }
   }
   else
   { // while we are still ramping the gain
      while (signal < bufferEnd)
      {
         switch (m_g->bytesPerSample)
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
         signal += 2 * m_g->bytesPerSample;
         outsignal += 2 * m_g->bytesPerSample;
         gain += ((tgtGain-gain) / m_g->decay);
      }
      m_g->instGain = gain ;
   }
}

void GainTool::gainFeedMulti(unsigned char* signal, unsigned char *outsignal, int len)
{
    float tgtGain = m_g->tgtGain ;
    float gain = m_g->instGain ;
    unsigned char *bufferEnd = signal + len;

    if (gain == tgtGain)
    { // steady state
       while (signal < bufferEnd)
       {
          switch (m_g->bytesPerSample)
          {
             case 1:
             {
                short int res;
                int i ;
                char *s = (char *) signal;
                char *o = (char *) outsignal;
                for (i = 0 ; i < m_g->nChannels ; i++)
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
                for (i = 0 ; i < m_g->nChannels ; i++)
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
                for (i = 0 ; i < m_g->nChannels ; i++)
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
          signal += m_g->nChannels * m_g->bytesPerSample;
          outsignal += m_g->nChannels * m_g->bytesPerSample;
       }
    }
    else
    { // while we are still ramping the gain
       while (signal < bufferEnd)
       {
          int i ;

          switch (m_g->bytesPerSample)
          {
             case 1:
             {
                short int res;
                char *s = (char *) signal;
                char *o = (char *) outsignal;
                for (i = 0 ; i < m_g->nChannels ; i++)
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
                for (i = 0 ; i < m_g->nChannels ; i++)
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
                for (i = 0 ; i < m_g->nChannels ; i++)
                {
                   res = (long long) (*s * gain);
                   *o = (long) (res > INT32_CEILING ? INT32_CEILING : res);
                   s++;
                   o++;
                }
             }
             break;
          }
          signal += m_g->nChannels * m_g->bytesPerSample;
          outsignal += m_g->nChannels * m_g->bytesPerSample;
          gain += ((tgtGain-gain) / m_g->decay);
       }
       m_g->instGain = gain ;
    }
}

void GainTool::gainFeed(unsigned char* signal, unsigned char *outsignal, int len)
{
    /* if the gain is 0dB, and we are not currently ramping, shortcut. */
    if (m_g->instGain == 1.0 && m_g->instGain == m_g->tgtGain)
    {
       if (signal != outsignal)
          memcpy(outsignal, signal, len);

        return ;
    }
    switch (m_g->nChannels)
    {
    case 1:
        gainFeedMono(signal, outsignal, len);
        break ;
    case 2:
        gainFeedStereo(signal, outsignal, len);
        break ;
    default:
        gainFeedMulti(signal, outsignal, len);
        break ;
    }
}

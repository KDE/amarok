/* **********
 *
 * This software is released under the provisions of the GPL version 2.
 * see file "COPYING".  If that file is not available, the full statement 
 * of the license can be found at
 *
 * http://www.fsf.org/licensing/licenses/gpl.txt
 *
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved.
 * Portions Copyright (c) Paul Cifarelli 2005
 *
 * ********** */
#include <stdlib.h>

#include "hxcomm.h"
#include "hxcore.h"
#include "hxprefs.h"
#include "hxstrutl.h"
#include "hxvsrc.h"
#include "hxresult.h"
#include "hxausvc.h"
#include "print.h"
#include "helix-sp.h"

#include "hsphook.h"

HSPPostMixAudioHook::HSPPostMixAudioHook(HelixSimplePlayer *player, int playerIndex) : 
   m_Player(player), m_lRefCount(0), m_index(playerIndex), m_count(0)
{
}

HSPPostMixAudioHook::~HSPPostMixAudioHook()
{
}

STDMETHODIMP
HSPPostMixAudioHook::QueryInterface(REFIID riid, void**ppvObj)
{
    if(IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXAudioHook *)this;
        return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXAudioHook))
    {
        AddRef();
        *ppvObj = (IHXAudioHook *)this;
        return HXR_OK;
    }
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(UINT32)
HSPPostMixAudioHook::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(UINT32)
HSPPostMixAudioHook::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP HSPPostMixAudioHook::OnBuffer(HXAudioData *pAudioInData, HXAudioData * /*pAudioOutData*/)
{
   unsigned long len;
   unsigned char *data;

   pAudioInData->pData->Get(data, len);

   m_count++;

#ifdef DEBUG_PURPOSES_ONLY
   if (!(m_count % 100))
   {
      STDERR("time: %d  ", pAudioInData->ulAudioTime);
      switch (pAudioInData->uAudioStreamType)
      {
         case INSTANTANEOUS_AUDIO:
            STDERR(" INSTANTANEOUS_AUDIO ");
            break;
         case STREAMING_AUDIO:
            STDERR(" STREAMING_AUDIO ");
            break;
         case TIMED_AUDIO:
            STDERR(" TIMED_AUDIO ");
            break;
         case STREAMING_INSTANTANEOUS_AUDIO:
            STDERR(" STREAMING_INSTANTANEOUS_AUDIO ");
            break;
      }
      STDERR("len %d\n", len);
   }
#endif

   // since this is a postmix hook, we get the samples in CD quality (2 channels would have ~4400 samples at 16 bits/sample)
   // for the scope we want only 512 samples, so we downsample here.
   // since the scope updates at a much slower rate, we collect these downsampled packets and resample again at the scope intervals

   struct DelayQueue *item = new struct DelayQueue;
   int *buf = item->buf;
   item->time = pAudioInData->ulAudioTime;
   int bytes_per_sample = m_format.uBitsPerSample / 8;

   // TODO: 32 bit samples
   if (bytes_per_sample != 1 && bytes_per_sample != 2)
      return 0; // no scope

   int current = 0;
   int samples_per_block = len / (m_format.uChannels * bytes_per_sample);
   int inc;
   int j,k=0;
   short *pint;

   if (samples_per_block > 512)
      inc = samples_per_block / 512;
   else
      inc = 1;

   // convert to mono and resample
   int a;
   unsigned char b[4];

   current = 0;
   while (k < (int) len)
   {
      a = 0;
      for (j=0; j<m_format.uChannels; j++)
      {
         switch (bytes_per_sample)
         {
            case 1:
               b[1] = 0;
               b[0] = data[k];
               break;
            case 2:
               b[1] = data[k+1];
               b[0] = data[k];
               break;
         }

         pint = (short *) &b[0];
         
         a += (int) *pint;
         k += bytes_per_sample;
      }
      a /= m_format.uChannels;

      buf[current] = a;
      current++;
      if (current >= 512)
      {
         current = 511;
         break;
      }

      k += m_format.uChannels * bytes_per_sample * (inc - 1);
   }

#ifndef TEST_APP
   m_Player->addScopeBuf(item);
#endif

   return 0;
}

STDMETHODIMP HSPPostMixAudioHook::OnInit(HXAudioFormat *pFormat)
{
   STDERR("HOOK OnInit AudioFormat: ch %d, bps %d, sps %d, mbs %d\n", pFormat->uChannels,
          pFormat->uBitsPerSample,
          pFormat->ulSamplesPerSec,
          pFormat->uMaxBlockSize);

   m_format = *pFormat;
   m_count = 0;

   return 0;
}


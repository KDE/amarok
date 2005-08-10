/* **********
 *
 * This software is released under the provisions of the GPL version 2.
 * see file "COPYING".  If that file is not available, the full statement 
 * of the license can be found at
 *
 * http://www.fsf.org/licensing/licenses/gpl.txt
 *
 * Copyright (c) Paul Cifarelli 2005
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved.
 * PCM time-domain equalizer:
 *    (c) 2002 Felipe Rivera <liebremx at users sourceforge net>
 *    (c) 2004 Mark Kretschmann <markey@web.de>
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
#ifndef HELIX_SW_VOLUME_INTERFACE
#include "gain.h"
#endif
#include "hsphook.h"
#include "iir_cf.h"         // IIR filter coefficients

#define SCOPE_BUF_PER_BLOCK 8
#define SCOPESIZE 512

HSPPreMixAudioHook::HSPPreMixAudioHook(HelixSimplePlayer *player, int playerIndex, IHXAudioStream *pAudioStream) : 
   m_Player(player), m_lRefCount(0), m_index(playerIndex), m_stream(pAudioStream), m_count(0)
{
   AddRef();
}

HSPPreMixAudioHook::~HSPPreMixAudioHook()
{
}

STDMETHODIMP
HSPPreMixAudioHook::QueryInterface(REFIID riid, void**ppvObj)
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
HSPPreMixAudioHook::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(UINT32)
HSPPreMixAudioHook::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP HSPPreMixAudioHook::OnBuffer(HXAudioData */*pAudioInData*/, HXAudioData */*pAudioOutData*/)
{
   m_count++;

#ifdef DEBUG_PURPOSES_ONLY
   if (!(m_count % 100))
   {
      STDERR("PRE: time: %d  ", pAudioInData->ulAudioTime);
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
      STDERR("pAudioOutData %lx, data %lx\n", pAudioOutData, pAudioOutData->pData);
   }
#endif

/*
   unsigned char *outbuf, *data;
   unsigned long len;
   IHXBuffer *ibuf;

   m_Player->pCommonClassFactory->CreateInstance(CLSID_IHXBuffer, (void **) &ibuf);
   if (ibuf)
   {
      pAudioInData->pData->Get(data, len);
      ibuf->SetSize(len);
      outbuf = ibuf->GetBuffer();
      memcpy(outbuf, data, len);
      pAudioOutData->pData = ibuf;
      pAudioOutData->ulAudioTime = pAudioInData->ulAudioTime;
      pAudioOutData->uAudioStreamType = pAudioInData->uAudioStreamType;
   }
*/

   return 0;
}

STDMETHODIMP HSPPreMixAudioHook::OnInit(HXAudioFormat *pFormat)
{
   STDERR("PRE MIX HOOK OnInit AudioFormat: ch %d, bps %d, sps %d, mbs %d\n", pFormat->uChannels,
          pFormat->uBitsPerSample,
          pFormat->ulSamplesPerSec,
          pFormat->uMaxBlockSize);

   m_format = *pFormat;

   return 0;
}


///////////////////////////////////////////////////////////////////////////////////////////////////

HSPPostMixAudioHook::HSPPostMixAudioHook(HelixSimplePlayer *player, int playerIndex) : 
   m_Player(player), m_lRefCount(0), m_index(playerIndex), m_count(0), m_item(0), 
   m_current(0), m_prevtime(0), m_i(0), m_j(2), m_k(1)
#ifndef HELIX_SW_VOLUME_INTERFACE
   , m_gaintool(0), m_gaindB(0.0)
#endif
{
   AddRef();
   memset(&m_format, 0, sizeof(m_format));
}

HSPPostMixAudioHook::~HSPPostMixAudioHook()
{
#ifndef HELIX_SW_VOLUME_INTERFACE
   if (m_gaintool)
      gainFree(m_gaintool);
#endif
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

STDMETHODIMP HSPPostMixAudioHook::OnBuffer(HXAudioData *pAudioInData, HXAudioData *pAudioOutData)
{
   unsigned long len;
   unsigned char *data;

   pAudioInData->pData->Get(data, len);

   m_count++;

#ifdef DEBUG_PURPOSES_ONLY
   if (!(m_count % 100))
   {
      STDERR("POST: time: %d  ", pAudioInData->ulAudioTime);
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
      STDERR("pAudioOutData %lx, data %lx\n", pAudioOutData, pAudioOutData->pData);
   }
#endif

   // feed the visualizations
   scopeify(pAudioInData->ulAudioTime, data, len);

#ifndef HELIX_SW_VOLUME_INTERFACE
   unsigned char *outbuf;
   IHXBuffer *ibuf;

   m_Player->pCommonClassFactory->CreateInstance(CLSID_IHXBuffer, (void **) &ibuf);
   if (ibuf)
   {
      ibuf->SetSize(len);
      outbuf = ibuf->GetBuffer();

      // equalize
      if (m_Player->ppctrl[m_index]->volume && m_Player->isEQenabled() && m_format.uBitsPerSample == 16)
      {
         equalize(data, outbuf, len);

         // finally adjust the volume
         len = volumeize(outbuf, len);
      }
      else
         // finally adjust the volume
         len = volumeize(data, outbuf, len);


      pAudioOutData->pData = ibuf;
      pAudioOutData->ulAudioTime = pAudioInData->ulAudioTime;
      pAudioOutData->uAudioStreamType = pAudioInData->uAudioStreamType;
   }
#else
   // equalize
   if (m_Player->ppctrl[m_index]->volume && m_Player->isEQenabled() && m_format.uBitsPerSample == 16)
   {
      unsigned char *outbuf;
      IHXBuffer *ibuf;

      m_Player->pCommonClassFactory->CreateInstance(CLSID_IHXBuffer, (void **) &ibuf);
      if (ibuf)
      {
         ibuf->SetSize(len);
         outbuf = ibuf->GetBuffer();
         equalize(data, outbuf, len);
         pAudioOutData->pData = ibuf;
         pAudioOutData->ulAudioTime = pAudioInData->ulAudioTime;
         pAudioOutData->uAudioStreamType = pAudioInData->uAudioStreamType;
      }
   }
#endif

   return 0;
}

STDMETHODIMP HSPPostMixAudioHook::OnInit(HXAudioFormat *pFormat)
{
   STDERR("POST MIX HOOK OnInit AudioFormat: ch %d, bps %d, sps %d, mbs %d\n", pFormat->uChannels,
          pFormat->uBitsPerSample,
          pFormat->ulSamplesPerSec,
          pFormat->uMaxBlockSize);

   m_format = *pFormat;
   m_count = 0;

   // set the filter coefficients, in case we need to use the equalizer
   switch(pFormat->ulSamplesPerSec)
   {
      case 8000:
         iir_cf = iir_cf10_8000;
         break;

      case 11025:
         iir_cf = iir_cf10_11025;
         break;
         
      case 16000:
         iir_cf = iir_cf10_16000;
         break;
         
      case 22050:
         iir_cf = iir_cf10_22050;
         break;

      case 32000:
         iir_cf = iir_cf10_32000;
         break;
         
      case 48000:
         iir_cf = iir_cf10_48000;
         break;

      case 44100:
      default:
         iir_cf = iir_cf10_44100;
         break;
   }

   m_i = 0;
   m_j = 2;
   m_k = 1;


#ifndef HELIX_SW_VOLUME_INTERFACE
   // setup the gain tool for volume
   if (m_gaintool)
      gainFree(m_gaintool);

   int bps = pFormat->uBitsPerSample / 8;
   m_gaintool = gainInit(pFormat->ulSamplesPerSec, pFormat->uChannels, bps);
   setGain(m_Player->ppctrl[m_index]->volume);
#endif

   return 0;
}


#ifndef HELIX_SW_VOLUME_INTERFACE
void HSPPostMixAudioHook::setGain(int volume)
{
   if (m_gaintool)
   {
      if (volume == 0)
         gainSetMute(m_gaintool);
      else
      {
         //m_gaindB = GAIN_MIN_dB + (GAIN_MAX_dB - GAIN_MIN_dB) * (float) volume / 100.0;
         //STDERR("GAIN set to %f\n", m_gaindB);
         //gainSetImmediatedB(m_gaindB, m_gaintool);

         gainSetImmediate( (float) volume / 100.0, m_gaintool );
         STDERR("GAIN set to %f\n", (float) volume);
      }
   }
}
#endif


void HSPPostMixAudioHook::scopeify(unsigned long time, unsigned char *data, size_t len)
{
   int bytes_per_sample = m_format.uBitsPerSample / 8;

   // TODO: 32 bit samples
   if (bytes_per_sample != 1 && bytes_per_sample != 2)
      return; // no scope

   unsigned long scopebuf_timeinc = (unsigned long)(1000.0 * (double)len / ((double)m_format.ulSamplesPerSec * (double)bytes_per_sample));
   DelayQueue *item = new DelayQueue(len);
   memcpy(item->buf, data, len);
   item->len = len;
   item->time = time;
   item->etime = time + scopebuf_timeinc;
   item->nchan = m_format.uChannels;
   item->bps = bytes_per_sample;
   item->spb = len / item->nchan;
   item->spb /= bytes_per_sample;
   item->tps = (double) scopebuf_timeinc / (double) item->spb;
   m_Player->addScopeBuf(item);
}

#ifdef __i386__
/* Round function provided by Frank Klemm which saves around 100K
 * CPU cycles in my PIII for each call to the IIR function with 4K samples
 */
__inline__ static int round_trick(float floatvalue_to_round)
{
    float   floattmp;
    int     rounded_value;

    floattmp      = (int) 0x00FD8000L + (floatvalue_to_round);
    rounded_value = *(int*)(&floattmp) - (int)0x4B7D8000L;

    if ( rounded_value != (short) rounded_value )
        rounded_value = ( rounded_value >> 31 ) ^ 0x7FFF;
    return rounded_value;
}
#endif

void HSPPostMixAudioHook::updateEQgains(int pamp, vector<int> &equalizerGains)
{
   for (int i=0; i<EQ_CHANNELS; i++)
   {
      preamp[i] = (float) pamp * 0.01;


      for (int j=0; j<EQ_MAX_BANDS; j++)
         gain[j][i] = (float)(equalizerGains[j]) * 0.012 - 0.2;         
   }
}


void HSPPostMixAudioHook::equalize(unsigned char *inbuf, unsigned char *outbuf, size_t length)
{
   int index, band, channel;
   int tempint, halflength;
   float out[EQ_CHANNELS], pcm[EQ_CHANNELS];
   short int *data = (short int *) inbuf;
   short int *dataout = (short int *) outbuf;

   /**
    * IIR filter equation is
    * y[n] = 2 * (alpha*(x[n]-x[n-2]) + gamma*y[n-1] - beta*y[n-2])
    *
    * NOTE: The 2 factor was introduced in the coefficients to save
    * 			a multiplication
    *
    * This algorithm cascades two filters to get nice filtering
    * at the expense of extra CPU cycles
    */
   /* 16bit, 2 bytes per sample, so divide by two the length of
    * the buffer (length is in bytes)
    */
   halflength = (length >> 1);
   for (index = 0; index < halflength; index+=m_format.uChannels)
   {
      /* For each channel */
      for (channel = 0; channel < m_format.uChannels; channel++)
      {
         pcm[channel] = (float) data[index+channel];
         
         /* Preamp gain */
         pcm[channel] *= preamp[channel];
         
         out[channel] = 0.;
         /* For each band */
         for (band = 0; band < BAND_NUM; band++)
         {
            /* Store Xi(n) */
            data_history[band][channel].x[m_i] = pcm[channel];
            /* Calculate and store Yi(n) */
            data_history[band][channel].y[m_i] =
               (
                  /* = alpha * [x(n)-x(n-2)] */
                  iir_cf[band].alpha * ( data_history[band][channel].x[m_i]
                   -  data_history[band][channel].x[m_k])
                  /* + gamma * y(n-1) */
                  + iir_cf[band].gamma * data_history[band][channel].y[m_j]
                  /* - beta * y(n-2) */
                  - iir_cf[band].beta * data_history[band][channel].y[m_k]
                  );
            /*
             * The multiplication by 2.0 was 'moved' into the coefficients to save
             * CPU cycles here */
            /* Apply the gain  */
            out[channel] +=  data_history[band][channel].y[m_i]*gain[band][channel]; // * 2.0;
         } /* For each band */
         
         /* Volume stuff
            Scale down original PCM sample and add it to the filters
            output. This substitutes the multiplication by 0.25
            Go back to use the floating point multiplication before the
            conversion to give more dynamic range
         */
         out[channel] += pcm[channel]*0.25;
         
         /* Round and convert to integer */
#ifdef __i386__
         tempint = round_trick(out[channel]);
#else
         tempint = (int)out[channel];
#endif
         /* Limit the output */
         if (tempint < -32768)
            dataout[index+channel] = -32768;
         else if (tempint > 32767)
            dataout[index+channel] = 32767;
         else
            dataout[index+channel] = tempint;
      } /* For each channel */
      
      m_i++; m_j++; m_k++;
      
      /* Wrap around the indexes */
      if (m_i == 3) m_i = 0;
      else if (m_j == 3) m_j = 0;
      else m_k = 0;
   }/* For each pair of samples */
}


#ifndef HELIX_SW_VOLUME_INTERFACE
int HSPPostMixAudioHook::volumeize(unsigned char *data, size_t len)
{
   gainFeed(data, data, len, m_gaintool);

   return len;
}


int HSPPostMixAudioHook::volumeize(unsigned char *data, unsigned char *outbuf, size_t len)
{
   gainFeed(data, outbuf, len, m_gaintool);

   return len;
}
#endif

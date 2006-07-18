/******************************************************************************
 *                                                                            *
 *   This program is free software; you can redistribute it and/or modify     *
 *   it under the terms of the GNU General Public License as published by     *
 *   the Free Software Foundation; either version 2 of the License, or        *
 *   (at your option) any later version.                                      *
 *                                                                            *
 *   This library is distributed in the hope that it will be useful, but      *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of               *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU        *
 *   Lesser General Public License for more details.                          *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with this library; if not, write to the Free Software      *
 *   Foundation, Inc., 51 Franklin St, 5th fl, Boston, MA 02110-1301,         *
 *   USA, or check http://www.fsf.org/about/contact.html                      *
 *                                                                            *
 *   Portions Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved. *
 *   Portions Copyright (c) 2005 Paul Cifarelli                               *
 *                                                                            *
 ******************************************************************************/

#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>

#include <stdio.h> 
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#include <config.h>

#include "hxcomm.h"
#include "hxcore.h"
#include "hxprefs.h"
#include "hxstrutl.h"
#include "hxvsrc.h"
#include "hxresult.h"
#include "hxausvc.h"
#include "helix-sp.h"

#include "ihxpckts.h"
#include "hxprefs.h"
#include "hspalsadevice.h"

#ifdef HX_LOG_SUBSYSTEM
#include "hxtlogutil.h"
#include "ihxtlogsystem.h"
#endif

#include "dllpath.h"

#include "hxbuffer.h"

#ifdef USE_HELIX_ALSA

IHXPreferences* z_pIHXPrefs = 0;
#define RA_AOE_NOERR         0
#define RA_AOE_GENERAL      -1
#define RA_AOE_DEVNOTOPEN   -2
#define RA_AOE_NOTENABLED   -3
#define RA_AOE_BADFORMAT    -4
#define RA_AOE_NOTSUPPORTED -5
#define RA_AOE_DEVBUSY      -6
#define RA_AOE_BADOPEN      -7

#ifdef __FreeBSD__
#define PTHREAD_MUTEX_FAST_NP PTHREAD_MUTEX_NORMAL
#endif

#if !defined(__NetBSD__) && !defined(__OpenBSD__)
	#include <sys/soundcard.h>
#else
	#include <soundcard.h>
#endif

typedef HX_RESULT (HXEXPORT_PTR FPRMSETDLLACCESSPATH) (const char*);


AudioQueue::AudioQueue( const HXAudioData *buf) : fwd(0)
{
   ad = *buf;
   ad.pData->AddRef();
}

AudioQueue::~AudioQueue()
{
   ad.pData->Release();
}


STDMETHODIMP
HSPAudioDevice::QueryInterface(REFIID riid, void**ppvObj)
{
    if(IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXAudioDevice *)this;
        return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXAudioDevice))
    {
        AddRef();
        *ppvObj = (IHXAudioDevice *)this;
        return HXR_OK;
    }
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(UINT32)
HSPAudioDevice::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(UINT32)
HSPAudioDevice::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP
HSPAudioDevice::CheckFormat( const HXAudioFormat* pAudioFormat )
{
   m_Player->print2stderr("########## Got to HSPAudioDevice::CheckFormat\n");

   return (_CheckFormat(pAudioFormat));
}

STDMETHODIMP
HSPAudioDevice::Close( const BOOL bFlush )
{
   m_Player->print2stderr("########## Got to HSPAudioDevice::Close flush %d\n", bFlush);

   pthread_mutex_lock(&m_m);

   if (bFlush)
   {
      clearQueue();
      _Drain();
   }

   _Reset();
   _CloseAudio();
   _CloseMixer();

   m_closed = true;

   m_ulCurrentTime = m_ulQTime = 0;

   if (m_pStreamResponse)
      m_pStreamResponse->Release();

   
   pthread_mutex_unlock(&m_m);
   
   return 0;
}
 
STDMETHODIMP
HSPAudioDevice::Drain()
{
   m_Player->print2stderr("########## Got to HSPAudioDevice::Drain\n");
   pthread_mutex_lock(&m_m);

   LONG32 err = _Drain();
   clearQueue();
   pthread_mutex_unlock(&m_m);
   return err;
}

STDMETHODIMP
HSPAudioDevice::GetCurrentAudioTime( REF(ULONG32) ulCurrentTime )
{
   //m_Player->print2stderr("########## Got to HSPAudioDevice::GetCurrentTime = %d\n", m_ulCurrentTime);

   int err = 0;
   snd_pcm_sframes_t frame_delay = 0;

   pthread_mutex_lock(&m_m);
   if (!m_closed)
   {
      err = snd_pcm_delay (m_pAlsaPCMHandle, &frame_delay);
      if (err < 0)
      {
#ifdef HX_LOG_SUBSYSTEM
         HXLOGL1 ( HXLOG_ADEV, "snd_pcm_status: %s", snd_strerror(err));        
#endif
         m_Player->print2stderr("########## HSPAudioDevice::GetCurrentAudioTime error getting frame_delay: %s\n", snd_strerror(err));
         pthread_mutex_unlock(&m_m);
         return -1;
      }

      ulCurrentTime = m_ulCurrentTime - (ULONG32)(((double)frame_delay * 1000.0) / (double)m_unSampleRate);

      //m_Player->print2stderr("########## HSPAudioDevice::GetCurrentAudioTime %d %d\n", ulCurrentTime, m_ulCurrentTime);
   }
   pthread_mutex_unlock(&m_m);

   return 0;
}

STDMETHODIMP_(UINT16)
HSPAudioDevice::GetVolume()
{
   m_Player->print2stderr("########## Got to HSPAudioDevice::GetVolume\n");
   return 0;
}

STDMETHODIMP_(BOOL)
   HSPAudioDevice::InitVolume(const UINT16 /*uMinVolume*/, const UINT16 /*uMaxVolume*/)
{
   m_Player->print2stderr("########## Got to HSPAudioDevice::InitVolume\n");
   return true;
}

STDMETHODIMP
HSPAudioDevice::Open(const HXAudioFormat* pAudioFormat, IHXAudioDeviceResponse* pStreamResponse)
{
   int err;

   m_Player->print2stderr("########## Got to HSPAudioDevice::Open\n");
   if (pStreamResponse)
      pStreamResponse->AddRef();

   pthread_mutex_lock(&m_m);  

   m_drain = false;
   m_closed = false;
   m_ulTotalWritten = 0;
   m_ulCurrentTime = 0;
   m_SWPause = false;
   m_pStreamResponse = pStreamResponse;
   if (!m_pAlsaPCMHandle)
   {
      err = _OpenAudio();
      if (err) m_Player->print2stderr("########## HSPAudioDevice::Open error (device) %d\n", err);
      err = SetDeviceConfig(pAudioFormat);
      if (err) m_Player->print2stderr("########## HSPAudioDevice::Open error (config) %d\n", err);
      m_ulCurrentTime = m_ulLastTime = m_ulQTime = 0;
   }

    if (m_pAlsaMixerHandle != NULL)
    {
       err = _OpenMixer();
       if (err) m_Player->print2stderr("########## HSPAudioDevice::Open error (mixer) %d\n", err);
    }

   pthread_mutex_unlock(&m_m);

   return 0;
}

STDMETHODIMP
HSPAudioDevice::Pause()
{
   m_Player->print2stderr("########## Got to HSPAudioDevice::Pause %d\n", m_bHasHardwarePauseAndResume);
   _Pause();
   return 0;
}

STDMETHODIMP
HSPAudioDevice::Reset()
{
   m_Player->print2stderr("########## Got to HSPAudioDevice::Reset\n");
   return (_Reset());
}

STDMETHODIMP
HSPAudioDevice::Resume()
{
   m_Player->print2stderr("########## Got to HSPAudioDevice::Resume\n");
   _Resume();

   return 0;
}

STDMETHODIMP
HSPAudioDevice::SetVolume( const UINT16 /*uVolume*/ )
{
   m_Player->print2stderr("########## Got to HSPAudioDevice::SetVolume\n");
   return 0;
}

STDMETHODIMP
HSPAudioDevice::Write( const HXAudioData* pAudioData )
{
   addBuf( new AudioQueue( pAudioData ) );
   return 0;
}

int HSPAudioDevice::sync()
{
   if (m_pStreamResponse)
   {
      ULONG32 curtime;
      if (!GetCurrentAudioTime(curtime) && curtime)
         return m_pStreamResponse->OnTimeSync(curtime);
      else
      {
         // probably a seek occurred
         //clearQueue();
         _Reset();
      }
   }
   return -1;
}


HX_RESULT HSPAudioDevice::OnTimeSync()
{
   HX_RESULT err;

   if (!(err = sync()))
      return HXR_OK;

   return err;
}

int
HSPAudioDevice::_Write( const HXAudioData* pAudioData )
{
   unsigned long len;
   long bytes;
   unsigned char *data;
   int err = 0;
   
   pAudioData->pData->Get(data, len);

   // if the time of this buf is earlier than the last, or the time between this buf and the last is > 1 buffer's worth, this was a seek
   if ( pAudioData->ulAudioTime < m_ulCurrentTime ||
        pAudioData->ulAudioTime - m_ulCurrentTime > (1000 * len) / (m_unNumChannels * m_unSampleRate) + 1 ) 
   {
      m_Player->print2stderr("########## seek detected %ld %ld, len = %ld %d\n", m_ulCurrentTime, pAudioData->ulAudioTime, len,
                             abs(pAudioData->ulAudioTime - (m_ulCurrentTime + (1000 * len) / (m_unNumChannels * m_unSampleRate))));
      //_Reset();
      //clearQueue();
   }

   if (!err)
   {
      err = WriteBytes(data, len, bytes);
      m_ulCurrentTime = pAudioData->ulAudioTime;
   }
   err = sync();

   //m_Player->print2stderr("########## %d %d\n", m_ulCurrentTime,pAudioData->ulAudioTime);

   //m_Player->print2stderr("########## Got to HSPAudioDevice::Write len=%d  byteswriten=%d err=%d time=%d\n",
   //                       len,bytes,err,m_ulCurrentTime);

   return err;
}


//------------------------------------------
// Ctors and Dtors.
//------------------------------------------
HSPAudioDevice::HSPAudioDevice(HelixSimplePlayer *player, const char *device) :
    m_pAlsaPCMHandle (NULL),
    m_pAlsaMixerHandle (NULL),
    m_pAlsaMixerElem (NULL),

    m_pPCMDeviceName (NULL),
    m_pMixerDeviceName (NULL),
    m_pMixerElementName (NULL),

    m_bHasHardwarePauseAndResume (FALSE),
    m_nBytesPlayedBeforeLastTrigger(0),

    m_nLastBytesPlayed(0),

    m_bGotInitialTrigger(FALSE),
    m_bUseMMAPTStamps(TRUE),
    m_lRefCount(0),
    m_wLastError(0),
    m_SWPause(false),
    m_Player(player),
    m_done(false),
    m_drain(false),
    m_closed(true),
    m_head(0),
    m_tail(0)
{
   pthread_mutexattr_t ma;

   pthread_mutexattr_init(&ma);
   pthread_mutexattr_settype(&ma, PTHREAD_MUTEX_FAST_NP); // note this is not portable outside linux and a few others
   pthread_mutex_init(&m_m, &ma);

   pthread_cond_init(&m_cv, NULL);

   // create thread that will wait for buffers to appear to send to the device
   pthread_create(&m_thrid, 0, writerThread, this);

   if (device)
   {
      int len = strlen( device );
      m_Player->pCommonClassFactory->CreateInstance(CLSID_IHXBuffer, (void **) &m_pPCMDeviceName);
      if (m_pPCMDeviceName)
         m_pPCMDeviceName->Set( (const unsigned char*) device, len + 1 );
   }
}

HSPAudioDevice::~HSPAudioDevice()
{
   pthread_mutex_lock(&m_m);
   m_done = true;
   pthread_mutex_unlock(&m_m);    
   pthread_cond_signal(&m_cv);
   void *tmp;
   pthread_join(m_thrid, &tmp);

   
   if(m_pPCMDeviceName)
   {
      HX_RELEASE(m_pPCMDeviceName);
   }
   
   if(m_pMixerDeviceName)
   {
      HX_RELEASE(m_pMixerDeviceName);
   }
   
   if(m_pMixerElementName)
   {
      HX_RELEASE(m_pMixerElementName);
   }
   
   pthread_cond_destroy(&m_cv);
   pthread_mutex_destroy(&m_m);
}

void HSPAudioDevice::addBuf(struct AudioQueue *item)
{
   pthread_mutex_lock(&m_m);

   m_ulQTime = item->ad.ulAudioTime;
   if (m_tail)
   {
      item->fwd = 0;
      m_tail->fwd = item;
      m_tail = item;
   }
   else
   {
      item->fwd = 0;
      m_head = item;
      m_tail = item;
   }

   pthread_mutex_unlock(&m_m);
   pthread_cond_signal(&m_cv);
}

AudioQueue *HSPAudioDevice::getBuf()
{
   pthread_mutex_lock(&m_m);
   
   AudioQueue *item = m_head;
   
   if (item)
   {
      m_head = item->fwd;
      if (!m_head)
         m_tail = 0;
   }
   
   pthread_mutex_unlock(&m_m);
      
   return item;
}

// NOTE THAT THIS IS NOT UNDER LOCK, AND SHOULD ONLY BE CALLED WITH THE MUTEX LOCKED
void HSPAudioDevice::clearQueue()
{
   AudioQueue *item;

   if (!m_tail)
      return;

   while (m_tail)
   {
      item = m_head;
      m_head = item->fwd;
      if (!m_head)
         m_tail = 0;
      delete item;
   } 
}


void *HSPAudioDevice::writerThread( void *arg )
{
   HSPAudioDevice *thisObj = (HSPAudioDevice *) arg;
   AudioQueue *item;

   pthread_mutex_lock(&thisObj->m_m);
   while (!thisObj->m_done)
   {
      pthread_mutex_unlock(&thisObj->m_m);
      item = thisObj->getBuf();

      if (item)
         thisObj->_Write(&item->ad);

      delete item;

      pthread_mutex_lock(&thisObj->m_m);
      if (!thisObj->m_tail)
         pthread_cond_wait(&thisObj->m_cv, &thisObj->m_m);
   }
   pthread_mutex_unlock(&thisObj->m_m);

   thisObj->m_Player->print2stderr("############ writerThread exit\n");
   return 0;
}


// These Device Specific methods must be implemented
// by the platform specific sub-classes.
INT16 HSPAudioDevice::GetAudioFd(void)
{
    //Not implemented.
    return -1;
}


//Device specific methods to open/close the mixer and audio devices.
HX_RESULT HSPAudioDevice::_OpenAudio()
{
    int err = 0;
    const char* szDevice;

    HX_ASSERT (m_pAlsaPCMHandle == NULL);
    if (m_pAlsaPCMHandle)
    {
        m_wLastError = RA_AOE_BADOPEN;
        return m_wLastError;
    }

    if(z_pIHXPrefs)
    {
        HX_RELEASE(m_pPCMDeviceName);
        z_pIHXPrefs->ReadPref("AlsaPCMDeviceName", m_pPCMDeviceName);
    }

    if(!m_pPCMDeviceName)
    {
        const char szDefaultDevice[] = "default";

        m_Player->pCommonClassFactory->CreateInstance(CLSID_IHXBuffer, (void **) &m_pPCMDeviceName);
        if (m_pPCMDeviceName)
           m_pPCMDeviceName->Set( (const unsigned char*) szDefaultDevice, sizeof(szDefaultDevice) );
    }

    szDevice = (const char*) m_pPCMDeviceName->GetBuffer();
    m_Player->print2stderr("###########  Opening ALSA PCM device %s\n", szDevice);

#ifdef HX_LOG_SUBSYSTEM
    HXLOGL2 (HXLOG_ADEV, "Opening ALSA PCM device %s", 
             szDevice);
#endif
    
    err = snd_pcm_open( &m_pAlsaPCMHandle, 
                        szDevice, 
                        SND_PCM_STREAM_PLAYBACK, 
                        0);
    if(err < 0)
    {
        m_Player->print2stderr("########### snd_pcm_open: %s %s\n", szDevice, snd_strerror (err));
#ifdef HX_LOG_SUBSYSTEM
        HXLOGL1 ( HXLOG_ADEV, "snd_pcm_open: %s",
                  szDevice, snd_strerror (err));
#endif

        m_wLastError = RA_AOE_BADOPEN;
    }

    if(err == 0)
    {
        err = snd_pcm_nonblock(m_pAlsaPCMHandle, TRUE);
        if(err < 0)
        {
           m_Player->print2stderr("########## snd_pcm_nonblock: %s\n", snd_strerror (err));
#ifdef HX_LOG_SUBSYSTEM
            HXLOGL1 ( HXLOG_ADEV, "snd_pcm_nonblock: %s",
                      snd_strerror (err));
#endif
            m_wLastError = RA_AOE_BADOPEN;
        }
    }

    if(err == 0)
    {
       m_Player->print2stderr("########## return from OpenAudio\n");
       m_wLastError = RA_AOE_NOERR;
    }
    else
    {
        if(m_pAlsaPCMHandle)
        {
            snd_pcm_close(m_pAlsaPCMHandle);
            m_pAlsaPCMHandle = NULL;
        }
    }

    return m_wLastError;
}


HX_RESULT HSPAudioDevice::_CloseAudio()
{
    if (!m_pAlsaPCMHandle)
    {
        m_wLastError = RA_AOE_DEVNOTOPEN;
        return m_wLastError;
    }

#ifdef HX_LOG_SUBSYSTEM
    HXLOGL2 (HXLOG_ADEV, "Closing ALSA PCM device");
#endif
   
    snd_pcm_close(m_pAlsaPCMHandle);
    m_pAlsaPCMHandle = NULL;
    m_wLastError = RA_AOE_NOERR;

    return m_wLastError;
}


HX_RESULT HSPAudioDevice::_OpenMixer()
{
    int err;
    const char* szDeviceName = NULL;
    const char* szElementName = NULL;
    int nElementIndex = 0;

    HX_ASSERT (m_pAlsaMixerHandle == NULL);
    if (m_pAlsaMixerHandle != NULL)
    {
        m_wLastError = RA_AOE_BADOPEN;
        return m_wLastError;
    }

    HX_ASSERT(m_pAlsaMixerElem == NULL);
    if (m_pAlsaMixerElem != NULL)
    {
        m_wLastError = RA_AOE_BADOPEN;
        return m_wLastError;
    }

    if(z_pIHXPrefs)
    {
        HX_RELEASE(m_pMixerDeviceName);
        z_pIHXPrefs->ReadPref("AlsaMixerDeviceName", m_pMixerDeviceName);
    }

    if(!m_pMixerDeviceName)
    {
        const char szDefaultDevice[] = "default";

        m_Player->pCommonClassFactory->CreateInstance(CLSID_IHXBuffer, (void **) &m_pMixerDeviceName);
        if (m_pMixerDeviceName)
           m_pMixerDeviceName->Set( (const unsigned char*) szDefaultDevice, sizeof(szDefaultDevice) );
    }

    if(z_pIHXPrefs)
    {
        HX_RELEASE(m_pMixerElementName);        
        z_pIHXPrefs->ReadPref("AlsaMixerElementName", m_pMixerElementName);
    }

    if(!m_pMixerElementName)
    {
        const char szDefaultElement[] = "PCM";

        m_Player->pCommonClassFactory->CreateInstance(CLSID_IHXBuffer, (void **) &m_pMixerElementName);
        if (m_pMixerElementName)
           m_pMixerElementName->Set( (const unsigned char*) szDefaultElement, sizeof(szDefaultElement) );
    }

    if(z_pIHXPrefs)
    {
        IHXBuffer* pElementIndex = NULL;
        z_pIHXPrefs->ReadPref("AlsaMixerElementIndex", pElementIndex);
        if(pElementIndex)
        {
            const char* szElementIndex = (const char*) pElementIndex->GetBuffer();
            nElementIndex = atoi(szElementIndex);

            HX_RELEASE(pElementIndex);        
        }
    }

    szDeviceName  = (const char*) m_pMixerDeviceName->GetBuffer();;
    szElementName = (const char*) m_pMixerElementName->GetBuffer();
    
#ifdef HX_LOG_SUBSYSTEM
    HXLOGL2 (HXLOG_ADEV, "Opening ALSA mixer device %s", 
             szDeviceName);
#endif

    err = snd_mixer_open(&m_pAlsaMixerHandle, 0);
    if (err < 0)
    {
#ifdef HX_LOG_SUBSYSTEM
        HXLOGL1 ( HXLOG_ADEV, "snd_mixer_open: %s",
                  snd_strerror (err));
#endif

        m_wLastError = RA_AOE_BADOPEN;
    }
  
    if (err == 0)
    {
        err = snd_mixer_attach(m_pAlsaMixerHandle, szDeviceName);
        if (err < 0) 
        {
#ifdef HX_LOG_SUBSYSTEM
            HXLOGL1 ( HXLOG_ADEV, "snd_mixer_attach: %s",
                      snd_strerror (err));
#endif

            m_wLastError = RA_AOE_BADOPEN;
        }
    }

    if (err == 0)
    {
        err = snd_mixer_selem_register(m_pAlsaMixerHandle, NULL, NULL);
        if (err < 0) 
        {
#ifdef HX_LOG_SUBSYSTEM
            HXLOGL1 ( HXLOG_ADEV, "snd_mixer_selem_register: %s",
                      snd_strerror (err));
#endif
            m_wLastError = RA_AOE_BADOPEN;
        }
    }

    if (err == 0)
    {
        err = snd_mixer_load(m_pAlsaMixerHandle);
        if(err < 0 )
        {
#ifdef HX_LOG_SUBSYSTEM
            HXLOGL1 ( HXLOG_ADEV, "snd_mixer_load: %s",
                      snd_strerror (err));
#endif

            m_wLastError = RA_AOE_NOTENABLED;
        }
    }

    if (err == 0)
    {
        /* Find the mixer element */
        snd_mixer_elem_t* fallback_elem = NULL;
        snd_mixer_elem_t* elem = snd_mixer_first_elem (m_pAlsaMixerHandle);
        snd_mixer_elem_type_t type;
        const char* elem_name = NULL;
        snd_mixer_selem_id_t *sid = NULL;
        int index;

        snd_mixer_selem_id_alloca(&sid);

        while (elem)
        {
            type = snd_mixer_elem_get_type(elem);
            if (type == SND_MIXER_ELEM_SIMPLE)
            {
                snd_mixer_selem_get_id(elem, sid);

                /* We're only interested in playback volume controls */
                if(snd_mixer_selem_has_playback_volume(elem) &&
                   !snd_mixer_selem_has_common_volume(elem))
                {
                    if (!fallback_elem)
                    {
                        fallback_elem = elem;
                    }

                    elem_name = snd_mixer_selem_id_get_name (sid);
                    index = snd_mixer_selem_id_get_index(sid);
                    if (strcmp(elem_name, szElementName) == 0 &&
                        index == nElementIndex)
                    {
                        break;
                    }
                }
            }
            
            elem = snd_mixer_elem_next(elem);
        }

        if (!elem && fallback_elem)
        {
            elem = fallback_elem;
            elem_name = NULL;
            type = snd_mixer_elem_get_type(elem);
            
            if (type == SND_MIXER_ELEM_SIMPLE)
            {
                snd_mixer_selem_get_id(elem, sid);
                elem_name = snd_mixer_selem_id_get_name (sid);
            }

#ifdef HX_LOG_SUBSYSTEM
            HXLOGL1 ( HXLOG_ADEV, "Could not find element %s, using element %s instead",
                     m_pMixerElementName,  elem_name? elem_name: "unknown");
#endif
        }
        else if (!elem)
        {
#ifdef HX_LOG_SUBSYSTEM
            HXLOGL1 ( HXLOG_ADEV, "Could not find a usable mixer element",
                      snd_strerror (err));
#endif
            m_wLastError = RA_AOE_BADOPEN;
            err = -1;
        }

        m_pAlsaMixerElem = elem;
    }
    
    if(err == 0)
    {
        if (m_pAlsaMixerHandle)
        {
            m_bMixerPresent = 1;
            _GetVolume();
        }
        else
        {
            m_bMixerPresent = 0;
        }

        m_wLastError = RA_AOE_NOERR;
    }
    else
    {
        if(m_pAlsaMixerHandle)
        {
            snd_mixer_close(m_pAlsaMixerHandle);
            m_pAlsaMixerHandle = NULL;
        }
    }

    return m_wLastError;
}

HX_RESULT HSPAudioDevice::_CloseMixer()
{
    int err;
    const char* szMixerDeviceName = NULL;

    if (!m_pAlsaMixerHandle)
    {
        m_wLastError = RA_AOE_DEVNOTOPEN;
        return m_wLastError;
    }

    if (!m_pMixerDeviceName)
    {
        m_wLastError = RA_AOE_DEVNOTOPEN;
        return m_wLastError;        
    }

    szMixerDeviceName = (const char*) m_pMixerDeviceName->GetBuffer();
    err = snd_mixer_detach(m_pAlsaMixerHandle, szMixerDeviceName);
    if (err < 0)
    {
#ifdef HX_LOG_SUBSYSTEM
        HXLOGL1 ( HXLOG_ADEV, "snd_mixer_detach: %s",
                  snd_strerror (err));
#endif
        m_wLastError = RA_AOE_GENERAL;
    }
  
    if(err == 0)
    {
        err = snd_mixer_close(m_pAlsaMixerHandle);
        if(err < 0)
        {
#ifdef HX_LOG_SUBSYSTEM
            HXLOGL1 ( HXLOG_ADEV, "snd_mixer_close: %s",
                      snd_strerror (err));            
#endif
            m_wLastError = RA_AOE_GENERAL;
        }
    }

    if(err == 0)
    {
        m_pAlsaMixerHandle = NULL;
        m_pAlsaMixerElem = NULL;
        m_wLastError = RA_AOE_NOERR;
    }

    return m_wLastError;
}


//Device specific method to set the audio device characteristics. Sample rate,
//bits-per-sample, etc.
//Method *must* set member vars. m_unSampleRate and m_unNumChannels.
HX_RESULT HSPAudioDevice::SetDeviceConfig( const HXAudioFormat* pFormat )
{
    snd_pcm_state_t state;

    HX_ASSERT(m_pAlsaPCMHandle != NULL);
    if (!m_pAlsaPCMHandle)
    {
        m_wLastError = RA_AOE_DEVNOTOPEN;
        return m_wLastError;
    }

    state = snd_pcm_state(m_pAlsaPCMHandle);
    if (state != SND_PCM_STATE_OPEN)
    {
#ifdef HX_LOG_SUBSYSTEM
        HXLOGL1 ( HXLOG_ADEV, "Device is not in open state in HSPAudioDevice::SetDeviceConfig (%d)", (int) state);
#endif
        m_wLastError = RA_AOE_DEVNOTOPEN;
        return m_wLastError;
    }

    /* Translate from HXAudioFormat to ALSA-friendly values */
    snd_pcm_format_t fmt;
    unsigned int sample_rate = 0;
    unsigned int channels = 0;
    unsigned int buffer_time = 500000;          /* 0.5 seconds */
    unsigned int period_time = buffer_time / 4; /* 4 interrupts per buffer */

    switch (pFormat->uBitsPerSample)
    {
    case 8:
        fmt = SND_PCM_FORMAT_S8;
        break;

    case 16:
        fmt = SND_PCM_FORMAT_S16_LE;        
        break;

    case 24:
        fmt = SND_PCM_FORMAT_S24_LE;
        break;

    case 32:
        fmt = SND_PCM_FORMAT_S32_LE;
        break;

    default:
        fmt = SND_PCM_FORMAT_UNKNOWN;
        break;        
    }
    
    if (fmt == SND_PCM_FORMAT_UNKNOWN)
    {
#ifdef HX_LOG_SUBSYSTEM
        HXLOGL1 ( HXLOG_ADEV, "Unknown bits per sample: %d", pFormat->uBitsPerSample);
#endif
        m_wLastError = RA_AOE_NOTENABLED;
        return m_wLastError;
    }
    sample_rate = pFormat->ulSamplesPerSec;
    channels = pFormat->uChannels;

    /* Apply to ALSA */
    int err = 0;
    snd_pcm_hw_params_t *hwparams;
    snd_pcm_sw_params_t *swparams;
    
    snd_pcm_hw_params_alloca(&hwparams);
    snd_pcm_sw_params_alloca(&swparams);

    /* Hardware parameters */
    err = snd_pcm_hw_params_any(m_pAlsaPCMHandle, hwparams);
    if (err < 0) 
    {
#ifdef HX_LOG_SUBSYSTEM
       HXLOGL1 ( HXLOG_ADEV, "snd_pcm_hw_params_any: %s", snd_strerror(err));
#endif
       m_wLastError = RA_AOE_NOTENABLED;
    }

    if (err == 0)
    {
       err = snd_pcm_hw_params_set_access(m_pAlsaPCMHandle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED);
       if (err < 0) 
       {
#ifdef HX_LOG_SUBSYSTEM
          HXLOGL1 ( HXLOG_ADEV, "snd_pcm_hw_params_set_access: %s", snd_strerror(err));
#endif
          m_wLastError = RA_AOE_NOTENABLED;
       }
    }

    if (err == 0)
    {
       err = snd_pcm_hw_params_set_format(m_pAlsaPCMHandle, hwparams, fmt);
       if (err < 0) 
       {
#ifdef HX_LOG_SUBSYSTEM
          HXLOGL1 ( HXLOG_ADEV, "snd_pcm_hw_params_set_format: %s", snd_strerror(err));
#endif
          m_wLastError = RA_AOE_NOTENABLED;
       }
    }

    if (err == 0)
    {
       err = snd_pcm_hw_params_set_channels(m_pAlsaPCMHandle, hwparams, channels);
       if (err < 0) 
       {
#ifdef HX_LOG_SUBSYSTEM
          HXLOGL1 ( HXLOG_ADEV, "snd_pcm_hw_params_set_channels: %s", snd_strerror(err));
#endif
          m_wLastError = RA_AOE_NOTENABLED;
       }
    }
    
    if (err == 0)
    {
       unsigned int sample_rate_out;
       sample_rate_out = sample_rate;
       
       err = snd_pcm_hw_params_set_rate_near(m_pAlsaPCMHandle, hwparams, &sample_rate_out, 0);
       if (err < 0) 
       {
#ifdef HX_LOG_SUBSYSTEM
          HXLOGL1 ( HXLOG_ADEV, "snd_pcm_hw_params_set_channels: %s", snd_strerror(err));
#endif
          m_wLastError = RA_AOE_NOTENABLED;
       }
       
       if (sample_rate_out != sample_rate)
       {
#ifdef HX_LOG_SUBSYSTEM
          HXLOGL2 ( HXLOG_ADEV, "Requested a sample rate of %d, got a rate of %d", 
                    sample_rate, sample_rate_out);
#endif
          
          sample_rate = sample_rate_out;
       }
    }
    
    if (err == 0)
    {
       unsigned int buffer_time_out;
       buffer_time_out = buffer_time;
       
       err = snd_pcm_hw_params_set_buffer_time_near(m_pAlsaPCMHandle, hwparams, &buffer_time_out, 0);
       if (err < 0) 
       {
#ifdef HX_LOG_SUBSYSTEM
          HXLOGL1 ( HXLOG_ADEV, "snd_pcm_hw_params_set_buffer_time_near: %s", 
                    snd_strerror(err));
#endif
          m_wLastError = RA_AOE_NOTENABLED;
       }
       
       if (buffer_time_out != buffer_time)
       {
#ifdef HX_LOG_SUBSYSTEM
          HXLOGL2 ( HXLOG_ADEV, "Requested a buffering time of %d, got a time of %d", 
                    buffer_time, buffer_time_out);
#endif
          
          buffer_time = buffer_time_out;
       }
    }
    
    if (err == 0)
    {
       unsigned int period_time_out;
       period_time_out = period_time;
       
       err = snd_pcm_hw_params_set_period_time_near(m_pAlsaPCMHandle, hwparams, &period_time_out, 0);
       if (err < 0) 
       {
#ifdef HX_LOG_SUBSYSTEM
          HXLOGL1 ( HXLOG_ADEV, "snd_pcm_hw_params_set_period_time_near: %s", 
                    snd_strerror(err));
#endif
          m_wLastError = RA_AOE_NOTENABLED;
       }
       
       if (period_time_out != period_time)
       {
#ifdef HX_LOG_SUBSYSTEM
          HXLOGL2 ( HXLOG_ADEV, "Requested a period time of %d, got a period of %d", 
                    period_time, period_time_out);
#endif
          period_time = period_time_out;
       }
    }
    
    /* Apply parameters */
    err = snd_pcm_hw_params(m_pAlsaPCMHandle, hwparams);
    if (err < 0) 
    {
#ifdef HX_LOG_SUBSYSTEM
       HXLOGL1 ( HXLOG_ADEV, "snd_pcm_hw_params: %s", 
                 snd_strerror(err));
#endif
       m_wLastError = RA_AOE_NOTENABLED;
    }
    
    /* read buffer & period sizes */
    snd_pcm_uframes_t buffer_size = 0;
    snd_pcm_uframes_t period_size = 0;
    
    if (err == 0)
    {
       err = snd_pcm_hw_params_get_buffer_size(hwparams, &buffer_size);
       if (err < 0) 
       {
#ifdef HX_LOG_SUBSYSTEM
          HXLOGL1 ( HXLOG_ADEV, "snd_pcm_hw_params_get_buffer_size: %s", 
                    snd_strerror(err));
#endif
          m_wLastError = RA_AOE_NOTENABLED;
       }
       else
       {
          HX_ASSERT (buffer_size > 0);
       }
    }
    
    if (err == 0)
    {
       err = snd_pcm_hw_params_get_period_size(hwparams, &period_size, 0);
       if (err < 0) 
       {
#ifdef HX_LOG_SUBSYSTEM
          HXLOGL1 ( HXLOG_ADEV, "snd_pcm_hw_params_get_period_size: %s", 
                    snd_strerror(err));
#endif
          m_wLastError = RA_AOE_NOTENABLED;
       }
    }
    
    /* Get hardware pause */
    if (err == 0)
    {
       int can_pause = 0;
       int can_resume = 0;
       
       can_pause = snd_pcm_hw_params_can_pause(hwparams);
       can_resume = snd_pcm_hw_params_can_resume(hwparams);
       
       // could we really have one without the other?
       m_bHasHardwarePauseAndResume = (can_pause && can_resume); 
       m_Player->print2stderr("########## can_pause %d can_resume %d\n", can_pause, can_resume);
    }
    
    /* Software parameters */
    if (err == 0)
    {
       err = snd_pcm_sw_params_current(m_pAlsaPCMHandle, swparams);
       if (err < 0) 
       {
#ifdef HX_LOG_SUBSYSTEM
          HXLOGL1 ( HXLOG_ADEV, "snd_pcm_sw_params_current: %s", 
                    snd_strerror(err));
#endif
          m_wLastError = RA_AOE_NOTENABLED;
       }
    }    
    
    snd_pcm_uframes_t start_threshold = ((buffer_size - 1) / period_size) * period_size;
    
    if (err == 0)
    {
       err = snd_pcm_sw_params_set_start_threshold(m_pAlsaPCMHandle, swparams, start_threshold);
       if (err < 0) 
       {
#ifdef HX_LOG_SUBSYSTEM
          HXLOGL1 ( HXLOG_ADEV, "snd_pcm_sw_params_set_start_threshold: %s", 
                    snd_strerror(err));
#endif
          m_wLastError = RA_AOE_NOTENABLED;
       }
    }
    
    if (err == 0)
    {
       err = snd_pcm_sw_params_set_avail_min(m_pAlsaPCMHandle, swparams, period_size);
       if (err < 0) 
       {
#ifdef HX_LOG_SUBSYSTEM
          HXLOGL1 ( HXLOG_ADEV, "snd_pcm_sw_params_set_avail_min: %s", 
                    snd_strerror(err));
#endif
          m_wLastError = RA_AOE_NOTENABLED;
       }
    }
    
    if (err == 0)
    {
       err = snd_pcm_sw_params_set_xfer_align(m_pAlsaPCMHandle, swparams, 1);
       if (err < 0) 
       {
#ifdef HX_LOG_SUBSYSTEM
          HXLOGL1 ( HXLOG_ADEV, "snd_pcm_sw_params_set_xfer_align: %s", 
                    snd_strerror(err));
#endif
          m_wLastError = RA_AOE_NOTENABLED;
       }
    }
    
    if (err == 0)
    {
       err = snd_pcm_sw_params_set_tstamp_mode(m_pAlsaPCMHandle, swparams, SND_PCM_TSTAMP_MMAP);
       if (err < 0) 
       {
#ifdef HX_LOG_SUBSYSTEM
          HXLOGL1 ( HXLOG_ADEV, "snd_pcm_sw_params_set_xfer_align: %s", 
                    snd_strerror(err));
#endif
          m_wLastError = RA_AOE_NOTENABLED;
       }
    }
    
    if (err == 0)
    {
       err = snd_pcm_sw_params_set_stop_threshold(m_pAlsaPCMHandle, swparams, ~0U);
       if (err < 0) 
       {
#ifdef HX_LOG_SUBSYSTEM
          HXLOGL1 ( HXLOG_ADEV, "snd_pcm_sw_params_set_stop_threshold: %s", 
                    snd_strerror(err));
#endif
          m_wLastError = RA_AOE_NOTENABLED;
       }
    }
    
    if (err == 0)
    {
       err = snd_pcm_sw_params(m_pAlsaPCMHandle, swparams);
       if (err < 0) 
       {
#ifdef HX_LOG_SUBSYSTEM
          HXLOGL1 ( HXLOG_ADEV, "snd_pcm_sw_params: %s", 
                    snd_strerror(err));
#endif
          m_wLastError = RA_AOE_NOTENABLED;
       }
    }
    
    /* If all the calls to this point have succeeded, move to the PREPARE state. 
       We will enter the RUNNING state when we've buffered enough for our start theshold. */
    if (err == 0)
    {
       err = snd_pcm_prepare (m_pAlsaPCMHandle);
       if (err < 0)
       {
#ifdef HX_LOG_SUBSYSTEM
          HXLOGL1 ( HXLOG_ADEV, "snd_pcm_prepare: %s", 
                    snd_strerror(err));
#endif
          m_wLastError = RA_AOE_NOTENABLED;
       }
    }
    
    /* Sanity check: See if we're now in the PREPARE state */
    if (err == 0)
    {
       snd_pcm_state_t state;
       state = snd_pcm_state (m_pAlsaPCMHandle);
       if (state != SND_PCM_STATE_PREPARED)
       {
#ifdef HX_LOG_SUBSYSTEM
          HXLOGL1 ( HXLOG_ADEV, "Expected to be in PREPARE state, actually in state %d", 
                    (int) state);
#endif
          m_wLastError = RA_AOE_NOTENABLED;
       }
    }
    
    /* Use avail to get the alsa buffer size, which is distinct from the hardware buffer 
       size. This will match what GetRoomOnDevice uses. */
    int alsa_buffer_size = 0;
    err = snd_pcm_avail_update(m_pAlsaPCMHandle);
    if(err < 0)
    {
#ifdef HX_LOG_SUBSYSTEM
       HXLOGL1 ( HXLOG_ADEV, "snd_pcm_avail_update: %s", snd_strerror(err));
#endif
    }
    else
    {
       alsa_buffer_size = snd_pcm_frames_to_bytes(m_pAlsaPCMHandle, err);
       err = 0;
    }
    
    if (err == 0)
    {
       m_wLastError = RA_AOE_NOERR;
       
       m_unSampleRate  = sample_rate;
       m_unNumChannels = channels;
       m_wBlockSize    = m_ulBytesPerGran;
       m_ulDeviceBufferSize = alsa_buffer_size;
       m_uSampFrameSize = snd_pcm_frames_to_bytes(m_pAlsaPCMHandle, 1) / channels;
       
#ifdef HX_LOG_SUBSYSTEM
       HXLOGL2 ( HXLOG_ADEV,  "Device Configured:\n");
       HXLOGL2 ( HXLOG_ADEV,  "         Sample Rate: %d",  m_unSampleRate);
       HXLOGL2 ( HXLOG_ADEV,  "        Sample Width: %d",  m_uSampFrameSize);
       HXLOGL2 ( HXLOG_ADEV,  "        Num channels: %d",  m_unNumChannels);
       HXLOGL2 ( HXLOG_ADEV,  "          Block size: %d",  m_wBlockSize);
       HXLOGL2 ( HXLOG_ADEV,  "  Device buffer size: %lu", m_ulDeviceBufferSize);
       HXLOGL2 ( HXLOG_ADEV,  "   Supports HW Pause: %d",  m_bHasHardwarePauseAndResume);
       HXLOGL2 ( HXLOG_ADEV,  "     Start threshold: %d",  start_threshold);
#endif
       
    }
    else
    {
       m_unSampleRate = 0;
       m_unNumChannels = 0;
       
       if (m_pAlsaPCMHandle)
       {
          _CloseAudio();            
       }
    }
    
    return m_wLastError;
}

//Device specific method to write bytes out to the audiodevice and return a
//count of bytes written.
HX_RESULT HSPAudioDevice::WriteBytes( UCHAR* buffer, ULONG32 ulBuffLength, LONG32& lCount )
{
    int err = 0, count = 0;
    unsigned int frames_written = 0;
    snd_pcm_sframes_t num_frames = 0;
    ULONG32 ulBytesToWrite = ulBuffLength;
    ULONG32 ulBytesWrote = 0;

    lCount = 0;

    HX_ASSERT(m_pAlsaPCMHandle);
    if (!m_pAlsaPCMHandle)
    {
        m_wLastError = RA_AOE_DEVNOTOPEN;
        return m_wLastError;
    }

    m_wLastError = RA_AOE_NOERR;

    if (ulBuffLength == 0)
    {
        lCount = ulBuffLength;
        return m_wLastError;
    }


    do
    {
       pthread_mutex_lock(&m_m);
       if (!m_closed)
       {
          if (!m_SWPause)
          {
             num_frames = snd_pcm_bytes_to_frames(m_pAlsaPCMHandle, ulBytesToWrite);
             err = snd_pcm_writei( m_pAlsaPCMHandle, buffer, num_frames );
          }
          else
             err = -EAGAIN;
       }
       else
       {
          pthread_mutex_unlock(&m_m);
          return 0;
       }
       pthread_mutex_unlock(&m_m);
       count++;
       if (err >= 0)
       {
          frames_written = err;
          
          pthread_mutex_lock(&m_m);
          if (!m_closed)
             ulBytesWrote = snd_pcm_frames_to_bytes (m_pAlsaPCMHandle, frames_written);
          pthread_mutex_unlock(&m_m);
          buffer += ulBytesWrote;
          ulBytesToWrite -= ulBytesWrote;
          lCount += ulBytesWrote;

          m_ulTotalWritten += ulBytesWrote;

       }
       else
       {
          switch (err)
          {
             case -EAGAIN:
                usleep(10000);
                break;
                
             case -EPIPE:
                HandleXRun();
                lCount = (LONG32) ulBuffLength;
                break;
                
             case -ESTRPIPE:
                HandleSuspend();
                lCount = (LONG32) ulBuffLength;
                break;
                
             default:
                m_Player->print2stderr("########### snd_pcm_writei: %s  num_frames=%ld\n", snd_strerror(err), num_frames);
#ifdef HX_LOG_SUBSYSTEM
                HXLOGL1 ( HXLOG_ADEV, "snd_pcm_writei: %s", snd_strerror(err));
#endif
                m_wLastError = RA_AOE_DEVBUSY;
          }
       }
    } while (err == -EAGAIN || (err>0 && ulBytesToWrite>0));
    
    //m_Player->print2stderr("############## count = %d\n", count);
    
    return m_wLastError;
}

/* Subtract the `struct timeval' values X and Y,
   storing the result in RESULT.
   Return 1 if the difference is negative, otherwise 0.  */

int
timeval_subtract (struct timeval *result, 
                  const struct timeval *x, 
                  const struct timeval *y_orig)
{
    struct timeval y = *y_orig;
    
    /* Perform the carry for the later subtraction by updating Y. */
    if (x->tv_usec < y.tv_usec) 
    {
        int nsec = (y.tv_usec - x->tv_usec) / 1000000 + 1;
        y.tv_usec -= 1000000 * nsec;
        y.tv_sec += nsec;
    }
    if ((x->tv_usec - y.tv_usec) > 1000000) 
    {
        int nsec = (x->tv_usec - y.tv_usec) / 1000000;
        y.tv_usec += 1000000 * nsec;
        y.tv_sec -= nsec;
    }

    /* Compute the time remaining to wait.
       `tv_usec' is certainly positive. */
    result->tv_sec = x->tv_sec - y.tv_sec;
    result->tv_usec = x->tv_usec - y.tv_usec;

    /* Return 1 if result is negative. */
    return x->tv_sec < y.tv_sec;
}

HX_RESULT HSPAudioDevice::GetBytesActuallyPlayedUsingTStamps(UINT64 &nBytesPlayed) const
{
    HX_RESULT retVal = HXR_FAIL;

    int err = 0;

    snd_timestamp_t trigger_tstamp, now_tstamp, diff_tstamp;
    snd_pcm_status_t* status;

    snd_pcm_status_alloca(&status);
    
    err = snd_pcm_status(m_pAlsaPCMHandle, status);
    if (err < 0)
    {
#ifdef HX_LOG_SUBSYSTEM
        HXLOGL1 ( HXLOG_ADEV, "snd_pcm_status: %s", snd_strerror(err));
#endif
    }

    if (err == 0)
    {
        snd_pcm_status_get_tstamp(status, &now_tstamp);
        snd_pcm_status_get_trigger_tstamp(status, &trigger_tstamp);

        if(!m_bGotInitialTrigger && now_tstamp.tv_sec == 0 && now_tstamp.tv_usec == 0)
        {
            /* Our first "now" timestamp appears to be invalid (or the user is very unlucky, and
               happened to start playback as the timestamp rolls over). Fall back to using 
               snd_pcm_delay. 
              
               XXXRGG: Is there a better way to figure out if the driver supports mmap'd 
               timestamps? */

            m_bUseMMAPTStamps = FALSE;
        }
        else
        {
            /* Timestamp seems to be valid */
            if(!m_bGotInitialTrigger)
            {
                m_bGotInitialTrigger = TRUE;
                memcpy(&m_tstampLastTrigger, &trigger_tstamp, sizeof(m_tstampLastTrigger));
            }
            else
            {
                if(memcmp(&m_tstampLastTrigger, &trigger_tstamp, sizeof(m_tstampLastTrigger)) != 0)
                {
                    /* There's been a trigger since last time -- restart the timestamp counter
                       XXXRGG: What if there's been multiple triggers? */
                    m_nBytesPlayedBeforeLastTrigger = m_nLastBytesPlayed;
                    memcpy(&m_tstampLastTrigger, &trigger_tstamp, sizeof(m_tstampLastTrigger));

#ifdef HX_LOG_SUBSYSTEM
                    HXLOGL1 ( HXLOG_ADEV, "Retriggered...");
#endif
                }
            }

            timeval_subtract (&diff_tstamp, &now_tstamp, &m_tstampLastTrigger);

            double fTimePlayed = (double) diff_tstamp.tv_sec + 
                ((double) diff_tstamp.tv_usec / 1e6);
            
            nBytesPlayed = (UINT64) ((fTimePlayed * (double) m_unSampleRate * m_uSampFrameSize * m_unNumChannels) + m_nBytesPlayedBeforeLastTrigger);
            retVal = HXR_OK;
        }    
    }

    return retVal;
}

HX_RESULT HSPAudioDevice::GetBytesActuallyPlayedUsingDelay (UINT64 &nBytesPlayed) const
{
    HX_RESULT retVal = HXR_FAIL;
    int err = 0;
    snd_pcm_sframes_t frame_delay = 0;

    err = snd_pcm_delay (m_pAlsaPCMHandle, &frame_delay);
    if (err < 0)
    {
#ifdef HX_LOG_SUBSYSTEM
        HXLOGL1 ( HXLOG_ADEV, "snd_pcm_status: %s", snd_strerror(err));        
#endif
    }
    else
    {
        int bytes_delay;
        bytes_delay = snd_pcm_frames_to_bytes (m_pAlsaPCMHandle, frame_delay);

        nBytesPlayed = m_ulTotalWritten - bytes_delay;
        retVal = HXR_OK;
    }

#ifdef HX_LOG_SUBSYSTEM
//    HXLOGL4 ( HXLOG_ADEV, "nBytesPlayed: %llu, m_ulTotalWritten: %llu\n", nBytesPlayed, m_ulTotalWritten);
#endif

    return retVal;
}

HX_RESULT HSPAudioDevice::GetBytesActuallyPlayedUsingAvail(UINT64 &nBytesPlayed) const
{
    /* Try this the hwsync way. This method seems to crash & burn with dmix,
       as avail seems to come from the device, and varies depending on what other
       dmix clients are writing to the slave device. Currently not used for that reason. */

    HX_RESULT retVal = HXR_FAIL;
    int err = 0;

    err = snd_pcm_hwsync(m_pAlsaPCMHandle);
    if(err < 0)
    {
#ifdef HX_LOG_SUBSYSTEM
        HXLOGL1 ( HXLOG_ADEV, "snd_pcm_hwsync: %s", snd_strerror(err));        
#endif
    }

    err = snd_pcm_avail_update(m_pAlsaPCMHandle);
    if(err < 0)
    {
#ifdef HX_LOG_SUBSYSTEM
        HXLOGL1 ( HXLOG_ADEV, "snd_pcm_avail_update: %s", snd_strerror(err));        
#endif
    }
    else
    {
        snd_pcm_sframes_t avail = err;
        int bytes_avail;
        bytes_avail = snd_pcm_frames_to_bytes (m_pAlsaPCMHandle, avail);

        nBytesPlayed = m_ulTotalWritten - (m_ulDeviceBufferSize - bytes_avail);
        retVal = HXR_OK;
    }

    return retVal;
}

HX_RESULT HSPAudioDevice::GetBytesActuallyPlayedUsingTimer(UINT64 &/*nBytesPlayed*/) const
{
    /* Look at the alsa timer api, and how we can lock onto it as a timer source. */

    return HXR_FAIL;
}

UINT64 HSPAudioDevice::GetBytesActualyPlayed(void) const
{
    HX_ASSERT(m_pAlsaPCMHandle);
    if (!m_pAlsaPCMHandle)
    {
        return 0;
    }

    HX_RESULT retVal = HXR_OK;
    UINT64 nBytesPlayed = 0;
    snd_pcm_state_t state;

    for(;;)
    {
        state = snd_pcm_state(m_pAlsaPCMHandle);
        switch(state)
        {
        case SND_PCM_STATE_OPEN:
        case SND_PCM_STATE_SETUP:
        case SND_PCM_STATE_PREPARED:
            /* If we're in one of these states, written and played should match. */
            m_nLastBytesPlayed = m_ulTotalWritten;
            return m_nLastBytesPlayed;

        case SND_PCM_STATE_XRUN:
            HandleXRun();
            continue;
     
        case SND_PCM_STATE_RUNNING:
            break;

        case SND_PCM_STATE_PAUSED:
            // return m_nLastBytesPlayed;
            break;

        case SND_PCM_STATE_DRAINING:
        case SND_PCM_STATE_SUSPENDED:
        case SND_PCM_STATE_DISCONNECTED:
            HX_ASSERT(!"Not reached");
            break;            
        }

        break;
    }

    // XXXRGG: Always use the delay method for now.
    m_bUseMMAPTStamps = FALSE;

    if (m_bUseMMAPTStamps)
    {
        retVal = GetBytesActuallyPlayedUsingTStamps(nBytesPlayed);
    }
 
    if (!m_bUseMMAPTStamps || FAILED(retVal))
    {
        /* MMAP'd timestamps are fishy. Try using snd_pcm_delay. */
        retVal = GetBytesActuallyPlayedUsingDelay(nBytesPlayed);            
    }

    m_nLastBytesPlayed = nBytesPlayed;
    return nBytesPlayed;
}


//this must return the number of bytes that can be written without blocking.
HX_RESULT HSPAudioDevice::GetRoomOnDevice(ULONG32& ulBytes) const
{
    ulBytes = 0;

    HX_ASSERT(m_pAlsaPCMHandle);
    if (!m_pAlsaPCMHandle)
    {
        m_wLastError = RA_AOE_DEVNOTOPEN;
        return m_wLastError;
    }

    int err = 0;
    err = snd_pcm_avail_update(m_pAlsaPCMHandle);
    if(err > 0)
    {
        ulBytes = snd_pcm_frames_to_bytes(m_pAlsaPCMHandle, err);
    }
    else
    {
        switch (err)
        {
        case -EAGAIN:
            break;

        case -EPIPE:
            HandleXRun();
            break;
               
        case -ESTRPIPE:
            HandleSuspend();
            break;

        default:
#ifdef HX_LOG_SUBSYSTEM
            HXLOGL1 ( HXLOG_ADEV, "snd_pcm_avail_update: %s", snd_strerror(err));
#endif
            m_wLastError = RA_AOE_DEVBUSY;
        }
    }

#ifdef HX_LOG_SUBSYSTEM
//    HXLOGL4 ( HXLOG_ADEV, "RoomOnDevice: %d", ulBytes);    
#endif

    return m_wLastError;
}


//Device specific method to get/set the devices current volume.
UINT16 HSPAudioDevice::_GetVolume() const
{
    HX_ASSERT(m_pAlsaMixerElem);
    if (!m_pAlsaMixerElem)
    {
        return 0;
    }

    UINT16 nRetVolume = 0;

    snd_mixer_elem_type_t type;    
    int err = 0;
    type = snd_mixer_elem_get_type(m_pAlsaMixerElem);
            
    if (type == SND_MIXER_ELEM_SIMPLE)
    {
        long volume, min_volume, max_volume; 

        if(snd_mixer_selem_has_playback_volume(m_pAlsaMixerElem) || 
           snd_mixer_selem_has_playback_volume_joined(m_pAlsaMixerElem))
        {
            err = snd_mixer_selem_get_playback_volume(m_pAlsaMixerElem,
                                                      SND_MIXER_SCHN_MONO, 
                                                      &volume);
            if (err < 0)
            {
#ifdef HX_LOG_SUBSYSTEM
                HXLOGL1 ( HXLOG_ADEV, "snd_mixer_selem_get_playback_volume: %s",
                          snd_strerror (err));
#endif
            }

            if (err == 0)
            {
                snd_mixer_selem_get_playback_volume_range(m_pAlsaMixerElem,
                                                          &min_volume,
                                                          &max_volume);

                if(max_volume > min_volume)
                {
                    nRetVolume = (UINT16) (100 * volume / (max_volume - min_volume));
                }
            }
        }        
    }

    return nRetVolume;
}


HX_RESULT HSPAudioDevice::_SetVolume(UINT16 unVolume)
{
    m_wLastError = RA_AOE_NOERR;

    HX_ASSERT(m_pAlsaMixerElem);
    if (!m_pAlsaMixerElem)
    {
        m_wLastError = RA_AOE_DEVNOTOPEN;
        return m_wLastError;
    }

    snd_mixer_elem_type_t type;    
    int err = 0;
    type = snd_mixer_elem_get_type(m_pAlsaMixerElem);
            
    if (type == SND_MIXER_ELEM_SIMPLE)
    {
        long volume, min_volume, max_volume, range; 

        if(snd_mixer_selem_has_playback_volume(m_pAlsaMixerElem) || 
           snd_mixer_selem_has_playback_volume_joined(m_pAlsaMixerElem))
        {
            snd_mixer_selem_get_playback_volume_range(m_pAlsaMixerElem,
                                                      &min_volume,
                                                      &max_volume);

            range = max_volume - min_volume;
            volume = (long) ((unVolume / 100) * range + min_volume);

            err = snd_mixer_selem_set_playback_volume( m_pAlsaMixerElem,
                                                       SND_MIXER_SCHN_FRONT_LEFT, 
                                                       volume);            
            if (err < 0)
            {
#ifdef HX_LOG_SUBSYSTEM
                HXLOGL1 ( HXLOG_ADEV, "snd_mixer_selem_set_playback_volume: %s",
                          snd_strerror (err));
#endif
                m_wLastError = RA_AOE_GENERAL;
            }

            if (!snd_mixer_selem_is_playback_mono (m_pAlsaMixerElem))
            {
                /* Set the right channel too */
                err = snd_mixer_selem_set_playback_volume( m_pAlsaMixerElem,
                                                           SND_MIXER_SCHN_FRONT_RIGHT, 
                                                           volume);            
                if (err < 0)
                {
#ifdef HX_LOG_SUBSYSTEM
                    HXLOGL1 ( HXLOG_ADEV, "snd_mixer_selem_set_playback_volume: %s",
                              snd_strerror (err));
#endif
                    m_wLastError = RA_AOE_GENERAL;
                }
            }
        }        
    }    

    return m_wLastError;
}

//Device specific method to drain a device. This should play the remaining
//bytes in the devices buffer and then return.
HX_RESULT HSPAudioDevice::_Drain()
{
    m_wLastError = RA_AOE_NOERR;

    HX_ASSERT(m_pAlsaPCMHandle);
    if (!m_pAlsaPCMHandle)
    {
        m_wLastError = RA_AOE_DEVNOTOPEN;
        return m_wLastError;
    }

    int err = 0;

    err = snd_pcm_drain(m_pAlsaPCMHandle);
    if (err < 0)
    {
#ifdef HX_LOG_SUBSYSTEM
        HXLOGL1 ( HXLOG_ADEV, "snd_pcm_drain: %s",
                  snd_strerror (err));
#endif
        m_wLastError = RA_AOE_GENERAL;
    }
    
    err = snd_pcm_prepare(m_pAlsaPCMHandle);
    if (err < 0)
    {
#ifdef HX_LOG_SUBSYSTEM
        HXLOGL1 ( HXLOG_ADEV, "snd_pcm_prepare: %s",
                  snd_strerror (err));
#endif
        m_wLastError = RA_AOE_GENERAL;
    }

    return m_wLastError;
}


//Device specific method to reset device and return it to a state that it
//can accept new sample rates, num channels, etc.
HX_RESULT HSPAudioDevice::_Reset()
{
    if (!m_pAlsaPCMHandle)
    {
        m_wLastError = RA_AOE_DEVNOTOPEN;
        return m_wLastError;
    }

    m_wLastError = RA_AOE_NOERR;

    m_nLastBytesPlayed = 0;

    int err = 0;

    err = snd_pcm_drop(m_pAlsaPCMHandle);
    if (err < 0)
    {
#ifdef HX_LOG_SUBSYSTEM
        HXLOGL1 ( HXLOG_ADEV, "snd_pcm_drop: %s",
                  snd_strerror (err));
#endif
        m_wLastError = RA_AOE_GENERAL;
    }

    err = snd_pcm_prepare(m_pAlsaPCMHandle);
    if (err < 0)
    {
#ifdef HX_LOG_SUBSYSTEM
        HXLOGL1 ( HXLOG_ADEV, "snd_pcm_prepare: %s",
                  snd_strerror (err));
#endif
        m_wLastError = RA_AOE_GENERAL;
    }

    return m_wLastError;
}

HX_RESULT HSPAudioDevice::_CheckFormat( const HXAudioFormat* pFormat )
{
    HX_ASSERT(m_pAlsaPCMHandle == NULL);

    m_wLastError = _OpenAudio();
    if(m_wLastError != RA_AOE_NOERR)
    {
        return m_wLastError;
    }

    m_wLastError = RA_AOE_NOERR;

    snd_pcm_format_t fmt;
    unsigned int sample_rate = 0;
    unsigned int channels = 0;

    switch (pFormat->uBitsPerSample)
    {
    case 8:
        fmt = SND_PCM_FORMAT_S8;
        break;

    case 16:
        fmt = SND_PCM_FORMAT_S16_LE;        
        break;

    case 24:
        fmt = SND_PCM_FORMAT_S24_LE;
        break;

    case 32:
        fmt = SND_PCM_FORMAT_S32_LE;
        break;

    default:
        fmt = SND_PCM_FORMAT_UNKNOWN;
        break;        
    }
    
    if (fmt == SND_PCM_FORMAT_UNKNOWN)
    {
#ifdef HX_LOG_SUBSYSTEM
        HXLOGL1 ( HXLOG_ADEV, "Unknown bits per sample: %d", pFormat->uBitsPerSample);
#endif
        m_wLastError = RA_AOE_NOTENABLED;
        return m_wLastError;
    }
    sample_rate = pFormat->ulSamplesPerSec;
    channels = pFormat->uChannels;

    /* Apply to ALSA */
    int err = 0;
	snd_pcm_hw_params_t *hwparams;

	snd_pcm_hw_params_alloca(&hwparams);

	err = snd_pcm_hw_params_any(m_pAlsaPCMHandle, hwparams);
	if (err < 0) 
    {
#ifdef HX_LOG_SUBSYSTEM
        HXLOGL1 ( HXLOG_ADEV, "snd_pcm_hw_params_any: %s", snd_strerror(err));
#endif
        m_wLastError = RA_AOE_NOTENABLED;
	}

    if (err == 0)
    {
        err = snd_pcm_hw_params_test_rate (m_pAlsaPCMHandle, hwparams, sample_rate, 0);
        if (err < 0)
        {
            m_wLastError = RA_AOE_BADFORMAT;
        }
    }

    if (err == 0)
    {
        err = snd_pcm_hw_params_test_channels (m_pAlsaPCMHandle, hwparams, channels);
        if (err < 0)
        {
            m_wLastError = RA_AOE_BADFORMAT;
        }
    }

    if (err == 0)
    {
        err = snd_pcm_hw_params_test_format (m_pAlsaPCMHandle, hwparams, fmt);
        if (err < 0)
        {
            m_wLastError = RA_AOE_BADFORMAT;
        }
    }

    _CloseAudio();

    return m_wLastError;
}


HX_RESULT HSPAudioDevice::CheckSampleRate( ULONG32 ulSampleRate )
{
    HX_ASSERT(m_pAlsaPCMHandle == NULL);
    bool shouldclose = false;

    if (!m_pAlsaPCMHandle)
    {
       m_wLastError = _OpenAudio();
       if(m_wLastError != RA_AOE_NOERR)
       {
          return m_wLastError;
       }
       shouldclose = true;
    }
       
    int err = 0;
    snd_pcm_hw_params_t *hwparams;
       
    snd_pcm_hw_params_alloca(&hwparams);
    
    m_wLastError = RA_AOE_NOERR;
    
    err = snd_pcm_hw_params_any(m_pAlsaPCMHandle, hwparams);
    if (err < 0) 
    {
#ifdef HX_LOG_SUBSYSTEM
       HXLOGL1 ( HXLOG_ADEV, "snd_pcm_hw_params_any: %s", snd_strerror(err));
#endif
       m_wLastError = RA_AOE_NOTENABLED;
    }
    
    if (err == 0)
    {
       err = snd_pcm_hw_params_test_rate (m_pAlsaPCMHandle, hwparams, ulSampleRate, 0);
       if (err < 0)
       {
          m_wLastError = RA_AOE_BADFORMAT;
       }
    }
    
    if (shouldclose)
       _CloseAudio();

    return m_wLastError;
}


HX_RESULT HSPAudioDevice::_Pause()
{
    HX_ASSERT(m_pAlsaPCMHandle);
    if (!m_pAlsaPCMHandle)
    {
        m_wLastError = RA_AOE_DEVNOTOPEN;
        return m_wLastError;
    }

    if (m_bHasHardwarePauseAndResume)
    {
        snd_pcm_state_t state;

        state = snd_pcm_state(m_pAlsaPCMHandle);
        if (state == SND_PCM_STATE_RUNNING)
        {
            int err = 0;
            err = snd_pcm_pause(m_pAlsaPCMHandle, 1);
            if (err < 0)
            {
#ifdef HX_LOG_SUBSYSTEM
                HXLOGL1 ( HXLOG_ADEV, "snd_pcm_pause: %s",
                          snd_strerror (err));
#endif
            
                m_wLastError = RA_AOE_NOTSUPPORTED;
            }
        }
    }
    else
    {
       pthread_mutex_lock(&m_m);
       m_SWPause = true;
       _Drain();
       _Reset();
       pthread_mutex_unlock(&m_m);
    }
    
    return m_wLastError;
}

HX_RESULT HSPAudioDevice::_Resume()
{
    HX_ASSERT(m_pAlsaPCMHandle);
    if (!m_pAlsaPCMHandle)
    {
        m_wLastError = RA_AOE_DEVNOTOPEN;
        return m_wLastError;
    }

    if (m_bHasHardwarePauseAndResume)
    {
        snd_pcm_state_t state;

        state = snd_pcm_state(m_pAlsaPCMHandle);
        if (state == SND_PCM_STATE_PAUSED)
        {
            int err = 0;
            err = snd_pcm_pause(m_pAlsaPCMHandle, 0);

            if (err < 0)
            {
#ifdef HX_LOG_SUBSYSTEM
                HXLOGL1 ( HXLOG_ADEV, "snd_pcm_pause: %s",
                          snd_strerror (err));
#endif
            
                m_wLastError = RA_AOE_NOTSUPPORTED;
            }
        }
    }
    else
    {
       pthread_mutex_lock(&m_m);
       m_SWPause = false;
       _Reset();
       pthread_mutex_unlock(&m_m);
    }
    
    return m_wLastError;
}

BOOL HSPAudioDevice::HardwarePauseSupported() const
{
    HX_ASSERT(m_pAlsaPCMHandle != NULL);

    return m_bHasHardwarePauseAndResume;
}


void HSPAudioDevice::HandleXRun(void) const
{
    int err = 0;

#ifdef HX_LOG_SUBSYSTEM
    HXLOGL2 ( HXLOG_ADEV, "Handling XRun");
#endif

    err = snd_pcm_prepare(m_pAlsaPCMHandle);
    if (err < 0)
    {
#ifdef HX_LOG_SUBSYSTEM
        HXLOGL1 ( HXLOG_ADEV, "snd_pcm_resume: %s (xrun)",
                  snd_strerror (err));
#endif
    }

    /* Catch up to the write position of the audio device so we get new data.
       XXXRGG: Is there some way we, the device, can force a rewind? */
    m_nLastBytesPlayed = m_ulTotalWritten;
}

void HSPAudioDevice::HandleSuspend(void) const
{
    int err = 0;

    do
    {
        err = snd_pcm_resume(m_pAlsaPCMHandle);
        if (err == 0)
        {
            break;
        }
        else if (err == -EAGAIN)
        {
            usleep(1000);
        }
    } while (err == -EAGAIN);

    if (err < 0) 
    {
        HandleXRun();
    }
}

#endif // HELIX_USE_ALSA

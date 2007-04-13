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

#ifndef _AUDLINUXALSA
#define _AUDLINUXALSA

#ifdef USE_HELIX_ALSA

#define ALSA_PCM_NEW_HW_PARAMS_API
#define ALSA_PCM_NEW_SW_PARAMS_API

#include <alsa/asoundlib.h>

class HelixSimplePlayer;
class AudioQueue
{
public:
   AudioQueue( const HXAudioData *buf );
   ~AudioQueue();

   AudioQueue  *fwd;
   HXAudioData ad;
   LONG32 bytes;
};

class HSPAudioDevice : public IHXAudioDevice
{
public:
   HSPAudioDevice(HelixSimplePlayer *player, const char *device);
   virtual ~HSPAudioDevice();

   /*
    *  IUnknown methods
    */
   STDMETHOD(QueryInterface)   (THIS_
                               REFIID riid,
                                void** ppvObj);
   STDMETHOD_(ULONG32,AddRef)  (THIS);
   STDMETHOD_(ULONG32,Release) (THIS);
   /*
    * IHXAudioDevice methods
    */ 
   STDMETHOD(CheckFormat) (
      THIS_
      const HXAudioFormat* pAudioFormat
      );

   STDMETHOD(Close) (
      THIS_
      const BOOL bFlush
      );
 
   STDMETHOD(Drain) (
      THIS
      );

   STDMETHOD(GetCurrentAudioTime) (
      THIS_
      REF(ULONG32) ulCurrentTime
      );

   STDMETHOD_(UINT16,GetVolume) (
      THIS
      );

   STDMETHOD_(BOOL,InitVolume) (
      THIS_
      const UINT16 uMinVolume,
      const UINT16 uMaxVolume
      );

   STDMETHOD(Open) (
      THIS_
      const HXAudioFormat* pAudioFormat,
      IHXAudioDeviceResponse* pStreamResponse
      );

   STDMETHOD(Pause) (
      THIS
      );

   STDMETHOD(Reset) (
      THIS
      );

   STDMETHOD(Resume) (
      THIS
      );

   STDMETHOD(SetVolume) (
      THIS_
      const UINT16 uVolume
      );

   STDMETHOD(Write) (
      THIS_
      const HXAudioData* pAudioData
      );

   HX_RESULT  OnTimeSync();

   void setDevice( const char *device );
   
protected:
   virtual INT16 GetAudioFd(void);
   
   //This ones important.
   virtual UINT64 GetBytesActualyPlayed(void) const;
   
   //Device specific method to set the audio device characteristics. Sample rate,
   //bits-per-sample, etc.
   //Method *must* set member vars. m_unSampleRate and m_unNumChannels.
   virtual HX_RESULT SetDeviceConfig( const HXAudioFormat* pFormat );
   
   //Device specific method to test wether or not the device supports the
   //give sample rate. If the device can not be opened, or otherwise tested,
   //it should return RA_AOE_DEVBUSY.
   virtual HX_RESULT CheckSampleRate( ULONG32 ulSampleRate );
   virtual HX_RESULT _CheckFormat( const HXAudioFormat* pFormat );
   
   //Device specific method to write bytes out to the audiodevice and return a
   //count of bytes written.
   virtual HX_RESULT WriteBytes( UCHAR* buffer, ULONG32 ulBuffLength, LONG32& lCount );
   
   //Device specific methods to open/close the mixer and audio devices.
   virtual HX_RESULT _OpenAudio();
   virtual HX_RESULT _CloseAudio();
   virtual HX_RESULT _OpenMixer();
   virtual HX_RESULT _CloseMixer();
   
   //Device specific method to reset device and return it to a state that it
   //can accept new sample rates, num channels, etc.
   virtual HX_RESULT _Reset();
   virtual HX_RESULT _Pause();
   virtual HX_RESULT _Resume();
  
   //Device specific method to get/set the devices current volume.
   virtual UINT16    _GetVolume() const;
   virtual HX_RESULT _SetVolume(UINT16 volume);
   
   //Device specific method to drain a device. This should play the remaining
   //bytes in the devices buffer and then return.
   virtual HX_RESULT _Drain();
   
   //Device specific method to return the amount of room available on the
   //audio device that can be written without blocking.
   virtual HX_RESULT GetRoomOnDevice( ULONG32& ulBytes) const;
   
   //A method to let us know if the hardware supports puase/resume.
   //We can use this to remove unneeded memcpys and other expensive
   //operations. The default implementation is 'No, not supported'.
   virtual BOOL HardwarePauseSupported() const;

   int _Write( const HXAudioData *pAudioData );

   int sync();
   
private:
   HSPAudioDevice();
   //protect the unintentional copy ctor.
   HSPAudioDevice( const HSPAudioDevice & );  //Not implemented.
   
   /* The constness imposed by the base class is a lost cause here --
      make all functions const, all members mutable. */
   void HandleXRun(void) const;
   void HandleSuspend(void) const;
   
   HX_RESULT GetBytesActuallyPlayedUsingTStamps (UINT64 &nBytesPlayed) const;
   HX_RESULT GetBytesActuallyPlayedUsingDelay   (UINT64 &nBytesPlayed) const;
   HX_RESULT GetBytesActuallyPlayedUsingAvail   (UINT64 &nBytesPlayed) const;
   HX_RESULT GetBytesActuallyPlayedUsingTimer   (UINT64 &nBytesPlayed) const;
   
   mutable snd_pcm_t*   m_pAlsaPCMHandle;
   mutable snd_mixer_t* m_pAlsaMixerHandle;
   mutable snd_mixer_elem_t* m_pAlsaMixerElem;
   
   mutable IHXBuffer* m_pPCMDeviceName;
   mutable IHXBuffer* m_pMixerDeviceName;
   mutable IHXBuffer* m_pMixerElementName;
   
   mutable BOOL m_bHasHardwarePauseAndResume;
   
   mutable UINT64 m_nBytesPlayedBeforeLastTrigger;
   
   mutable UINT64 m_nLastBytesPlayed;
   
   mutable snd_timestamp_t m_tstampLastTrigger;
   mutable BOOL m_bGotInitialTrigger;
   
   mutable BOOL m_bUseMMAPTStamps;

   LONG32             m_lRefCount;
   mutable LONG32     m_wLastError;
   BOOL               m_bMixerPresent;
   UINT32             m_unSampleRate;
   UINT16             m_unNumChannels;
   UINT32             m_wBlockSize;
   UINT32             m_ulBytesPerGran;
   UINT32             m_ulDeviceBufferSize;
   UINT16             m_uSampFrameSize;
   UINT32             m_ulTotalWritten;
   UINT32             m_ulCurrentTime;
   UINT32             m_ulQTime;
   UINT32             m_ulLastTime;
   BOOL               m_SWPause;

   HelixSimplePlayer *m_Player;
   IHXAudioDeviceResponse *m_pStreamResponse;

   bool                        m_done;
   bool                        m_drain;
   bool                        m_closed;
   AudioQueue                 *m_head;
   AudioQueue                 *m_tail;
   pthread_t                   m_thrid;
   pthread_mutex_t             m_m;
   pthread_cond_t              m_cv;

   void addBuf(struct AudioQueue *item);
   void pushBuf(struct AudioQueue *item);
   AudioQueue *getBuf();
   void clearQueue();

   static void *writerThread( void *arg );
};

#endif  // USE_HELIX_ALSA

#endif  //_AUDIOOUTLINUXALSA

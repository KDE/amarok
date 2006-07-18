/*
 *
 * This software is released under the provisions of the GPL version 2.
 * see file "COPYING".  If that file is not available, the full statement 
 * of the license can be found at
 *
 * http://www.fsf.org/licensing/licenses/gpl.txt
 *
 * Copyright (c) Paul Cifarelli 2005
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved.
 *
 * at this point, this class should be HelixNotSoSimplePlayer...
 */

#ifndef _HELIX_SIMPLEPLAYER_LIB_H_INCLUDED_
#define _HELIX_SIMPLEPLAYER_LIB_H_INCLUDED_

class HSPClientAdviceSink;
class HSPClientContext;
class IHXErrorSinkControl;
class HelixSimplePlayerAudioStreamInfoResponse;

#include <limits.h>
#include <sys/param.h>
#include <pthread.h>
#include <vector>
#include <config.h>
#include <iostream>
using std::vector;

#define MAX_PATH PATH_MAX

#define MAX_PLAYERS 100 // that should do it...
#define MAX_SCOPE_SAMPLES 5120

class HelixSimplePlayer;
class CHXURL;
class IHXVolumeAdviseSink;
class IHXAudioPlayer;
class IHXAudioStream;
class IHXAudioCrossFade;
class IHXPlayer;
class IHXPlayer2;
class IHXErrorSink;
class IHXErrorSinkControl;
class IHXAudioPlayer;
class IHXVolume;
class IHXPlayerNavigator;
class IHXClientEngineSelector;
class IHXClientEngine;
class IHXAudioHook;
class IHXAudioStreamInfoResponse;
class IHXCommonClassFactory;
class IHXPluginEnumerator;
class IHXPlugin2Handler;
class IHXAudioDeviceManager;
class IHXAudioHookManager;
class IHXPreferences;
class HSPAudioDevice;
class IHXAudioDeviceResponse;
#ifdef USE_HELIX_ALSA
struct _snd_mixer;
struct _snd_mixer_elem;
#endif

// scope delay queue
class DelayQueue
{
public:
   DelayQueue() : fwd(0), len(0), time(0), etime(0), nchan(0), bps(0),  allocd(false), buf(0) {}
   DelayQueue(int bufsize) : fwd(0), len(bufsize), time(0), etime(0), nchan(0), bps(0), allocd(true), buf(0) 
      { buf = new unsigned char [ bufsize ]; }
   DelayQueue(DelayQueue &src) : fwd(0), len(src.len), time(src.time), etime(src.etime), nchan(src.nchan), bps(src.bps),  allocd(true), buf(0)
      { buf = new unsigned char [ len ]; memcpy( (void *) buf, (void *) src.buf, src.len ); }
   ~DelayQueue() { if (allocd) delete [] buf; }
   struct DelayQueue *fwd;
   int           len;      // len of the buffer
   unsigned long time;     // start time of the buffer
   unsigned long etime;    // end time of the buffer
   int           nchan;    // number of channels
   int           bps;      // bytes per sample
   double        tps;      // time per sample
   int           spb;      // samples per buffer
   bool          allocd;   // did we allocate the memory?
   unsigned char *buf;
};


// simple list of supported mime type
struct MimeList
{
   MimeList(char *mimestr, char *ext) : mimetypes(0), mimeexts(0)
      { mimetypes = new char[strlen(mimestr)+1]; strcpy(mimetypes,mimestr); mimeexts = new char[strlen(ext)+1]; strcpy(mimeexts,ext); }
   ~MimeList() { delete [] mimetypes; delete [] mimeexts; }
   struct MimeList *fwd;
   char *mimetypes;
   char *mimeexts;
};

class HelixSimplePlayer
{
public:
   enum { ALL_PLAYERS = -1 };

   HelixSimplePlayer();
   virtual ~HelixSimplePlayer();

   void init(const char *corelibpath, const char *pluginslibpath, const char *codecspath, int numPlayers = 1);
   void tearDown();
   int  initDirectSS();
   int  addPlayer();                                                 // add another player
   void play(int playerIndex = ALL_PLAYERS, 
             bool fadein = false, bool fadout = false,
             unsigned long fadetime = 0);                            // play the current url, waiting for it to finish
   void play(const char *url, int playerIndex = ALL_PLAYERS,
             bool fadein = false, bool fadeout = false,
             unsigned long fadetime = 0);                            // play the file, setting it as the current url; wait for it to finish
   int  setURL(const char *url, 
               int playerIndex = ALL_PLAYERS,
               bool islocal = true);                                 // set the current url
   bool done(int playerIndex = ALL_PLAYERS);                         // test to see if the player(s) is(are) done
   void start(int playerIndex = ALL_PLAYERS, 
              bool fadein = false, 
              unsigned long fadetime = 0);                           // start the player
   void start(const char *file, int playerIndex = ALL_PLAYERS,
              bool fadein = false,
              unsigned long fadetime = 0);                           // start the player, setting the current url first
   void stop(int playerIndex = ALL_PLAYERS);                         // stop the player(s)
   void pause(int playerIndex = ALL_PLAYERS);                        // pause the player(s)
   void resume(int playerIndex = ALL_PLAYERS);                       // pause the player(s)
   void seek(unsigned long pos, int playerIndex = ALL_PLAYERS);      // seek to the pos
   unsigned long where(int playerIndex) const;                       // where is the player in the playback
   unsigned long duration(int playerIndex) const;                    // how long (ms) is this clip?
   unsigned long getVolume(int playerIndex);                         // get the current volume
   void setVolume(unsigned long vol, int playerIndex = ALL_PLAYERS); // set the volume
   void setMute(bool mute, int playerIndex = ALL_PLAYERS);           // set mute: mute = true to mute the volume, false to unmute
   bool getMute(int playerIndex);                                    // get the mute state of the player
   void dispatch();                                                  // dispatch the player(s)

   void cleanUpStream(int playerIndex);                              // cleanup after stream complete
   bool isPlaying(int playerIndex) const;                            // is the player currently playing?
   bool isLocal(int playerIndex) const;                              // playing a local file
   int numPlayers() const { return nNumPlayers; }                    // return the number of players


   virtual void onVolumeChange(int) {}                   // called when the volume is changed
   virtual void onMuteChange(int) {}                     // called when mute is changed
   virtual void onContacting(const char */*host*/) {}    // called when contacting a host
   virtual void onBuffering(int /*percentage*/) {} // called when buffering

   int getError() const { return theErr; }

   static char* RemoveWrappingQuotes(char* str);
   //void  setUsername(const char *username) { m_pszUsername = username; }
   //void  setPassword(const char *password) { m_pszPassword = password; }
   //void  setGUIDFile(const char *file) { m_pszGUIDFile = file; }
   bool  ReadGUIDFile();

   typedef struct
   {
      char title[512];
      char artist[512];
      unsigned long bitrate;
   } metaData;

private:
   void  DoEvent();
   void  DoEvents(int nTimeDelta);
   unsigned long GetTime();

   char                     mCoreLibPath[MAXPATHLEN];
   char                     mPluginLibPath[MAXPATHLEN];
   int                      theErr;
   IHXErrorSink*            pErrorSink;
   IHXErrorSinkControl*     pErrorSinkControl;
   IHXClientEngineSelector* pCEselect;
   IHXCommonClassFactory*   pCommonClassFactory;
   IHXPluginEnumerator*     pPluginE;
   IHXPlugin2Handler*       pPlugin2Handler;
   IHXAudioDeviceManager*   pAudioDeviceManager;
   IHXAudioHookManager*     pAudioHookManager;
   IHXAudioHook*            pFinalAudioHook;
   HSPAudioDevice*          pAudioDevice;

   struct playerCtrl
   {
      bool                        bPlaying;
      bool                        bStarting;
      bool                        bFadeIn;
      bool                        bFadeOut;
      unsigned long               ulFadeLength;
      IHXAudioStream*             pStream;
      HSPClientContext*           pHSPContext;
      IHXPlayer*                  pPlayer;
      IHXPlayer2*                 pPlayer2;
      IHXAudioPlayer*             pAudioPlayer;
      IHXAudioCrossFade*          pCrossFader;
      IHXVolume*                  pVolume;
      IHXVolumeAdviseSink*        pVolumeAdvise;
      IHXAudioStreamInfoResponse* pStreamInfoResponse;
      IHXAudioHook*               pPreMixHook;
      IHXAudioHook*               pPostMixHook;
      metaData                    md;
      char*                       pszURL;
      bool                        isLocal;
      unsigned short              volume;
      bool                        ismute;
      // scope
      int                         scopecount;
      struct DelayQueue          *scopebufhead;
      struct DelayQueue          *scopebuftail;
      pthread_mutex_t             m_scope_m;
   } **ppctrl;

   IHXAudioDeviceResponse *pAudioDeviceResponse;
   bool                    bURLFound;
   int                     nNumPlayers;
   int                     nNumPlayRepeats;
   int                     nTimeDelta;
   int                     nStopTime;
   bool		           bStopTime;
   void*                   core_handle;
   bool 		   bStopping;
   int 		           nPlay;

public:
   const IHXAudioPlayer *getAudioPlayer(int playerIndex) const { return ppctrl[playerIndex]->pAudioPlayer; }
   const IHXAudioCrossFade *getCrossFader(int playerIndex) const { return ppctrl[playerIndex]->pCrossFader; }

   // crossfade
   void setFadeout(bool fadeout, unsigned long fadelength, int playerIndex = ALL_PLAYERS);

   // scope
   void addScopeBuf(struct DelayQueue *item, int playerIndex);
   DelayQueue *getScopeBuf(int playerIndex);
   int getScopeCount(int playerIndex) { return playerIndex >= 0 && playerIndex < nNumPlayers ? ppctrl[playerIndex]->scopecount : 0; }
   int peekScopeTime(unsigned long &t, int playerIndex);
   void clearScopeQ(int playerIndex = ALL_PLAYERS);

   // equalizer
   void enableEQ(bool enabled) { m_eq_enabled = enabled; }
   bool isEQenabled() { return m_eq_enabled; }
   void updateEQgains();

   int numPlugins() const;
   int getPluginInfo(int index, 
                     const char *&description, 
                     const char *&copyright, 
                     const char *&moreinfourl) const;


   metaData *getMetaData(int playerIndex);

   const MimeList *getMimeList() const { return mimehead; }
   int getMimeListLen() const { return mimelistlen; }

private:

   bool                 bEnableAdviceSink;
   bool                 bEnableVerboseMode;
   IHXClientEngine*     pEngine;   
   IHXPreferences*      pEngineContext;   
   char*                m_pszUsername;
   char*                m_pszPassword;
   char*                m_pszGUIDFile;
   char*                m_pszGUIDList;
   int                  m_Error;
   unsigned long        m_ulNumSecondsPlayed;

   pthread_mutex_t      m_engine_m;

   // supported mime type list
   MimeList            *mimehead;
   int                  mimelistlen;

   // equalizer
   bool                 m_eq_enabled;

public:
   // 0 = OSS
   // 1 = OldOSSsupport
   // 2 = ESound
   // 3 = Alsa
   // 4 = USound
   enum AUDIOAPI
   {
      OSS,
      OLDOSS,
      ESOUND,
      ALSA,
      USOUND
   };

   void setOutputSink( AUDIOAPI out );
   AUDIOAPI getOutputSink();
   void setDevice( const char *dev );
   const char *getDevice();
   void setAlsaCapableCore() { m_AlsaCapableCore = true; }
   virtual int fallbackToOSS() { return 0; }

   int                  m_preamp;
   vector<int>          m_equalizerGains;

private:
   AUDIOAPI m_outputsink;
   char *m_device;

   // work around the annoying problem of the core reseting the PCM volume on every url change
protected:
   void openAudioDevice();
   void closeAudioDevice();
#ifdef USE_HELIX_ALSA
   int getDirectMasterVolume();
   void setDirectMasterVolume(int vol);
#endif
   int getDirectPCMVolume();
   void setDirectPCMVolume(int vol);

private:
   AUDIOAPI m_direct;
   bool m_AlsaCapableCore;
   int m_nDevID;
#ifdef USE_HELIX_ALSA
   struct _snd_mixer*      m_pAlsaMixerHandle;
   struct _snd_mixer_elem* m_pAlsaMasterMixerElem;
   struct _snd_mixer_elem* m_pAlsaPCMMixerElem;
#endif
   char *m_alsaDevice;
   bool m_urlchanged;
   int m_volBefore;
   int m_volAtStart;
#ifdef USE_HELIX_ALSA
   int m_MvolBefore;
   int m_MvolAtStart;
#endif

   int m_numPlugins;
   struct pluginInfo
   {
      char *description; 
      char *copyright; 
      char *moreinfourl; 
   } **m_pluginInfo;


protected:
   virtual int print2stdout(const char *fmt, ...)
#ifdef __GNUC__
      __attribute__ ((format (printf, 2, 3)))
#endif
      ;
   virtual int print2stderr(const char *fmt, ...)
#ifdef __GNUC__
      __attribute__ ((format (printf, 2, 3)))
#endif
      ;
   virtual void notifyUser(unsigned long/*code*/, const char */*moreinfo*/, const char */*moreinfourl*/) {}
   virtual void interruptUser(unsigned long/*code*/, const char */*moreinfo*/, const char */*moreinfourl*/) {}

   friend class HSPClientAdviceSink;
   friend class HSPErrorSink;
   friend class HSPAuthenticationManager;
   friend class HelixSimplePlayerAudioStreamInfoResponse;
   friend class HSPPreMixAudioHook;
   friend class HSPPostProcessor;
   friend class HSPPostMixAudioHook;
   friend class HSPFinalAudioHook;
   friend class HelixSimplePlayerVolumeAdvice;
   friend class HSPEngineContext;
   friend class HSPAudioDevice;
};

#endif

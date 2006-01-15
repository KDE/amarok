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
#include "hxclsnk.h"
#include "hxerror.h"
#include "hxauth.h"
#include "hxprefs.h"
#include "hxstrutl.h"
#include "hxvsrc.h"
#include "hxresult.h"
#include "hxplugn.h"

#include "hspadvisesink.h"
#include "hsperror.h"
#include "hspauthmgr.h"
#include "hspcontext.h"
#include "print.h"
#include <X11/Xlib.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "hxausvc.h"

#include "dllpath.h"

#include <config.h>

#include "helix-sp.h"
#include "hspvoladvise.h"
#include "utils.h"
#include "hsphook.h"
#include "hxfiles.h"

#ifdef USE_HELIX_ALSA
#include <alsa/asoundlib.h>
#endif

#ifdef HX_LOG_SUBSYSTEM
#include "hxtlogutil.h"
#endif

#ifdef __FreeBSD__
#define PTHREAD_MUTEX_FAST_NP PTHREAD_MUTEX_NORMAL
#endif

#if !defined(__NetBSD__) && !defined(__OpenBSD__)
	#include <sys/soundcard.h>
#else
	#include <soundcard.h>
#endif

typedef HX_RESULT (HXEXPORT_PTR FPRMSETDLLACCESSPATH) (const char*);

class HelixSimplePlayerAudioStreamInfoResponse : public IHXAudioStreamInfoResponse
{
public:
   HelixSimplePlayerAudioStreamInfoResponse(HelixSimplePlayer *player, int playerIndex) : 
      m_Player(player), m_index(playerIndex), m_lRefCount(0) {}
   virtual ~HelixSimplePlayerAudioStreamInfoResponse() {}

   /*
    *  IUnknown methods
    */
   STDMETHOD(QueryInterface)   (THIS_
                               REFIID riid,
                               void** ppvObj);
    
   STDMETHOD_(ULONG32,AddRef)  (THIS);

   STDMETHOD_(ULONG32,Release) (THIS);

   /*
    * IHXAudioStreamInfoResponse methods
    */
   STDMETHOD(OnStream) (THIS_ 
                        IHXAudioStream *pAudioStream
                        );
private:
   HelixSimplePlayer *m_Player;
   IHXAudioStream    *m_Stream;
   int                m_index;
   LONG32             m_lRefCount;
   HXAudioFormat      m_audiofmt;
};

STDMETHODIMP
HelixSimplePlayerAudioStreamInfoResponse::QueryInterface(REFIID riid, void**ppvObj)
{
    if(IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXAudioStreamInfoResponse *)this;
        return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXAudioStreamInfoResponse))
    {
        AddRef();
        *ppvObj = (IHXAudioStreamInfoResponse*)this;
        return HXR_OK;
    }
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(UINT32)
HelixSimplePlayerAudioStreamInfoResponse::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(UINT32)
HelixSimplePlayerAudioStreamInfoResponse::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP HelixSimplePlayerAudioStreamInfoResponse::OnStream(IHXAudioStream *pAudioStream)
{
   m_Player->STDERR("Stream Added on player %d, stream duration %d, sources %d\n", m_index, m_Player->duration(m_index), m_Player->ppctrl[m_index]->pPlayer->GetSourceCount());

   m_Player->ppctrl[m_index]->pStream = pAudioStream;
   m_Player->ppctrl[m_index]->pPreMixHook = new HSPPreMixAudioHook(m_Player, m_index, pAudioStream);

   // addpremixhook adds another ref
   pAudioStream->AddPreMixHook(m_Player->ppctrl[m_index]->pPreMixHook, false);
   m_Player->ppctrl[m_index]->pPreMixHook->Release(); // release the addref from the premix constructor
   m_Player->ppctrl[m_index]->bStarting = false;

   return HXR_OK;
}

// Constants
const int DEFAULT_TIME_DELTA = 2000;
const int DEFAULT_STOP_TIME =  -1;
const int SLEEP_TIME         = 10;
const int GUID_LEN           = 64;

// *** IUnknown methods ***

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::QueryInterface
//  Purpose:
//	Implement this to export the interfaces supported by your 
//	object.
//
STDMETHODIMP HelixSimplePlayerVolumeAdvice::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = (IUnknown*)(IHXClientAdviseSink*)this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXVolumeAdviseSink))
    {
	AddRef();
	*ppvObj = (IHXVolumeAdviseSink*)this;
	return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::AddRef
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32) HelixSimplePlayerVolumeAdvice::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::Release
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32) HelixSimplePlayerVolumeAdvice::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP HelixSimplePlayerVolumeAdvice::OnVolumeChange(const UINT16 /*uVolume*/)
{
   m_Player->onVolumeChange(m_index);
#ifdef HELIX_SW_VOLUME_INTERFACE
   m_Player->ppctrl[m_index]->volume = uVolume;
#endif
   return HXR_OK;
}

STDMETHODIMP HelixSimplePlayerVolumeAdvice::OnMuteChange(const BOOL bMute)
{
   m_Player->onMuteChange(m_index);
   m_Player->ppctrl[m_index]->ismute = bMute;
   return HXR_OK;
}


void HelixSimplePlayer::cleanUpStream(int /* playerIndex */)
{
   // dont do anything here for now
}


void HelixSimplePlayer::updateEQgains()
{
   for (int i = 0; i<nNumPlayers; i++)
      if (pFinalAudioHook && isEQenabled())
         ((HSPFinalAudioHook *)pFinalAudioHook)->updateEQgains(m_preamp, m_equalizerGains);
}

/*
 *  handle one event
 */
void HelixSimplePlayer::DoEvent()
{
   struct _HXxEvent *pNothing = 0x0;
   struct timeval    mtime;
   
   mtime.tv_sec  = 0;
   mtime.tv_usec = SLEEP_TIME * 1000;
   usleep(SLEEP_TIME*1000);
   pEngine->EventOccurred(pNothing);
}

/*
 *  handle events for at most nTimeDelta milliseconds
 */
void HelixSimplePlayer::DoEvents(int)
{
    DoEvent();
}

/*
 *  return the number of milliseconds since the epoch
 */
UINT32 HelixSimplePlayer::GetTime()
{
   timeval t;
   gettimeofday(&t, NULL);

   // FIXME:
   // the fact that the result is bigger than a UINT32 is really irrelevant; 
   // we can still play a stream for many many years...
   return (UINT32)((t.tv_sec * 1000) + (t.tv_usec / 1000));
}

char* HelixSimplePlayer::RemoveWrappingQuotes(char* str)
{
   int len = strlen(str);
   if (len > 0)
   {
      if (str[len-1] == '"') str[--len] = 0;
      if (str[0] == '"') {int i = 0; do { str[i] = str[i+1]; ++i; } while(--len); }
   }
   return str;
}


HelixSimplePlayer::HelixSimplePlayer() :
   theErr(HXR_FAILED),
   pErrorSink(NULL),
   pErrorSinkControl(NULL),
   pPluginE(0),
   pPlugin2Handler(0),
   ppctrl(NULL),
   bURLFound(false),
   nNumPlayers(0),
   nNumPlayRepeats(1),
   nTimeDelta(DEFAULT_TIME_DELTA),
   nStopTime(DEFAULT_STOP_TIME),
   bStopTime(true),
   bStopping(false),
   nPlay(0),
   bEnableAdviceSink(false),
   bEnableVerboseMode(false),
   pEngine(NULL),
   pEngineContext(NULL),
   m_pszUsername(NULL),
   m_pszPassword(NULL),
   m_pszGUIDFile(NULL),
   m_pszGUIDList(NULL),
   m_Error(0),
   m_ulNumSecondsPlayed(0),
   mimehead(0),
   mimelistlen(0),
   scopecount(0),
   scopebufhead(0),
   scopebuftail(0),
   m_outputsink(OSS),
   m_device(0),
#ifdef USE_HELIX_ALSA
   m_direct(ALSA),  // TODO: out why my alsa direct HW reader doesnt pickup changes in kmix (the whole purpose of this!)
#else
   m_direct(OSS),
#endif
   m_AlsaCapableCore(false),
   m_nDevID(-1),
   //m_pAlsaPCMHandle(NULL),
   m_pAlsaMixerHandle(NULL),
   m_pAlsaMixerElem(NULL),
   m_alsaDevice("default"),
   m_urlchanged(0),
   m_volBefore(-1),
   m_volAtStart(-1),
   m_preamp(0)
{
   m_xf.crossfading = 0;

   pthread_mutexattr_t ma;

   pthread_mutexattr_init(&ma);
   pthread_mutexattr_settype(&ma, PTHREAD_MUTEX_FAST_NP); // note this is not portable outside linux and a few others
   pthread_mutex_init(&m_engine_m, &ma);
   pthread_mutex_init(&m_scope_m, &ma);
}

void HelixSimplePlayer::init(const char *corelibhome, const char *pluginslibhome, const char *codecshome, int numPlayers)
{
   int i;

   theErr = HXR_OK;

   FPRMCREATEENGINE        fpCreateEngine;
   FPRMSETDLLACCESSPATH    fpSetDLLAccessPath;

   SafeSprintf(mCoreLibPath, MAX_PATH, "%s/%s", corelibhome, "clntcore.so");

   // Allocate arrays to keep track of players and client
   // context pointers
   ppctrl = new struct playerCtrl *[MAX_PLAYERS];
   memset(ppctrl, 0, sizeof(struct playerCtrl *) * MAX_PLAYERS);

   if (!ppctrl)
   {
      STDOUT("Error: Out of Memory.\n");
      theErr = HXR_UNEXPECTED;
      return;
   }

   fpCreateEngine    = NULL;

   // prepare/load the HXCore module
   //STDOUT("Simpleplayer is looking for the client core at %s\n", mCoreLibPath );

   core_handle = dlopen(mCoreLibPath, RTLD_LAZY | RTLD_GLOBAL);
   if (!core_handle)
   {
      STDERR("splayer: failed to open corelib, errno %d\n", errno);
      theErr = HXR_FAILED;
      return;
   }
   fpCreateEngine = (FPRMCREATEENGINE) dlsym(core_handle, "CreateEngine");
   fpSetDLLAccessPath = (FPRMSETDLLACCESSPATH) dlsym(core_handle, "SetDLLAccessPath");

   if (fpCreateEngine == NULL ||
       fpSetDLLAccessPath == NULL )
   {
      theErr = HXR_FAILED;
      return;
   }

   //Now tell the client core where to find the plugins and codecs it
   //will be searching for.
   if (NULL != fpSetDLLAccessPath)
   {
      //Create a null delimited, double-null terminated string
      //containing the paths to the encnet library (DT_Common) and
      //the sdpplin library (DT_Plugins)...
      char pPaths[256]; /* Flawfinder: ignore */
      char* pPathNextPosition = pPaths;
      memset(pPaths, 0, 256);
      UINT32 ulBytesLeft = 256;

      char* pNextPath = new char[256];
      memset(pNextPath, 0, 256);

      SafeSprintf(pNextPath, 256, "DT_Common=%s", corelibhome);
      //STDERR("Common DLL path %s\n", pNextPath );
      UINT32 ulBytesToCopy = strlen(pNextPath) + 1;
      if (ulBytesToCopy <= ulBytesLeft)
      {
         memcpy(pPathNextPosition, pNextPath, ulBytesToCopy);
         pPathNextPosition += ulBytesToCopy;
         ulBytesLeft -= ulBytesToCopy;
      }

      SafeSprintf(pNextPath, 256, "DT_Plugins=%s", pluginslibhome);
      //STDERR("Plugin path %s\n", pNextPath );
      ulBytesToCopy = strlen(pNextPath) + 1;
      if (ulBytesToCopy <= ulBytesLeft)
      {
         memcpy(pPathNextPosition, pNextPath, ulBytesToCopy);
         pPathNextPosition += ulBytesToCopy;
         ulBytesLeft -= ulBytesToCopy;
      }
      
      SafeSprintf(pNextPath, 256, "DT_Codecs=%s", codecshome);
      //STDERR("Codec path %s\n", pNextPath );
      ulBytesToCopy = strlen(pNextPath) + 1;
      if (ulBytesToCopy <= ulBytesLeft)
      {
         memcpy(pPathNextPosition, pNextPath, ulBytesToCopy);
         pPathNextPosition += ulBytesToCopy;
         ulBytesLeft -= ulBytesToCopy;
         *pPathNextPosition='\0';
      }

      fpSetDLLAccessPath((char*)pPaths);
       
      HX_VECTOR_DELETE(pNextPath);
   }

   // do I need to do this???
   //XInitThreads();

   // create client engine
   if (HXR_OK != fpCreateEngine((IHXClientEngine**)&pEngine))
   {
      theErr = HXR_FAILED;
      return;
   }

   pCommonClassFactory = 0;
   // get the common class factory
   pEngine->QueryInterface(IID_IHXCommonClassFactory, (void **) &pCommonClassFactory);
   if (!pCommonClassFactory)
      STDERR("no CommonClassFactory\n");

   // get the engine setup interface
   IHXClientEngineSetup *pEngineSetup = 0;
   pEngine->QueryInterface(IID_IHXClientEngineSetup, (void **) &pEngineSetup);
   if (!pEngineSetup)
      STDERR("no engine setup interface\n");
   else
   {
      pEngineContext = new HSPEngineContext(this, pCommonClassFactory);
      pEngineContext->AddRef();
#ifdef HX_LOG_SUBSYSTEM
      HX_ENABLE_LOGGING(pEngineContext);
#endif
      pEngineSetup->Setup(pEngineContext);
      pEngineSetup->Release();
   }

   // get the client engine selector
   pCEselect = 0;
   pEngine->QueryInterface(IID_IHXClientEngineSelector, (void **) &pCEselect);
   if (!pCEselect)
      STDERR("no CE selector\n");

   pPluginE = 0;
   // get the plugin enumerator
   pEngine->QueryInterface(IID_IHXPluginEnumerator, (void **) &pPluginE);
   if (!pPluginE)
      STDERR("no plugin enumerator\n");

   pPlugin2Handler = 0;
   // get the plugin2handler
   pEngine->QueryInterface(IID_IHXPlugin2Handler, (void **) &pPlugin2Handler);
   if (!pPlugin2Handler)
      STDERR("no plugin enumerator\n");

   pAudioHookManager = 0;
   pFinalAudioHook = 0;
   pEngine->QueryInterface(IID_IHXAudioHookManager, (void **) &pAudioHookManager);
   if (!pAudioHookManager)
      print2stderr("no audio device hook manager\n");


   // create players
   for (i = 0; i < numPlayers; i++)
   {
      addPlayer();
   }

   // install hook for visualizations, equalizer, and volume control
   HSPFinalAudioHook *pPMAH = new HSPFinalAudioHook(this);
   pAudioHookManager->AddHook(pPMAH);
   pFinalAudioHook = pPMAH;

   if (pPlugin2Handler)
   {
      IHXValues* pPluginProps;
      const char* szPropName;
      IHXBuffer* pPropValue;
      char *value;
      HX_RESULT ret;
      MimeList *ml;
      mimehead = 0;
      char mime[1024];
      char ext[1024];
      bool hasmime, hasexts;

      pPluginProps = 0;
      int n = pPlugin2Handler->GetNumOfPlugins2();
      m_numPlugins = n;
      STDERR("Got the plugin2 handler: numplugins =  %d\n", n);
      m_pluginInfo = new pluginInfo* [n];
      for (i=0; i<n; i++)
      {
         m_pluginInfo[i] = new pluginInfo;
         m_pluginInfo[i]->description = "";
         m_pluginInfo[i]->copyright = "";
         m_pluginInfo[i]->moreinfourl = "";
         hasmime = hasexts = false;
         pPlugin2Handler->GetPluginInfo(i, pPluginProps);
         if (pPluginProps)
         {
            ret = pPluginProps->GetFirstPropertyCString(szPropName, pPropValue);
            while(SUCCEEDED(ret))
            {
               value = (char*)pPropValue->GetBuffer();
               if (!strcmp(szPropName, "FileMime"))
               {
                  strcpy(mime, value);
                  hasmime = true;
               }

               if (!strcmp(szPropName, "FileExtensions"))
               {
                  strcpy(ext, value);
                  hasexts = true;
               }

               if (!strcmp(szPropName, "Description"))
               {
                  m_pluginInfo[i]->description = new char[ strlen(value) + 1 ];
                  strcpy(m_pluginInfo[i]->description, value);
               }

               if (!strcmp(szPropName, "Copyright"))
               {
                  m_pluginInfo[i]->copyright = new char[ strlen(value) + 1 ];
                  strcpy(m_pluginInfo[i]->copyright, value);
               }


               if (!strcmp(szPropName, "PlgCopy"))
               {
                  m_pluginInfo[i]->moreinfourl = new char[ strlen(value) + 1 ];
                  strcpy(m_pluginInfo[i]->moreinfourl, value);
               }

               ret = pPluginProps->GetNextPropertyCString(szPropName, pPropValue);
            }
            HX_RELEASE(pPluginProps);
            if (hasmime && hasexts)
            {
               mimelistlen++;
               ml = new MimeList(mime, ext);
               ml->fwd = mimehead;
               mimehead = ml;
            }
         }
      }
   }
}

int HelixSimplePlayer::initDirectSS()
{
   if (m_outputsink == ALSA && m_AlsaCapableCore)
   {
      closeAudioDevice();
      m_direct = ALSA;
      openAudioDevice();
   }
   else if (m_outputsink == ALSA)
   {
      closeAudioDevice();
      m_direct = OSS;
      m_outputsink = OSS;
      openAudioDevice();
      return -1;
   }
   else
   {
      closeAudioDevice();
      m_direct = OSS;
      openAudioDevice();
   }

   m_volBefore = m_volAtStart = getDirectHWVolume();
   STDERR("***VolAtStart is %d\n", m_volAtStart);
   setDirectHWVolume(m_volAtStart);
   return 0;
}

int HelixSimplePlayer::addPlayer()
{
   if ((nNumPlayers+1) == MAX_PLAYERS)
   {
      STDERR("MAX_PLAYERS: %d   nNumPlayers: %d\n", MAX_PLAYERS, nNumPlayers);
      return -1;
   }

   ppctrl[nNumPlayers] = new struct playerCtrl;
   memset(ppctrl[nNumPlayers], 0, sizeof(struct playerCtrl));

   ppctrl[nNumPlayers]->bPlaying  = false;
   ppctrl[nNumPlayers]->bStarting = false;
   ppctrl[nNumPlayers]->bFadeIn   = false;
   ppctrl[nNumPlayers]->bFadeOut  = false;
   ppctrl[nNumPlayers]->ulFadeTime = 0;
   ppctrl[nNumPlayers]->pStream = 0;
   ppctrl[nNumPlayers]->pszURL = 0;

   memset(&ppctrl[nNumPlayers]->md, 0, sizeof(ppctrl[nNumPlayers]->md));

   ppctrl[nNumPlayers]->pHSPContext = new HSPClientContext(nNumPlayers, this);
   if (!ppctrl[nNumPlayers]->pHSPContext)
   {
      STDOUT("Error: Out of Memory. num players is %d\n", nNumPlayers);
      theErr = HXR_UNEXPECTED;
      return -1;
   }
   ppctrl[nNumPlayers]->pHSPContext->AddRef();

   //initialize the example context  

   char pszGUID[GUID_LEN + 1]; /* Flawfinder: ignore */ // add 1 for terminator
   //char* token = NULL;
   IHXPreferences* pPreferences = NULL;

   if (HXR_OK != pEngine->CreatePlayer(ppctrl[nNumPlayers]->pPlayer))
   {
      theErr = HXR_FAILED;
      return -1;
   }
   
   pszGUID[0] = '\0';   

// disable for now - I dont know what I was thinking
#ifdef _DISABLE_CUZ_I_MUST_HAVE_BEEN_NUTS_
   if (m_pszGUIDList)
   {
      // Get next GUID from the GUID list
      if (nNumPlayers == 0)
      {
         token = strtok(m_pszGUIDList, "\n\0");
      }
      else
      {
         token = strtok(NULL, "\n\0");
      }
      if (token)
      {
         strncpy(pszGUID, token, GUID_LEN); /* Flawfinder: ignore */
         pszGUID[GUID_LEN] = '\0';
      }
   }
#endif
   
   ppctrl[nNumPlayers]->pPlayer->QueryInterface(IID_IHXPreferences, (void**) &pPreferences);
   ppctrl[nNumPlayers]->pHSPContext->Init(ppctrl[nNumPlayers]->pPlayer, pPreferences, pszGUID);
   ppctrl[nNumPlayers]->pPlayer->SetClientContext(ppctrl[nNumPlayers]->pHSPContext);
   HX_RELEASE(pPreferences);
   
   ppctrl[nNumPlayers]->pPlayer->QueryInterface(IID_IHXErrorSinkControl, (void**) &pErrorSinkControl);
   if (pErrorSinkControl)
   {
      ppctrl[nNumPlayers]->pHSPContext->QueryInterface(IID_IHXErrorSink, (void**) &pErrorSink);
      if (pErrorSink)
         pErrorSinkControl->AddErrorSink(pErrorSink, HXLOG_EMERG, HXLOG_INFO);
      HX_RELEASE(pErrorSink);
   }
   
   HX_RELEASE(pErrorSinkControl);

   // Get the Player2 interface
   ppctrl[nNumPlayers]->pPlayer->QueryInterface(IID_IHXPlayer2, (void**) &ppctrl[nNumPlayers]->pPlayer2);
   if (!ppctrl[nNumPlayers]->pPlayer2)
      STDERR("no player2 device\n");
   
   // Get the Audio Player
   ppctrl[nNumPlayers]->pPlayer->QueryInterface(IID_IHXAudioPlayer, (void**) &ppctrl[nNumPlayers]->pAudioPlayer);
   if (ppctrl[nNumPlayers]->pAudioPlayer)
   {
      // ...and now the volume interface
      //ppctrl[nNumPlayers]->pVolume = ppctrl[nNumPlayers]->pAudioPlayer->GetAudioVolume();
      //ppctrl[nNumPlayers]->pVolume = ppctrl[nNumPlayers]->pAudioPlayer->GetDeviceVolume();
      if (ppctrl[nNumPlayers]->pVolume)
      {
#ifndef HELIX_SW_VOLUME_INTERFACE
         HelixSimplePlayerVolumeAdvice *pVA = new HelixSimplePlayerVolumeAdvice(this, nNumPlayers);
         pVA->AddRef();
         ppctrl[nNumPlayers]->pVolume->AddAdviseSink((IHXVolumeAdviseSink *)pVA);
         ppctrl[nNumPlayers]->pVolumeAdvise = pVA;
         ppctrl[nNumPlayers]->volume = 50; // should get volume advise, which will set this properly
#else
         // set the helix sw interface volume to 100, we'll control the volume either ourselves post equalization or by
         // amaorok using direct hardware volume
         //ppctrl[nNumPlayers]->pVolume->SetVolume(100);
         ppctrl[nNumPlayers]->pVolume->SetVolume(100);
         ppctrl[nNumPlayers]->pVolumeAdvise = 0;
#endif
      }

      // add the IHXAudioStreamInfoResponse it the AudioPlayer
      HelixSimplePlayerAudioStreamInfoResponse *pASIR = new HelixSimplePlayerAudioStreamInfoResponse(this, nNumPlayers);
      ppctrl[nNumPlayers]->pAudioPlayer->SetStreamInfoResponse(pASIR);
      ppctrl[nNumPlayers]->pStreamInfoResponse = pASIR;

      // ...and get the CrossFader
      ppctrl[nNumPlayers]->pAudioPlayer->QueryInterface(IID_IHXAudioCrossFade, (void **) &(ppctrl[nNumPlayers]->pCrossFader));
      if (!ppctrl[nNumPlayers]->pCrossFader)
         STDERR("CrossFader not available\n");

      // install hook for visualizations, equalizer, and volume control
      HSPPostMixAudioHook *pPMAH = new HSPPostMixAudioHook(this, nNumPlayers);
      ppctrl[nNumPlayers]->pAudioPlayer->AddPostMixHook(pPMAH, false, true);
      ppctrl[nNumPlayers]->pPostMixHook= pPMAH;
   }
   else
      STDERR("No AudioPlayer Found - how can we play music!!\n");
   
   ++nNumPlayers;

   //STDERR("Added player, total is %d\n",nNumPlayers);
   return 0;
}

HelixSimplePlayer::~HelixSimplePlayer()
{
   tearDown();

   // only delete now, not evertime you re-init the engine, since you need this to re-init the engine
   delete [] m_device;
}

void HelixSimplePlayer::tearDown()
{
   int i;
   FPRMCLOSEENGINE         fpCloseEngine;

   if (theErr != HXR_OK) // failed to initialize properly
      return;

   // make sure all players are stopped,
   stop();

   STDERR("TEARDOWN\n");

   for (i=nNumPlayers-1; i>=0; i--)
   {
      if (ppctrl[i]->pCrossFader)
         ppctrl[i]->pCrossFader->Release();

      if (ppctrl[i]->pAudioPlayer)
      {
         ppctrl[i]->pAudioPlayer->RemovePostMixHook(ppctrl[i]->pPostMixHook);
         ppctrl[i]->pPostMixHook->Release();

         ppctrl[i]->pAudioPlayer->RemoveStreamInfoResponse((IHXAudioStreamInfoResponse *) ppctrl[i]->pStreamInfoResponse);

         if (ppctrl[i]->pVolume)
         {
            if (ppctrl[i]->pVolumeAdvise)
            {
               ppctrl[i]->pVolume->RemoveAdviseSink(ppctrl[i]->pVolumeAdvise);
               ppctrl[i]->pVolumeAdvise->Release();
            }
            ppctrl[i]->pVolume->Release();
         }

         ppctrl[i]->pAudioPlayer->Release();
      }

      if ( ppctrl[i]->pszURL )
         delete [] ppctrl[i]->pszURL;

      if (ppctrl[i]->pHSPContext)
         ppctrl[i]->pHSPContext->Release();

      if (ppctrl[i]->pPlayer2)
         ppctrl[i]->pPlayer2->Release();

      if (ppctrl[i]->pPlayer && pEngine)
      {
         pEngine->ClosePlayer(ppctrl[i]->pPlayer);
         ppctrl[i]->pPlayer->Release();
      }
 
      delete ppctrl[i];
  }

   delete [] ppctrl;

   if (pCommonClassFactory)
      pCommonClassFactory->Release();
   if (pCEselect)
      pCEselect->Release();
   if (pPluginE)
      pPluginE->Release();
   if (pPlugin2Handler)
      pPlugin2Handler->Release();
   if (pAudioHookManager)
   {
      pAudioHookManager->RemoveHook(pFinalAudioHook);
      pFinalAudioHook->Release();
      pAudioHookManager->Release();
   }
   if (pEngineContext)
      pEngineContext->Release();

   fpCloseEngine  = (FPRMCLOSEENGINE) dlsym(core_handle, "CloseEngine");
   if (fpCloseEngine && pEngine)
   {
      fpCloseEngine(pEngine);
      pEngine = NULL;
   }

   dlclose(core_handle);

   if (m_pszUsername)
   {
      delete [] m_pszUsername;
   }
   if (m_pszPassword)
   {
      delete [] m_pszPassword;
   }
   if (m_pszGUIDFile)
   {
      delete [] m_pszGUIDFile;
   }
   if (m_pszGUIDList)
   {
      delete [] m_pszGUIDList;
   }

   for (i=0; i<m_numPlugins; i++)
      delete m_pluginInfo[i];
   delete [] m_pluginInfo;

   if (bEnableVerboseMode)
   {
      STDOUT("\nDone.\n");
   }

   MimeList *ml = mimehead, *mh;
   while (ml)
   {
      mh = ml->fwd;
      delete ml;
      ml = mh;
   }

   closeAudioDevice();
   
   theErr = HXR_FAILED;
   pErrorSink = NULL;
   pErrorSinkControl = NULL;
   pPluginE = 0;
   pPlugin2Handler = 0;
   ppctrl = NULL;
   bURLFound = false;
   nNumPlayers = 0;
   nNumPlayRepeats = 1;
   nTimeDelta = DEFAULT_TIME_DELTA;
   nStopTime = DEFAULT_STOP_TIME;
   bStopTime = true; 
   bStopping = false;
   nPlay = 0;
   bEnableAdviceSink = false;
   bEnableVerboseMode = false;
   pEngine = NULL;
   m_pszUsername = NULL;
   m_pszPassword = NULL;
   m_pszGUIDFile = NULL;
   m_pszGUIDList = NULL;
   m_Error = 0;
   m_ulNumSecondsPlayed = 0;
   mimehead = 0;
   scopecount = 0;
   scopebufhead = 0;
   scopebuftail = 0;
   m_preamp = 0;
}


void HelixSimplePlayer::setOutputSink( HelixSimplePlayer::AUDIOAPI out ) 
{ 
#ifdef USE_HELIX_ALSA
   m_outputsink = out;
#else
   m_outputsink = OSS;
#endif
}

HelixSimplePlayer::AUDIOAPI HelixSimplePlayer::getOutputSink() 
{ 
   return m_outputsink; 
}

void HelixSimplePlayer::setDevice( const char *dev ) 
{ 
   delete [] m_device;

   int len = strlen(dev);
   m_device = new char [len + 1];
   strcpy(m_device, dev); 
}

const char *HelixSimplePlayer::getDevice() 
{ 
   return m_device; 
}


#define MAX_DEV_NAME 255
#define HX_VOLUME  SOUND_MIXER_PCM
void HelixSimplePlayer::openAudioDevice()
{
   switch (m_direct)
   {
      case OSS:
      {
         //Check the environmental variable to let user overide default device.
         char *pszOverrideName = getenv( "AUDIO" ); /* Flawfinder: ignore */
         char szDevName[MAX_DEV_NAME]; /* Flawfinder: ignore */
         
         // Use defaults if no environment variable is set.
         if ( pszOverrideName && strlen(pszOverrideName)>0 )
         {
            SafeStrCpy( szDevName, pszOverrideName, MAX_DEV_NAME );
         }
         else
         {
            SafeStrCpy( szDevName, "/dev/mixer", MAX_DEV_NAME );
         }
         
         // Open the audio device if it isn't already open
         if ( m_nDevID < 0 )
         {
            m_nDevID = ::open( szDevName, O_WRONLY );
         }
         
         if ( m_nDevID < 0 )
         {
            STDERR("Failed to open audio(%s)!!!!!!! Code is: %d  errno: %d\n",
                   szDevName, m_nDevID, errno );
        
            //Error opening device.
         }
      }
      break;

      case ALSA:
      {
#ifdef USE_HELIX_ALSA
         int err;
         
         STDERR("Opening ALSA mixer device PCM\n");

         err = snd_mixer_open(&m_pAlsaMixerHandle, 0);
         if (err < 0)
            STDERR("snd_mixer_open: %s\n", snd_strerror (err));
  
         if (err == 0)
         {
            err = snd_mixer_attach(m_pAlsaMixerHandle, m_alsaDevice);
            if (err < 0) 
               STDERR("snd_mixer_attach: %s\n", snd_strerror (err));
         }

         if (err == 0)
         {
            err = snd_mixer_selem_register(m_pAlsaMixerHandle, NULL, NULL);
            if (err < 0) 
               STDERR("snd_mixer_selem_register: %s\n", snd_strerror (err));
         }

         if (err == 0)
         {
            err = snd_mixer_load(m_pAlsaMixerHandle);
            if(err < 0 )
               STDERR("snd_mixer_load: %s\n", snd_strerror (err));
         }

         if (err == 0)
         {
            /* Find the mixer element */
            snd_mixer_elem_t* elem = snd_mixer_first_elem(m_pAlsaMixerHandle);
            snd_mixer_elem_type_t type;
            const char* elem_name = NULL;
            snd_mixer_selem_id_t *sid = NULL;

            snd_mixer_selem_id_alloca(&sid);

            while (elem)
            {
               type = snd_mixer_elem_get_type(elem);
               if (type == SND_MIXER_ELEM_SIMPLE)
               {
                  snd_mixer_selem_get_id(elem, sid);

                  /* We're only interested in playback volume controls */
                  if(snd_mixer_selem_has_playback_volume(elem) && !snd_mixer_selem_has_common_volume(elem) )
                  {
                     elem_name = snd_mixer_selem_id_get_name(sid);
                     if (strcmp(elem_name, "PCM") == 0) // take the first PCM element we find
                        break;
                  }
               }
            
               elem = snd_mixer_elem_next(elem);
            }

            if (!elem)
            {
               STDERR("Could not find a usable mixer element\n", snd_strerror(err));
               err = -1;
            }
            
            m_pAlsaMixerElem = elem;
         }
         
         if (err != 0)
         {
            if(m_pAlsaMixerHandle)
            {
               snd_mixer_close(m_pAlsaMixerHandle);
               m_pAlsaMixerHandle = NULL;
            }
         }
#endif
      }
      break;

      default:
         STDERR("Unknown audio interface in openAudioDevice()\n");
   }
}

void HelixSimplePlayer::closeAudioDevice()
{
   switch (m_direct)
   {
      case OSS:
      {
         if( m_nDevID >= 0 )
         {
            ::close( m_nDevID );
            m_nDevID = -1;
         }
      }
      break;

      case ALSA:
      {
#ifdef USE_HELIX_ALSA
         int err;

         if (m_pAlsaMixerHandle && m_pAlsaMixerElem)
         {
            err = snd_mixer_detach(m_pAlsaMixerHandle, "PCM");
            if (err < 0)
               STDERR("snd_mixer_detach: %s\n", snd_strerror(err));
  
            if(err == 0)
            {
               err = snd_mixer_close(m_pAlsaMixerHandle);
               if(err < 0)
                  STDERR("snd_mixer_close: %s\n", snd_strerror (err));            
            }
            
            if(err == 0)
            {
               m_pAlsaMixerHandle = NULL;
               m_pAlsaMixerElem = NULL;
            }
         }
#endif
      }
      break;

      default:
         STDERR("Unknown audio interface in openAudioDevice()\n");
   }
}

int HelixSimplePlayer::getDirectHWVolume()
{
   int nRetVolume   = 0;

   switch (m_direct)
   {
      case OSS:
      {
         int nVolume      = 0;
         int nLeftVolume  = 0;
         int nRightVolume = 0;
    
         if (m_nDevID < 0 || (::ioctl( m_nDevID, MIXER_READ(HX_VOLUME), &nVolume) < 0))
         {
            STDERR("ioctl fails when reading HW volume: mnDevID=%d, errno=%d\n", m_nDevID, errno);
            nRetVolume = 50; // sensible default
         }
         else
         {
            nLeftVolume  = (nVolume & 0x000000ff); 
            nRightVolume = (nVolume & 0x0000ff00) >> 8;
         
            //Which one to use? Average them?
            nRetVolume = nLeftVolume ;
         }
      }
      break;

      case ALSA:
      {
#ifdef USE_HELIX_ALSA
         if (!m_pAlsaMixerElem)
            return nRetVolume;

         snd_mixer_elem_type_t type;    
         int err = 0;
         type = snd_mixer_elem_get_type(m_pAlsaMixerElem);
            
         if (type == SND_MIXER_ELEM_SIMPLE)
         {
            long volumeL, volumeR, min_volume, max_volume; 

            if(snd_mixer_selem_has_playback_volume(m_pAlsaMixerElem) || 
               snd_mixer_selem_has_playback_volume_joined(m_pAlsaMixerElem))
            {
               err = snd_mixer_selem_get_playback_volume(m_pAlsaMixerElem,
                                                         SND_MIXER_SCHN_FRONT_LEFT, 
                                                         &volumeL);
               if (err < 0)
                  STDERR("snd_mixer_selem_get_playback_volume (L): %s\n", snd_strerror (err));
               else
               {
                  if ( snd_mixer_selem_is_playback_mono ( m_pAlsaMixerElem )) 
                     volumeR = volumeL;
                  else
                  {
                     err = snd_mixer_selem_get_playback_volume(m_pAlsaMixerElem,
                                                               SND_MIXER_SCHN_FRONT_RIGHT, 
                                                               &volumeR);
                     if (err < 0)
                        STDERR("snd_mixer_selem_get_playback_volume (R): %s\n", snd_strerror (err));
                  }
               }
                  
               if (err == 0)
               {
                  snd_mixer_selem_get_playback_volume_range(m_pAlsaMixerElem,
                                                            &min_volume,
                                                            &max_volume);

                  if(max_volume > min_volume)
                     nRetVolume = (UINT16) (0.5 + (100.0 * (double)(volumeL + volumeR) / (2.0 * (max_volume - min_volume))));
               }
            }        
         }
#endif
      }
      break;

      default:
         STDERR("Unknown audio interface in getDirectHWVolume()\n");
   }

   return nRetVolume; 
}

void HelixSimplePlayer::setDirectHWVolume(int vol)
{
   switch (m_direct)
   {
      case OSS:
      {
         int nNewVolume=0;

         //Set both left and right volumes.
         nNewVolume = (vol & 0xff) | ((vol & 0xff) << 8);
    
         if (::ioctl( m_nDevID, MIXER_WRITE(HX_VOLUME), &nNewVolume) < 0)
            STDERR("Unable to set direct HW volume\n");
      }
      break;

      case ALSA:
      {
#ifdef USE_HELIX_ALSA
         if (!m_pAlsaMixerElem)
            return;

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
               volume = (long) (((double)vol / 100) * range + min_volume);


               err = snd_mixer_selem_set_playback_volume( m_pAlsaMixerElem,
                                                          SND_MIXER_SCHN_FRONT_LEFT, 
                                                          volume);            
               if (err < 0)
                  STDERR("snd_mixer_selem_set_playback_volume: %s\n", snd_strerror (err));

               if (!snd_mixer_selem_is_playback_mono (m_pAlsaMixerElem))
               {
                  /* Set the right channel too */
                  err = snd_mixer_selem_set_playback_volume( m_pAlsaMixerElem,
                                                             SND_MIXER_SCHN_FRONT_RIGHT, 
                                                             volume);            
                  if (err < 0)
                     STDERR("snd_mixer_selem_set_playback_volume: %s\n", snd_strerror (err));
               }
            }        
         }    
#endif
      }
      break;

      default:
         STDERR("Unknown audio interface in setDirectHWVolume()\n");
   }
}

bool HelixSimplePlayer::isLocal(int playerIndex) const
{
   return (ppctrl[playerIndex]->isLocal && duration(playerIndex)); // must have a length to be local
}

int HelixSimplePlayer::setURL(const char *file, int playerIndex, bool islocal)
{
   if (playerIndex == ALL_PLAYERS)
   {
      int i;
      
      for (i=0; i<nNumPlayers; i++)
         setURL(file, i);
   }
   else
   {
      int len = strlen(file);
      if (len >= MAXPATHLEN)
         return -1;;

      if (ppctrl[playerIndex]->pszURL)
         delete [] ppctrl[playerIndex]->pszURL;
      
      // see if the file is already in the form of a url
      char *tmp = strstr(file, "://");
      if (!tmp)
      {
         char pszURLOrig[MAXPATHLEN];
         const char* pszAddOn;
         
         strcpy(pszURLOrig, file);
         RemoveWrappingQuotes(pszURLOrig);
         pszAddOn = "file://";
         
         ppctrl[playerIndex]->pszURL = new char[strlen(pszURLOrig)+strlen(pszAddOn)+1];
         if ( (len + strlen(pszAddOn)) < MAXPATHLEN )
         {
            islocal = true;
            sprintf( ppctrl[playerIndex]->pszURL, "%s%s", pszAddOn, pszURLOrig );
         }
         else
            return -1;
      }
      else
      {
         ppctrl[playerIndex]->pszURL = new char[len + 1];
         if (ppctrl[playerIndex]->pszURL)
            strcpy(ppctrl[playerIndex]->pszURL, file);
         else
            return -1;
      }

      ppctrl[playerIndex]->isLocal = islocal;

#ifdef __NOCROSSFADER__
      STDERR("opening %s on player %d, src cnt %d\n", 
             ppctrl[playerIndex]->pszURL, playerIndex, ppctrl[playerIndex]->pPlayer->GetSourceCount());

      if (HXR_OK == ppctrl[playerIndex]->pPlayer->OpenURL(ppctrl[playerIndex]->pszURL))
      {
         STDERR("opened player on %d src cnt %d\n", playerIndex, ppctrl[playerIndex]->pPlayer->GetSourceCount());         
         m_urlchanged = true;
      }
#else
      /* try OpenRequest instead... */
      IHXRequest *ireq = 0;
      pthread_mutex_lock(&m_engine_m);
      pCommonClassFactory->CreateInstance(CLSID_IHXRequest, (void **)&ireq);
      if (ireq)
      {
         //STDERR("GOT THE IHXRequest Interface!!\n");
         ireq->SetURL(ppctrl[playerIndex]->pszURL);
         ppctrl[playerIndex]->pPlayer2->OpenRequest(ireq);
         m_urlchanged = true;
         ireq->Release();
      }
      pthread_mutex_unlock(&m_engine_m);

#endif
      
   }
   
   return 0;
}

int HelixSimplePlayer::numPlugins() const
{
   if (pPluginE)
   {
      return ((int)pPluginE->GetNumOfPlugins());
   }

   return 0;
}

int HelixSimplePlayer::getPluginInfo(int index, 
                                     const char *&description, 
                                     const char *&copyright, 
                                     const char *&moreinfourl) const
{
   if (index < m_numPlugins)
   {
      description = m_pluginInfo[index]->description;
      copyright   = m_pluginInfo[index]->copyright;
      moreinfourl = m_pluginInfo[index]->moreinfourl;
   
      return 0;
   }
   return -1;
}

void HelixSimplePlayer::enableCrossFader(int playerFrom, int playerTo)
{
   if (playerFrom < nNumPlayers && playerTo < nNumPlayers)
   {
      m_xf.crossfading = true;
      m_xf.fromIndex = playerFrom;
      m_xf.toIndex = playerTo;
   }   
}

void HelixSimplePlayer::disableCrossFader()
{
   m_xf.crossfading = false;
}

// the purpose is to setup the next stream for crossfading.  the player needs to get the new stream in the AudioPlayerResponse before the cross fade can begin
void HelixSimplePlayer::crossFade(const char *url, unsigned long /*startPos*/, unsigned long xfduration)
{
   if (m_xf.crossfading)
   {
      m_xf.duration = xfduration;
      m_xf.fromStream = 0;
      m_xf.fromStream = ppctrl[m_xf.fromIndex]->pAudioPlayer->GetAudioStream(0);
      if (m_xf.fromStream)
      {
         STDERR("Got Stream 1\n");
         setURL(url, m_xf.toIndex);
      }
      else
         disableCrossFader(); // what else can I do?
   }
}


void HelixSimplePlayer::startCrossFade()
{
   if (xf().crossfading)
   {
      // figure out when to start the crossfade
      int startFrom = duration(xf().fromIndex) - xf().duration;
      //int whereFrom = where(xf().fromIndex) + 1000; // 1 sec is just majic...otherwise the crossfader just doesnt work

      // only fade in the new stream if we are playing one already
      if (xf().fromStream)
      {
         STDERR("Player %d where %d  duration %d  startFrom %d\n", xf().fromIndex, where(xf().fromIndex), duration(xf().fromIndex), startFrom);
         
         // fade out the now-playing-stream
         //(getCrossFader(xf().fromIndex))->CrossFade(xf().fromStream, 0, startFrom > whereFrom ? startFrom : whereFrom, 0, xf().duration);

         // fade in the new stream
         //(getCrossFader(xf().toIndex))->CrossFade(0, xf().toStream, 0, 0, xf().duration);
         start(xf().toIndex);
         
         // switch from and to for the next stream
         int index = xf().toIndex;
         xf().toIndex = xf().fromIndex;
         xf().fromIndex = index;
         xf().fromStream = xf().toStream = 0;
      }
      else
      {
         // here we suppose that we should be starting the first track
         if (xf().toStream) // this would mean that the stream could not be initialize, or wasnt finished initializing yet
            start(xf().toIndex);
      }
   }
}


void HelixSimplePlayer::play(const char *file, int playerIndex, bool fadein, bool fadeout, unsigned long fadetime)
{
   if (!setURL(file, playerIndex))
      play(playerIndex, fadein, fadeout, fadetime);
}

void HelixSimplePlayer::play(int playerIndex, bool fadein, bool fadeout, unsigned long fadetime)
{
   int i;
   int firstPlayer = playerIndex == ALL_PLAYERS ? 0 : playerIndex;
   int lastPlayer  = playerIndex == ALL_PLAYERS ? nNumPlayers : playerIndex + 1;
   
   nPlay = 0;
   nNumPlayRepeats=1;
   while(nPlay < nNumPlayRepeats) 
   {
      nPlay++;
      if (bEnableVerboseMode)
      {
         STDOUT("Starting play #%d...\n", nPlay);
      }
      //STDERR("firstplayer = %d  lastplayer=%d\n",firstPlayer,lastPlayer);
      
      UINT32 starttime, endtime, now;
      for (i = firstPlayer; i < lastPlayer; i++)
      {
         // start is already protected...
         start(i, fadein, fadeout, fadetime);

         starttime = GetTime();
         endtime = starttime + nTimeDelta;
         while (1)
         {
            pthread_mutex_lock(&m_engine_m);
            DoEvents(nTimeDelta);
            pthread_mutex_unlock(&m_engine_m);
            now = GetTime();
            if (now >= endtime)
               break;
         }
      }

      starttime = GetTime();
      if (nStopTime == -1)
      {
         bStopTime = false;
      }
      else
      {
         endtime = starttime + nStopTime;
      }
      bStopping = false;
      // Handle events coming from all of the players
      while (!done(playerIndex))
      {
         now = GetTime();
         if (!bStopping && bStopTime && now >= endtime)
         {
	    // Stop all of the players, as they should all be done now
	    if (bEnableVerboseMode)
	    {
               STDOUT("\nEnd (Stop) time reached. Stopping...\n");
	    }
	    stop(playerIndex);
            bStopping = true;
         }
         pthread_mutex_lock(&m_engine_m);
         DoEvent();
         pthread_mutex_unlock(&m_engine_m);
      }

      // Stop all of the players, as they should all be done now
      if (bEnableVerboseMode)
      {
         STDOUT("\nPlayback complete. Stopping all players...\n");
      }
      stop(playerIndex);

      // repeat until nNumRepeats
   }
}

void HelixSimplePlayer::start(int playerIndex, bool fadein, bool fadeout, unsigned long fadetime)
{
   if (playerIndex == ALL_PLAYERS)
   {
      int i;
      for (i=0; i<nNumPlayers; i++)
         start(i, fadein, fadeout, fadetime);
   }
   else
   {
      if (!ppctrl[playerIndex]->pszURL)
         return;
      
      if (bEnableVerboseMode)
      {
         STDOUT("Starting player %d...\n", playerIndex);
      }

      ppctrl[playerIndex]->bFadeIn = fadein;
      ppctrl[playerIndex]->bFadeOut = fadeout;
      ppctrl[playerIndex]->ulFadeTime = fadetime;
      if (!ppctrl[playerIndex]->bPlaying)
      {
         pthread_mutex_lock(&m_engine_m);
         ppctrl[playerIndex]->pPlayer->Begin();
         pthread_mutex_unlock(&m_engine_m);

         ppctrl[playerIndex]->bPlaying = true;
         ppctrl[playerIndex]->bStarting = true;
         //STDERR("Begin player %d\n", playerIndex);
      }
   }
}


void HelixSimplePlayer::start(const char *file, int playerIndex, bool fadein, bool fadeout, unsigned long fadetime)
{
   setURL(file, playerIndex);
   start(playerIndex, fadein, fadeout, fadetime);
}



bool HelixSimplePlayer::done(int playerIndex)
{
   BOOL bAllDone = true;

   if (playerIndex == ALL_PLAYERS)
      // Start checking at the end of the array since those players
      // were started last and are therefore more likely to not be
      // finished yet.
      for (int i = nNumPlayers - 1; i >= 0 && bAllDone; i--)
      {
         pthread_mutex_lock(&m_engine_m);
         if (ppctrl[i]->bStarting || !ppctrl[i]->pPlayer->IsDone())
            ppctrl[i]->bPlaying = (bAllDone = false);
         pthread_mutex_unlock(&m_engine_m);
      }
   else
   {
      if (playerIndex < nNumPlayers)
      {
         pthread_mutex_lock(&m_engine_m);
         if ((bAllDone = (!ppctrl[playerIndex]->bStarting && ppctrl[playerIndex]->pPlayer->IsDone())))
            ppctrl[playerIndex]->bPlaying = false;
         pthread_mutex_unlock(&m_engine_m);
      }
   }

   return bAllDone;
}

void HelixSimplePlayer::stop(int playerIndex)
{
   if (playerIndex == ALL_PLAYERS)
      for (int i = 0; i < nNumPlayers; i++)
      {
         pthread_mutex_lock(&m_engine_m);
         ppctrl[i]->pPlayer->Stop();
         pthread_mutex_unlock(&m_engine_m);

         ppctrl[i]->bPlaying = false;
         ppctrl[i]->bStarting = false;
      }
   else
   {
      if (playerIndex < nNumPlayers)
      {
         pthread_mutex_lock(&m_engine_m);
         ppctrl[playerIndex]->pPlayer->Stop();
         pthread_mutex_unlock(&m_engine_m);
         ppctrl[playerIndex]->bPlaying = false;
         ppctrl[playerIndex]->bStarting = false;
         memset(&ppctrl[playerIndex]->md, 0, sizeof(ppctrl[playerIndex]->md));
      }
   }
}

HelixSimplePlayer::metaData *HelixSimplePlayer::getMetaData(int playerIndex)
{
   return &ppctrl[playerIndex]->md;
}


void HelixSimplePlayer::dispatch()
{
   struct _HXxEvent *pNothing = 0x0;
   struct timeval tv;
   int volAfter = 0;
   
   tv.tv_sec = 0;
   tv.tv_usec = SLEEP_TIME*1000;

   if (m_urlchanged)
   {
      m_volBefore = getDirectHWVolume();
      m_urlchanged = false;
      STDERR("Volume is: %d\n", m_volBefore);
   }
   pEngine->EventOccurred(pNothing);
   if (m_volBefore > 0 && m_volAtStart != m_volBefore && (volAfter = getDirectHWVolume()) != m_volBefore)
   {
      STDERR("RESETTING VOLUME TO: %d\n", m_volBefore);
      setDirectHWVolume(m_volBefore);
      STDERR("Now Volume is %d\n", getDirectHWVolume());
      m_volBefore = -1;
   }
}


void HelixSimplePlayer::pause(int playerIndex)
{
   int i;
   
   if (playerIndex == ALL_PLAYERS)
      for (i=0; i<nNumPlayers; i++)
         pause(i);
   else
      if (playerIndex < nNumPlayers)
      {
         pthread_mutex_lock(&m_engine_m);
         ppctrl[playerIndex]->pPlayer->Pause();
         pthread_mutex_unlock(&m_engine_m);
         ppctrl[playerIndex]->bPlaying = false;
      }
}

void HelixSimplePlayer::resume(int playerIndex)
{
   int i;
   
   if (playerIndex == ALL_PLAYERS)
      for (i=0; i<nNumPlayers; i++)
         resume(i);
   else
      if (playerIndex < nNumPlayers)
      {
         pthread_mutex_lock(&m_engine_m);
         ppctrl[playerIndex]->pPlayer->Begin();
         pthread_mutex_unlock(&m_engine_m);
         ppctrl[playerIndex]->bPlaying = true;
      }
}


void HelixSimplePlayer::seek(unsigned long pos, int playerIndex)
{
   int i;
   
   if (playerIndex == ALL_PLAYERS)
      for (i=0; i<nNumPlayers; i++)
         seek(pos, i);
   else
      if (playerIndex < nNumPlayers)
      {
         pthread_mutex_lock(&m_engine_m);
         ppctrl[playerIndex]->pPlayer->Seek(pos);
         pthread_mutex_unlock(&m_engine_m);
      }
}

unsigned long HelixSimplePlayer::where(int playerIndex) const
{
   if (playerIndex < nNumPlayers && ppctrl[playerIndex]->pHSPContext)
      //return ppctrl[playerIndex]->pHSPContext->position();
      return ppctrl[playerIndex]->pPlayer->GetCurrentPlayTime();
   else
      return 0;
}

unsigned long HelixSimplePlayer::duration(int playerIndex) const
{
   if (playerIndex < nNumPlayers && ppctrl[playerIndex]->pHSPContext)
      return ppctrl[playerIndex]->pHSPContext->duration();
   else
      return 0;
}

unsigned long HelixSimplePlayer::getVolume(int playerIndex)
{
   unsigned long vol;

   if (playerIndex < nNumPlayers)
   {
      pthread_mutex_lock(&m_engine_m);
      vol = ppctrl[playerIndex]->volume;
      //if (ppctrl[playerIndex]->pVolume)
         //pVolume->GetVolume();
      pthread_mutex_unlock(&m_engine_m);

      return (vol);
   }
   else
      return 0;
}

void HelixSimplePlayer::setVolume(unsigned long vol, int playerIndex)
{
   int i;
   
   if (playerIndex == ALL_PLAYERS)
   {
      for (i=0; i<nNumPlayers; i++)
         setVolume(vol, i);
   }
   else
      if (playerIndex < nNumPlayers)
      {
         pthread_mutex_lock(&m_engine_m);
#ifndef HELIX_SW_VOLUME_INTERFACE
         ppctrl[playerIndex]->volume = vol;
         ((HSPFinalAudioHook *)pFinalAudioHook)->setGain(vol);
#else
         ppctrl[playerIndex]->pVolume->SetVolume(vol);
#endif
         pthread_mutex_unlock(&m_engine_m);
      }
}

void HelixSimplePlayer::setMute(bool mute, int playerIndex)
{
   int i;
   
   if (playerIndex == ALL_PLAYERS)
   {
      for (i=0; i<nNumPlayers; i++)
         setMute(mute, i);
   }
   else
      if (playerIndex < nNumPlayers)
      {
         pthread_mutex_lock(&m_engine_m);
         ppctrl[playerIndex]->pVolume->SetMute(mute);
         pthread_mutex_unlock(&m_engine_m);
      }
}


bool HelixSimplePlayer::getMute(int playerIndex)
{
   bool ismute;

   if (playerIndex < nNumPlayers)
   {
      pthread_mutex_lock(&m_engine_m);
      ismute = ppctrl[playerIndex]->ismute;
//pVolume->GetMute();
      pthread_mutex_unlock(&m_engine_m);
      
      return ismute;
   }
   else
      return false;
}

bool HelixSimplePlayer::ReadGUIDFile()
{
   BOOL  bSuccess = false;
   FILE* pFile    = NULL;
   int   nNumRead = 0;
   int   readSize = 10000;
   char*  pszBuffer = new char[readSize];

   if (m_pszGUIDFile)
   {
      if((pFile = fopen(m_pszGUIDFile, "r")) != NULL)
      {
         // Read in the entire file
         nNumRead = fread(pszBuffer, sizeof(char), readSize, pFile);
         pszBuffer[nNumRead] = '\0';
         
         // Store it for later parsing
         m_pszGUIDList = new char[nNumRead + 1];
         strcpy(m_pszGUIDList, pszBuffer); /* Flawfinder: ignore */
         
         fclose(pFile);
         pFile = NULL;

         if (nNumRead > 0)
         {
            bSuccess = true;
         }
      }
   }

   delete [] pszBuffer;
   
   return bSuccess;
}


void HelixSimplePlayer::addScopeBuf(struct DelayQueue *item) 
{
   pthread_mutex_lock(&m_scope_m);
   
   if (scopebuftail)
   {
      item->fwd = 0;
      scopebuftail->fwd = item;
      scopebuftail = item;
      scopecount++;
   }
   else
   {
      item->fwd = 0;
      scopebufhead = item;
      scopebuftail = item;
      scopecount = 1;
   }
   pthread_mutex_unlock(&m_scope_m);
}

struct DelayQueue *HelixSimplePlayer::getScopeBuf()
{
   pthread_mutex_lock(&m_scope_m);
   
   struct DelayQueue *item = scopebufhead;
   
   if (item)
   {
      scopebufhead = item->fwd;
      scopecount--;
      if (!scopebufhead)
         scopebuftail = 0;
   }
   
   pthread_mutex_unlock(&m_scope_m);
   
   return item;
}

int HelixSimplePlayer::peekScopeTime(unsigned long &t) 
{
   if (scopebufhead)
      t = scopebufhead->time;
   else
      return -1;
   return 0;
}

void HelixSimplePlayer::clearScopeQ()
{
   struct DelayQueue *item;
   while ((item = getScopeBuf()))
      delete item;
}


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

#include "hspadvisesink.h"
#include "hsperror.h"
#include "hspauthmgr.h"
#include "hspcontext.h"
#include "print.h"
#include <X11/Xlib.h>
#include <dlfcn.h>
#include <errno.h>

#include "hxausvc.h"

#include "dllpath.h"

#include "globals.h" //for global struct.

#include "helix-sp.h"
#include "hspvoladvise.h"
#include "utils.h"

struct _stGlobals* HelixSimplePlayer::g_pstGlobals = NULL;

struct _stGlobals*& HelixSimplePlayer::GetGlobal()
{
   if( g_pstGlobals == NULL )
   {
      g_pstGlobals = new struct _stGlobals();
   }
   return g_pstGlobals;   
}

class HelixSimplePlayerAudioStreamInfoResponse : public IHXAudioStreamInfoResponse
{
public:
   HelixSimplePlayerAudioStreamInfoResponse(HelixSimplePlayer *player, int playerIndex) : 
      m_Player(player), m_index(playerIndex) {}
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
   int                m_index;
   LONG32             m_lRefCount;
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
        *ppvObj = (IHXVolume*)this;
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
   m_Player->xf().toStream = pAudioStream;
   STDERR("Stream Added player %d crossfade? %d\n", m_index, m_Player->xf().crossfading);
}

// Constants
const int DEFAULT_TIME_DELTA = 2000;
const int DEFAULT_STOP_TIME =  -1;
const int SLEEP_TIME         = 10;
const int GUID_LEN           = 64;

#ifdef TEST_APP
// Function prototypes
void  PrintUsage(const char* pszAppName);
char* GetAppName(char* pszArgv0);
#endif


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

STDMETHODIMP HelixSimplePlayerVolumeAdvice::OnVolumeChange(const UINT16 uVolume)
{
   STDERR("Volume change: %d\n", uVolume);
   m_Player->onVolumeChange(m_index);
}

STDMETHODIMP HelixSimplePlayerVolumeAdvice::OnMuteChange(const BOOL bMute)
{
   STDERR("Mute change: %d\n", bMute);
   m_Player->onMuteChange(m_index);
}

/*
 *  handle one event
 */
void HelixSimplePlayer::DoEvent()
{
    struct _HXxEvent* pNothing = 0;
    struct timeval    mtime;

    mtime.tv_sec  = 0;
    mtime.tv_usec = SLEEP_TIME * 1000;
    usleep(SLEEP_TIME*1000);
    GetGlobal()->pEngine->EventOccurred(pNothing);
}

/*
 *  handle events for at most nTimeDelta milliseconds
 */
void HelixSimplePlayer::DoEvents(int nTimeDelta)
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

static char* HelixSimplePlayer::RemoveWrappingQuotes(char* str)
{
   int len = strlen(str);
   if (len > 0)
   {
      if (str[len-1] == '"') str[--len] = 0;
      if (str[0] == '"') { int i = 0; do { str[i++] = str[i+1]; } while(--len); }
   }
   return str;
}


HelixSimplePlayer::HelixSimplePlayer() :
   theErr(HXR_OK),
   ppHSPContexts(NULL),
   ppPlayers(NULL),
   pPlayerNavigator(NULL),
   pErrorSink(NULL),
   pErrorSinkControl(NULL),
   ppVolume(NULL),
   ppszURL(NULL),
   bURLFound(FALSE),
   nNumPlayers(0),
   nNumPlayRepeats(1),
   nTimeDelta(DEFAULT_TIME_DELTA),
   nStopTime(DEFAULT_STOP_TIME),
   bStopTime(true),
   pszGUIDList(NULL),
   bStopping(false),
   nPlay(0)
{
   m_xf.crossfading = 0;
}

void HelixSimplePlayer::init(const char *corelibhome, const char *pluginslibhome, const char *codecshome, int numPlayers)
{
   int i;

   FPRMCREATEENGINE        fpCreateEngine;
   FPRMSETDLLACCESSPATH    fpSetDLLAccessPath;

    SafeSprintf(mCoreLibPath, MAX_PATH, "%s/%s", corelibhome, "clntcore.so");

    // Allocate arrays to keep track of players and client
    // context pointers
    ppHSPContexts   = new HSPClientContext*[MAX_PLAYERS];
    memset(ppHSPContexts, 0, sizeof(HSPClientContext *) * MAX_PLAYERS);
    ppPlayers      = new IHXPlayer*[MAX_PLAYERS];
    memset(ppPlayers, 0, sizeof(IHXPlayer *) * MAX_PLAYERS);
    ppAudioPlayer      = new IHXAudioPlayer*[MAX_PLAYERS];
    memset(ppAudioPlayer, 0, sizeof(IHXPlayer *) * MAX_PLAYERS);
    ppVolume       = new IHXVolume*[MAX_PLAYERS];
    memset(ppVolume, 0, sizeof(IHXVolume *) * MAX_PLAYERS);
    ppCrossFader       = new IHXAudioCrossFade*[MAX_PLAYERS];
    memset(ppCrossFader, 0, sizeof(IHXVolume *) * MAX_PLAYERS);
    ppszURL        = new char *[MAX_PLAYERS];
    memset(ppszURL, 0, sizeof(char *) * MAX_PLAYERS);

    if (!ppHSPContexts || !ppPlayers || !ppszURL)
    {
       if (numPlayers > MAX_PLAYERS)
       {
          STDOUT("Error: Out of Memory. Perhaps you are trying to launch too many players at once.\n");
       }
       else
       {
          STDOUT("Error: Out of Memory.\n");
       }
       theErr = HXR_UNEXPECTED;
       return;
    }

    fpCreateEngine    = NULL;

    // prepare/load the HXCore module
    STDOUT("Simpleplayer is looking for the client core at %s\n", mCoreLibPath );

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
       STDERR("Common DLL path %s\n", pNextPath );
       UINT32 ulBytesToCopy = strlen(pNextPath) + 1;
       if (ulBytesToCopy <= ulBytesLeft)
       {
          memcpy(pPathNextPosition, pNextPath, ulBytesToCopy);
          pPathNextPosition += ulBytesToCopy;
          ulBytesLeft -= ulBytesToCopy;
       }

       SafeSprintf(pNextPath, 256, "DT_Plugins=%s", pluginslibhome);
       STDERR("Plugin path %s\n", pNextPath );
       ulBytesToCopy = strlen(pNextPath) + 1;
       if (ulBytesToCopy <= ulBytesLeft)
       {
          memcpy(pPathNextPosition, pNextPath, ulBytesToCopy);
          pPathNextPosition += ulBytesToCopy;
          ulBytesLeft -= ulBytesToCopy;
       }
       
       SafeSprintf(pNextPath, 256, "DT_Codecs=%s", codecshome);
       STDERR("Codec path %s\n", pNextPath );
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
    XInitThreads();

    // create client engine
    IHXClientEngine* pEngine;
    if (HXR_OK != fpCreateEngine((IHXClientEngine**)&pEngine))
    {
       theErr = HXR_FAILED;
       return;
    }
    GetGlobal()->pEngine = pEngine;

    // get the client engine selector
    pCEselect = 0;
    pEngine->QueryInterface(IID_IHXClientEngineSelector, &pCEselect);
    if (pCEselect)
       STDERR("Got the CE selector!\n");
    else
       STDERR("no CE selector\n");
    
    // create players
    for (i = 0; i < numPlayers; i++)
    {
       addPlayer();
    }
}

int HelixSimplePlayer::addPlayer()
{
   if ((nNumPlayers+1) == MAX_PLAYERS)
   {
      STDERR("MAX_PLAYERS: %d   nNumPlayers: %d\n", MAX_PLAYERS, nNumPlayers);
      return -1;
   }

   ppHSPContexts[nNumPlayers] = new HSPClientContext(nNumPlayers, this);

   if (!ppHSPContexts[nNumPlayers])
   {
      if (nNumPlayers > MAX_PLAYERS)
      {
         STDOUT("Error: Out of Memory. Perhaps you are trying to launch too many players at once.\n");
      }
      else
      {
         STDOUT("Error: Out of Memory.\n");
      }
      theErr = HXR_UNEXPECTED;
      return -1;
   }
   
   ppHSPContexts[nNumPlayers]->AddRef();

   //initialize the example context
   pszGUIDList = GetGlobal()->g_pszGUIDList;
   char pszGUID[GUID_LEN + 1]; /* Flawfinder: ignore */ // add 1 for terminator
   char* token = NULL;
   IHXPreferences* pPreferences = NULL;
   IHXClientEngine* pEngine = GetGlobal()->pEngine;
   if (HXR_OK != pEngine->CreatePlayer(ppPlayers[nNumPlayers]))
   {
      theErr = HXR_FAILED;
      return -1;
   }
   
   pszGUID[0] = '\0';
   
   if (pszGUIDList)
   {
      // Get next GUID from the GUID list
      if (nNumPlayers == 0)
      {
         token = strtok(pszGUIDList, "\n\0");
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
   
   ppPlayers[nNumPlayers]->QueryInterface(IID_IHXPreferences,
                                          (void**) &pPreferences);

   ppHSPContexts[nNumPlayers]->Init(ppPlayers[nNumPlayers], pPreferences, pszGUID);
   
   ppPlayers[nNumPlayers]->SetClientContext(ppHSPContexts[nNumPlayers]);
   
   HX_RELEASE(pPreferences);
   
   if (!nNumPlayers) // the first player is the parent
   {
      ppPlayers[nNumPlayers]->QueryInterface(IID_IHXPlayerNavigator, (void **) &pPlayerNavigator);
      pPlayerNavigator->SetParentPlayer(ppPlayers[nNumPlayers]);
   }
   
   if (pPlayerNavigator && nNumPlayers)
   {
      STDERR("Got the navigator!\n");
      pPlayerNavigator->AddChildPlayer(ppPlayers[nNumPlayers]);
   }

   ppPlayers[nNumPlayers]->QueryInterface(IID_IHXErrorSinkControl,
                                          (void**) &pErrorSinkControl);
   if (pErrorSinkControl)
   {
      ppHSPContexts[nNumPlayers]->QueryInterface(IID_IHXErrorSink,
                                                (void**) &pErrorSink);
      
      if (pErrorSink)
      {
         pErrorSinkControl->AddErrorSink(pErrorSink, HXLOG_EMERG, HXLOG_INFO);
      }
      
      HX_RELEASE(pErrorSink);
   }
   
   HX_RELEASE(pErrorSinkControl);
   
   // Get the Audio Player
   ppPlayers[nNumPlayers]->QueryInterface(IID_IHXAudioPlayer,
                                          (void**) &ppAudioPlayer[nNumPlayers]);
   
   if (ppAudioPlayer[nNumPlayers])
   {
      // ...and now the volume interface
      ppVolume[nNumPlayers] = ppAudioPlayer[nNumPlayers]->GetAudioVolume();
      if (!ppVolume[nNumPlayers])
         STDERR("No Volume Interface - how can we play music!!\n");
      else
      {
         HelixSimplePlayerVolumeAdvice *pVA = new HelixSimplePlayerVolumeAdvice(this, nNumPlayers);
         ppVolume[nNumPlayers]->AddAdviseSink((IHXVolumeAdviseSink *)pVA);
      }

      // add the IHXAudioStreamInfoResponse it the AudioPlayer
      HelixSimplePlayerAudioStreamInfoResponse *pASIR = new HelixSimplePlayerAudioStreamInfoResponse(this, nNumPlayers);
      ppAudioPlayer[nNumPlayers]->SetStreamInfoResponse(pASIR);

      // ...and the CrossFader
      ppAudioPlayer[nNumPlayers]->QueryInterface(IID_IHXAudioCrossFade,
                                                 (void **) &ppCrossFader[nNumPlayers]);
      if (!ppCrossFader[nNumPlayers])
         STDERR("CrossFader not available\n");
      else
      {
         STDERR("Got the CrossFader device!\n");
         STDERR("AudioPlayer 0x%lx\n", ppAudioPlayer[nNumPlayers]);
         STDERR("CrossFader 0x%lx\n", ppCrossFader[nNumPlayers]);
      }
   }
   else
      STDERR("No AudioPlayer Found - how can we play music!!\n");
   
   ++nNumPlayers;

   STDERR("Added player, total is %d\n",nNumPlayers);
   return 0;
}

HelixSimplePlayer::~HelixSimplePlayer()
{
   int i;
   FPRMCLOSEENGINE         fpCloseEngine;
  
   if ( ppszURL )
   {
      for (i=0; i<nNumPlayers; i++)
         delete [] ppszURL[i];
   }

   if (ppVolume)
   {
      for (i=0; i<nNumPlayers; i++)
      {
         if (ppVolume[i])
         {
            ppVolume[i]->Release();
            ppVolume[i] = NULL;
         }
      }
      delete []ppVolume;
   }

   if (ppHSPContexts)
   {
      for (i = 0; i < nNumPlayers; i++)
      {
         if (ppHSPContexts[i])
         {
            ppHSPContexts[i]->Release();
            ppHSPContexts[i] = NULL;
         }
      }
      
      delete []ppHSPContexts;
      ppHSPContexts = NULL;
   }

   if (ppPlayers)
   {
      for (i = 0; i < nNumPlayers; i++)
      {
         if (ppPlayers[i])
         {
            if (GetGlobal()->pEngine)
            {
               GetGlobal()->pEngine->ClosePlayer(ppPlayers[i]);
            }
            ppPlayers[i]->Release();
            ppPlayers[i] = NULL;
         }
      }
      delete []ppPlayers;
      ppPlayers = NULL;
   }

   fpCloseEngine  = (FPRMCLOSEENGINE) dlsym(core_handle, "CloseEngine");
   if (fpCloseEngine && GetGlobal()->pEngine)
   {
      fpCloseEngine(GetGlobal()->pEngine);
      GetGlobal()->pEngine = NULL;
   }

   dlclose(core_handle);

   if (GetGlobal()->bEnableVerboseMode)
   {
      STDOUT("\nDone.\n");
   }
   
   if (GetGlobal()->g_pszUsername)
   {
      delete [] GetGlobal()->g_pszUsername;
   }
   if (GetGlobal()->g_pszPassword)
   {
      delete [] GetGlobal()->g_pszPassword;
   }
   if (GetGlobal()->g_pszGUIDFile)
   {
      delete [] GetGlobal()->g_pszGUIDFile;
   }
   if (GetGlobal()->g_pszGUIDList)
   {
      delete [] GetGlobal()->g_pszGUIDList;
   }
   
   // If an an error occurred in this function return it
   if (theErr != HXR_OK)
   {
      return;
   }
   // If an error occurred during playback, return that
   else if (GetGlobal()->g_Error != HXR_OK)
   {
      theErr = GetGlobal()->g_Error; 
      return;
   }
   // If all went well, return the number of seconds played (if there
   // was only one player)...
   else if (nNumPlayers == 1)
   {
      theErr = GetGlobal()->g_ulNumSecondsPlayed;
      return;
   }
   // or HXR_OK (if there was more than one player)
   else
   {
      theErr = HXR_OK;
      return;
   }
}

int HelixSimplePlayer::setURL(const char *file, int playerIndex)
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
      
      if (ppszURL[playerIndex])
         delete [] ppszURL[playerIndex];
      
      // see if the file is already in the form of a url
      char *tmp = strstr(file, "://");
      if (!tmp)
      {
         char pszURLOrig[MAXPATHLEN];
         char* pszAddOn;
         
         strcpy(pszURLOrig, file);
         RemoveWrappingQuotes(pszURLOrig);
         pszAddOn = "file://";
         
         ppszURL[playerIndex] = new char[strlen(pszURLOrig)+strlen(pszAddOn)+1];
         if ( (len + strlen(pszAddOn)) < MAXPATHLEN )
            sprintf( ppszURL[playerIndex], "%s%s", pszAddOn, pszURLOrig ); /* Flawfinder: ignore */
         else
            return -1;
      }
      else
      {
         ppszURL[playerIndex] = new char[len + 1];
         if (ppszURL[playerIndex])
            strcpy(ppszURL[playerIndex], file);
         else
            return -1;
      }
      
      STDERR("opening %s on player %d, src cnt %d\n", ppszURL[playerIndex], playerIndex, ppPlayers[playerIndex]->GetSourceCount());
      if (HXR_OK == ppPlayers[playerIndex]->OpenURL(ppszURL[playerIndex]))
      {
         STDERR("opened player on %d src cnt %d\n", playerIndex, ppPlayers[playerIndex]->GetSourceCount());
//         IHXStreamSource *pStreamSource = 0;
//         IHXStream *pStream = 0;
//         ppPlayers[playerIndex]->GetSource(0, (IUnknown *&)pStreamSource);
//         if (pStreamSource)
//         {
//            STDERR("Got StreamSource, stream count is %d!!\n", pStreamSource->GetStreamCount());
//            pStreamSource->GetStream(0, (IUnknown *&)pStream);
//            if (pStream)
//            {
//               STDERR("Got Stream!!\n");
//
//               HX_RELEASE(pStream);
//            }
//
//            HX_RELEASE(pStreamSource);
//         }
//         else
//            STDERR("guess again, no stream source\n");
         
      }
   }
   
   return 0;
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
void HelixSimplePlayer::crossFade(const char *url, unsigned long startPos, unsigned long xfduration)
{
   if (m_xf.crossfading)
   {
      m_xf.duration = xfduration;
      m_xf.fromStream = 0;
      m_xf.fromStream = ppAudioPlayer[m_xf.fromIndex]->GetAudioStream(0);
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
      int whereFrom = where(xf().fromIndex) + 1000; // 1 sec is just majic...otherwise the crossfader just doesnt work

      // only fade in the new stream if we are playing one already
      if (xf().fromStream)
      {
         STDERR("Player %d where %d  duration %d  startFrom %d\n", xf().fromIndex, where(xf().fromIndex), duration(xf().fromIndex), startFrom);
         
         // fade out the now-playing-stream
         (getCrossFader(xf().fromIndex))->CrossFade(xf().fromStream, 0, startFrom > whereFrom ? startFrom : whereFrom, 0, xf().duration);

         // fade in the new stream
         (getCrossFader(xf().toIndex))->CrossFade(0, xf().toStream, 0, 0, xf().duration);
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


void HelixSimplePlayer::play(const char *file, int playerIndex)
{
   if (!setURL(file, playerIndex))
      play(playerIndex);
}

void HelixSimplePlayer::play(int playerIndex)
{
   int i;
   int firstPlayer = playerIndex == ALL_PLAYERS ? 0 : playerIndex;
   int lastPlayer  = playerIndex == ALL_PLAYERS ? nNumPlayers : playerIndex + 1;
   
   nPlay = 0;
   nNumPlayRepeats=1;
   while(nPlay < nNumPlayRepeats) 
   {
      nPlay++;
      if (GetGlobal()->bEnableVerboseMode)
      {
         STDOUT("Starting play #%d...\n", nPlay);
      }
      STDERR("firstplayer = %d  lastplayer=%d\n",firstPlayer,lastPlayer);
      
      UINT32 starttime, endtime, now;
      for (i = firstPlayer; i < lastPlayer; i++)
      {
         start(i);

         starttime = GetTime();
         endtime = starttime + nTimeDelta;
         while (1)
         {
            DoEvents(nTimeDelta);
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
	    if (GetGlobal()->bEnableVerboseMode)
	    {
               STDOUT("\nEnd (Stop) time reached. Stopping...\n");
	    }
	    stop(playerIndex);
            bStopping = true;
         }
         DoEvent();
      }

      // Stop all of the players, as they should all be done now
      if (GetGlobal()->bEnableVerboseMode)
      {
         STDOUT("\nPlayback complete. Stopping all players...\n");
      }
      stop(playerIndex);

      // repeat until nNumRepeats
   }
}

void HelixSimplePlayer::start(int playerIndex)
{
   if (playerIndex == ALL_PLAYERS)
   {
      int i;
      for (i=0; i<nNumPlayers; i++)
         start(i);
   }
   else
   {
      if (!ppszURL[playerIndex])
         return;
      
      if (GetGlobal()->bEnableVerboseMode)
      {
         STDOUT("Starting player %d...\n", playerIndex);
      }
      ppPlayers[playerIndex]->Begin();
   }
}


void HelixSimplePlayer::start(const char *file, int playerIndex)
{
   setURL(file, playerIndex);
   start(playerIndex);
}



bool HelixSimplePlayer::done(int playerIndex)
{
   BOOL bAllDone = TRUE;
   
   if (playerIndex == ALL_PLAYERS)
      // Start checking at the end of the array since those players
      // were started last and are therefore more likely to not be
      // finished yet.
      for (int i = nNumPlayers - 1; i >= 0 && bAllDone; i--)
      {
         if (!ppPlayers[i]->IsDone())
         {
            bAllDone = FALSE;
         }
      }
   else
   {
      if (playerIndex < nNumPlayers)
         bAllDone = ppPlayers[playerIndex]->IsDone();
   }
   
   return bAllDone;
}

void HelixSimplePlayer::stop(int playerIndex)
{
   if (playerIndex == ALL_PLAYERS)
      for (int i = 0; i < nNumPlayers; i++)
      {
         ppPlayers[i]->Stop();
      }
   else
   {
      if (playerIndex < nNumPlayers)
         ppPlayers[playerIndex]->Stop();
   }
}

void HelixSimplePlayer::dispatch()
{
   struct _HXxEvent* pNothing = 0;
   struct timeval tv;
   
   tv.tv_sec = 0;
   tv.tv_usec = SLEEP_TIME*1000;
   pCEselect->Select(0, 0, 0, 0, &tv);
   HelixSimplePlayer::GetGlobal()->pEngine->EventOccurred(pNothing);
   usleep(1);
}


void HelixSimplePlayer::pause(int playerIndex)
{
   int i;
   
   if (playerIndex == ALL_PLAYERS)
      for (i=0; i<nNumPlayers; i++)
         pause(i);
   else
      if (playerIndex < nNumPlayers)
         ppPlayers[playerIndex]->Pause();
}

void HelixSimplePlayer::resume(int playerIndex)
{
   int i;
   
   if (playerIndex == ALL_PLAYERS)
      for (i=0; i<nNumPlayers; i++)
         resume(i);
   else
      if (playerIndex < nNumPlayers)
         ppPlayers[playerIndex]->Begin();
}


void HelixSimplePlayer::seek(unsigned long pos, int playerIndex)
{
   int i;
   
   if (playerIndex == ALL_PLAYERS)
      for (i=0; i<nNumPlayers; i++)
         seek(pos, i);
   else
      if (playerIndex < nNumPlayers)
         ppPlayers[playerIndex]->Seek(pos);
}

unsigned long HelixSimplePlayer::where(int playerIndex)
{
   return ppPlayers[playerIndex]->GetCurrentPlayTime();
}

unsigned long HelixSimplePlayer::duration(int playerIndex)
{
   return ppHSPContexts[playerIndex]->duration();
}

void HelixSimplePlayer::initVolume(unsigned short minV, unsigned short maxV, int playerIndex)
{
//   int i;
//
//   if (playerIndex == ALL_PLAYERS)
//   {
//      for (i=0; i<nNumPlayers; i++)
//         initVolume(minV, maxV, i);
//   }
//   else
//      if (playerIndex < nNumPlayers)
//         ppAudioDevice[playerIndex]->InitVolume(minV,maxV);
}

unsigned long HelixSimplePlayer::getVolume(int playerIndex)
{
   if (playerIndex < nNumPlayers)
      return (ppVolume[playerIndex]->GetVolume());
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
         ppVolume[playerIndex]->SetVolume(vol);
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
         ppVolume[playerIndex]->SetMute(mute);
}


bool HelixSimplePlayer::getMute(int playerIndex)
{
   if (playerIndex < nNumPlayers)
      return ppVolume[playerIndex]->GetMute();
   else
      return false;
}

bool HelixSimplePlayer::ReadGUIDFile()
{
   BOOL  bSuccess = FALSE;
   FILE* pFile    = NULL;
   int   nNumRead = 0;
   int   readSize = 10000;
   char*  pszBuffer = new char[readSize];

   if (GetGlobal()->g_pszGUIDFile)
   {
      if((pFile = fopen(GetGlobal()->g_pszGUIDFile, "r")) != NULL)
      {
         // Read in the entire file
         nNumRead = fread(pszBuffer, sizeof(char), readSize, pFile);
         pszBuffer[nNumRead] = '\0';
         
         // Store it for later parsing
         GetGlobal()->g_pszGUIDList = new char[nNumRead + 1];
         strcpy(GetGlobal()->g_pszGUIDList, pszBuffer); /* Flawfinder: ignore */
         
         fclose(pFile);
         pFile = NULL;

         if (nNumRead > 0)
         {
            bSuccess = TRUE;
         }
      }
   }

   delete [] pszBuffer;
   
   return bSuccess;
}

#ifdef TEST_APP
char* GetAppName(char* pszArgv0)
{
    char* pszAppName;

    pszAppName = strrchr(pszArgv0, '\\');

    if (NULL == pszAppName)
    {
        return pszArgv0;
    }
    else
    {
        return pszAppName + 1;
    }
}

void PrintUsage(const char* pszAppName)
{
    STDOUT("\n");

#if defined _DEBUG || defined DEBUG
    STDOUT("USAGE:\n%s [-as0] [-d D] [-n N] [-t T] [-st ST] [-g file] [-u username] [-p password] <URL>\n", pszAppName);
#else
    STDOUT("USAGE:\n%s [-as0] [-n N] [-t T] [-g file] [-u username] [-p password] <URL>\n", pszAppName);
#endif
    STDOUT("       -a : optional flag to show advise sink output\n");
    STDOUT("       -s : optional flag to output useful status messages\n");
    STDOUT("       -0 : optional flag to disable all output windows\n");
    STDOUT("       -l : optional flag to tell the player where to find its DLLs\n");

#if defined _DEBUG || defined DEBUG
    STDOUT("       -d : HEX flag to print out DEBUG info\n");
    STDOUT("            0x8000 -- for audio methods calling sequence\n"
            "0x0002 -- for variable values\n");
#endif
    STDOUT("       -n : optional flag to spawn N players\n");
    STDOUT("       -rn: optional flag to repeat playback N times\n");
    STDOUT("       -t : wait T milliseconds between starting players (default: %d)\n", DEFAULT_TIME_DELTA);
    STDOUT("       -st: wait ST milliseconds util stoping players (default: %d)\n", DEFAULT_STOP_TIME);
    STDOUT("       -g : use the list of GUIDS in the specified newline-delimited file\n");
    STDOUT("            to give each of the N players a different GUID\n");
    STDOUT("       -u : username to use in authentication response\n");
    STDOUT("       -p : password to use in authentication response\n");
    STDOUT("\n");
}

int main( int argc, char *argv[] )
{
   int   i;
   char  dllhome[MAX_PATH];
   int   nNumPlayers = 1;
   int   nNumPlayRepeats = 1;
   int   nTimeDelta = DEFAULT_TIME_DELTA;
   int   nStopTime = DEFAULT_STOP_TIME;
   bool  bURLFound = false;

    //See if the user has set their HELIX_LIBS env var. This is overridden by the
    //-l option.
    const char* pszHelixLibs = getenv("HELIX_LIBS");
    if( pszHelixLibs )
        SafeStrCpy( dllhome,  pszHelixLibs, MAX_PATH);

    int volscale = 100;

    for (i = 1; i < argc; i++)
    {
       if (0 == stricmp(argv[i], "-v"))
       {
          if (++i == argc)
          {
             STDOUT("\nError: Invalid value for -n option.\n\n");
             PrintUsage(GetAppName(argv[0]));
             return -1;
          }
          volscale = atoi(argv[i]);
       }
       else if (0 == stricmp(argv[i], "-n"))
       {
          if (++i == argc)
          {
             STDOUT("\nError: Invalid value for -n option.\n\n");
             PrintUsage(GetAppName(argv[0]));
             return -1;
          }
          nNumPlayers = atoi(argv[i]);
          if (nNumPlayers < 1)
          {
             STDOUT("\nError: Invalid value for -n option.\n\n");
             PrintUsage(GetAppName(argv[0]));
             return -1;
          }
       }
       else if (0 == stricmp(argv[i], "-rn"))
       {
          if (++i == argc)
          {
             STDOUT("\nError: Invalid value for -rn option.\n\n");
             PrintUsage(GetAppName(argv[0]));
             return -1;
          }
          nNumPlayRepeats = atoi(argv[i]);
          if (nNumPlayRepeats < 1)
          {
             STDOUT("\nError: Invalid value for -rn option.\n\n");
             PrintUsage(GetAppName(argv[0]));
             return -1;
          }
       }
       else if (0 == stricmp(argv[i], "-t"))
       {
          if (++i == argc)
          {
             STDOUT("\nError: Invalid value for -t option.\n\n");
             PrintUsage(GetAppName(argv[0]));
             return -1;
          }
          nTimeDelta = atoi(argv[i]);
          if (nTimeDelta < 0)
          {
             STDOUT("\nError: Invalid value for -t option.\n\n");
             PrintUsage(GetAppName(argv[0]));
             return -1;
          }
       }
       else if (0 == stricmp(argv[i], "-st"))
       {
          if (++i == argc)
          {
             STDOUT("\nError: Invalid value for -st option.\n\n");
             PrintUsage(GetAppName(argv[0]));
             return -1;
          }
          nStopTime = atoi(argv[i]);
          if (nStopTime < 0)
          {
             STDOUT("\nError: Invalid value for -st option.\n\n");
             PrintUsage(GetAppName(argv[0]));
             return -1;
          }
       }
#if defined _DEBUG || defined DEBUG
       else if (0 == stricmp(argv[i], "-d"))
       {
          if (++i == argc)
          {
             STDOUT("\nError: Invalid value for -d option.\n\n");
             PrintUsage(GetAppName(argv[0]));
             return -1;
          }
          debug_level() = (int)strtoul(argv[i], 0, 0);
       }
#endif
       else if (0 == stricmp(argv[i], "-u"))
       {
          char *puser = new char[1024];
          strcpy(puser, "");
          if (++i == argc)
          {
             STDOUT("\nError: Invalid value for -u option.\n\n");
             PrintUsage(GetAppName(argv[0]));
             return -1;
          }
          SafeStrCpy(puser,  argv[i], 1024);
          //HelixSimplePlayer::setUsername(puser);
       }
       else if (0 == stricmp(argv[i], "-p"))
       {
          char *ppass = new char[1024];
          strcpy(ppass, ""); /* Flawfinder: ignore */
          if (++i == argc)
          {
             STDOUT("\nError: Invalid value for -p option.\n\n");
             PrintUsage(GetAppName(argv[0]));
             return -1;
          }
          SafeStrCpy(ppass,  argv[i], 1024);
          //HelixSimplePlayer::setPassword(ppass);
       }
       else if (0 == stricmp(argv[i], "-g"))
       {
          char *pfile = new char[1024];
          strcpy(pfile, ""); /* Flawfinder: ignore */
          if (++i == argc)
          {
             STDOUT("\nError: Invalid value for -g option.\n\n");
             PrintUsage(GetAppName(argv[0]));
             return -1;
          }
          SafeStrCpy(pfile, HelixSimplePlayer::RemoveWrappingQuotes(argv[i]), 1024);
          //HelixSimplePlayer::setGUIDFile(pfile);
          if (!HelixSimplePlayer::ReadGUIDFile())
          {
             STDOUT("\nError: Unable to read file specified by -g option.\n\n");
             return -1;
          }
       }
       else if (0 == stricmp(argv[i], "-l"))
       {
          if (++i == argc)
          {
             STDOUT("\nError: Invalid value for -l option.\n\n");
             PrintUsage(GetAppName(argv[0]));
             return -1;
          }
          SafeStrCpy(dllhome, HelixSimplePlayer::RemoveWrappingQuotes(argv[i]), MAX_PATH);
       }
       else if (!bURLFound)
       {
          bURLFound  = TRUE;
          //if no "://" was found lets add file:// by default so that you
          //can refer to local content as just ./splay ~/Content/startrek.rm,
          //for example, and not ./splay file:///home/gregory/Content/startrek.rm
       }
       else
       {
//            PrintUsage(GetAppName(argv[0]));
//            return -1;
       }
    }
    
    if (!bURLFound)
    {
       if (argc > 1)
       {
          STDOUT("\nError: No media file or URL was specified.\n\n");
       }
       PrintUsage(GetAppName(argv[0]));
       return -1;
    }
    // start first player
    
    HelixSimplePlayer splay;

    splay.init("/usr/local/RealPlayer/common","/usr/local/RealPlayer/plugins","/usr/local/RealPlayer/codecs", 2);
    
    splay.enableCrossFader(0, 1);
    splay.setURL(argv[i-2],0);
    bool xfstart = 0, didseek = 0, xfsetup = 0;
    unsigned long d = 15000;
    unsigned long xfpos;
    unsigned long counter = 0;
    int nowplayingon = 0;

    splay.dispatch();
    STDERR("Before sleep\n");
    sleep(2);

    splay.start(splay.xf().fromIndex);
    while (!splay.done())
    {
       splay.dispatch();
       
       if (!(counter % 1000))
          STDERR("time: %ld/%ld %ld/%ld count %d\n", splay.where(0), splay.duration(0), splay.where(1), splay.duration(1), counter);
       
       counter++;
       
       if (splay.duration(splay.xf().fromIndex))
          xfpos = splay.duration(0) - d;

       if (!xfsetup && splay.where(splay.xf().fromIndex) > xfpos - 2000)
       {
          xfsetup = 1;
          splay.crossFade(argv[i-1], splay.where(splay.xf().fromIndex), d);
       }

       if (!xfstart && splay.where(splay.xf().fromIndex) > xfpos - 1000)
       {
          xfstart = 1;
          splay.startCrossFade();
       }
    }


//    if (splay.getError() == HXR_OK)
//    {
//       int vol = splay.getVolume(0);
//       bool mute;
//
//       splay.setMute(false);
//       mute = splay.getMute(0);
//       if (mute)
//          STDERR("Mute is ON\n");
//       else
//          STDERR("Mute is OFF\n");
//
//       splay.setVolume(volscale);
//       vol = splay.getVolume(0);
//       splay.play(argv[i-1]);
//    }
}
#endif // TEST_APP

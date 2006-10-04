
/*
 *
 * This software is released under the provisions of the GPL version 2.
 * see file "COPYING".  If that file is not available, the full statement 
 * of the license can be found at
 *
 * http://www.fsf.org/licensing/licenses/gpl.txt
 *
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved.
 * Portions (c) Paul Cifarelli 2005
 */

#include <stdio.h>

#include "hxcomm.h"
#include "hxmon.h"
#include "hxcore.h"
#include "hxengin.h"
#include "hxclsnk.h"
#include "ihxpckts.h"

#include "hxausvc.h"
#include "helix-sp.h"
#include "hspadvisesink.h"
#include "utils.h"

#if	defined(__cplusplus)
extern	"C"	{
#endif	/* defined(__cplusplus) */

#if	defined(__cplusplus)
}
#endif	/* defined(__cplusplus) */

HSPClientAdviceSink::HSPClientAdviceSink(IUnknown* pUnknown, LONG32 lClientIndex, HelixSimplePlayer *pSplay)
    : m_splayer(pSplay)
    , m_lRefCount (0)
    , m_lClientIndex (lClientIndex)
    , m_pUnknown (NULL)
    , m_pRegistry (NULL)
    , m_pScheduler (NULL)
    , m_position(0)
    , m_duration(0)
    , m_lCurrentBandwidth(0)
    , m_lAverageBandwidth(0)
    , m_bOnStop(0)
{
   //m_splayer->bEnableAdviceSink = true;

   if (pUnknown)
   {
      m_pUnknown = pUnknown;
      m_pUnknown->AddRef();
      
      if (HXR_OK != m_pUnknown->QueryInterface(IID_IHXRegistry, (void**)&m_pRegistry))
      {
         m_pRegistry = NULL;
      }
      
      if (HXR_OK != m_pUnknown->QueryInterface(IID_IHXScheduler, (void**)&m_pScheduler))
      {
         m_pScheduler = NULL;
      }
      
      IHXPlayer* pPlayer;
      if(HXR_OK == m_pUnknown->QueryInterface(IID_IHXPlayer,
                                              (void**)&pPlayer))
      {
         pPlayer->AddAdviseSink(this);
         pPlayer->Release();
      }
   }
}

HSPClientAdviceSink::~HSPClientAdviceSink(void)
{
    if (m_pScheduler)
    {
        m_pScheduler->Release();
        m_pScheduler = NULL;
    }

    if (m_pRegistry)
    {
	m_pRegistry->Release();
	m_pRegistry = NULL;
    }

    if (m_pUnknown)
    {
	m_pUnknown->Release();
	m_pUnknown = NULL;
    }
}


// *** IUnknown methods ***

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::QueryInterface
//  Purpose:
//	Implement this to export the interfaces supported by your 
//	object.
//
STDMETHODIMP HSPClientAdviceSink::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = (IUnknown*)(IHXClientAdviseSink*)this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXClientAdviseSink))
    {
	AddRef();
	*ppvObj = (IHXClientAdviseSink*)this;
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
STDMETHODIMP_(ULONG32) HSPClientAdviceSink::AddRef()
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
STDMETHODIMP_(ULONG32) HSPClientAdviceSink::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

/*
 *	IHXClientAdviseSink methods
 */

/************************************************************************
 *	Method:
 *	    IHXClientAdviseSink::OnPosLength
 *	Purpose:
 *	    Called to advise the client that the position or length of the
 *	    current playback context has changed.
 */
STDMETHODIMP
HSPClientAdviceSink::OnPosLength(UINT32	  ulPosition,
				   UINT32	  ulLength)
{
   if (m_splayer->bEnableAdviceSink)
   {
      m_splayer->print2stdout("OnPosLength(%ld, %ld)\n", ulPosition, ulLength);
   }
   m_position = ulPosition;
   m_duration = ulLength;

   return HXR_OK;
}

/************************************************************************
 *	Method:
 *	    IHXClientAdviseSink::OnPresentationOpened
 *	Purpose:
 *	    Called to advise the client a presentation has been opened.
 */
STDMETHODIMP HSPClientAdviceSink::OnPresentationOpened()
{
/*
    if (m_splayer && m_splayer->xf().crossfading && m_lClientIndex == m_splayer->xf().toIndex)
    {
       m_splayer->print2stderr("Crossfading...\n");
       m_splayer->xf().toStream = 0;
       m_splayer->xf().toStream = m_splayer->getAudioPlayer(m_lClientIndex)->GetAudioStream(0);
       if (m_splayer->xf().toStream)
       {
          m_splayer->print2stderr("Got Stream 2\n");
          m_splayer->startCrossFade();
       }
       else
          m_splayer->stop(m_lClientIndex);
    }
*/
   //m_splayer->bEnableAdviceSink = true;

   if (m_splayer->bEnableAdviceSink)
   {
      m_splayer->print2stdout("OnPresentationOpened()\n");
   }

   return HXR_OK;
}


/************************************************************************
 *	Method:
 *	    IHXClientAdviseSink::OnPresentationClosed
 *	Purpose:
 *	    Called to advise the client a presentation has been closed.
 */
STDMETHODIMP HSPClientAdviceSink::OnPresentationClosed()
{
    if (m_splayer->bEnableAdviceSink)
    {
        m_splayer->print2stdout("OnPresentationClosed()\n");
    }

    return HXR_OK;
}

void HSPClientAdviceSink::GetStatistics (char* pszRegistryKey)
{
    char    szRegistryValue[MAX_DISPLAY_NAME] = {0}; /* Flawfinder: ignore */
    INT32   lValue = 0;
    INT32   i = 0;
    INT32   lStatistics = 8;
    UINT32 *plValue;
    
    // collect statistic
    for (i = 0; i < lStatistics; i++)
    {
	plValue = NULL;
	switch (i)
	{
	case 0:
	    SafeSprintf(szRegistryValue, MAX_DISPLAY_NAME, "%s.Normal", pszRegistryKey);
	    break;
	case 1:
	    SafeSprintf(szRegistryValue, MAX_DISPLAY_NAME, "%s.Recovered", pszRegistryKey);
	    break;
	case 2:
	    SafeSprintf(szRegistryValue, MAX_DISPLAY_NAME, "%s.Received", pszRegistryKey);
	    break;
	case 3:
	    SafeSprintf(szRegistryValue, MAX_DISPLAY_NAME, "%s.Lost", pszRegistryKey);
	    break;
	case 4:
	    SafeSprintf(szRegistryValue, MAX_DISPLAY_NAME, "%s.Late", pszRegistryKey);
	    break;
	case 5:
	    SafeSprintf(szRegistryValue, MAX_DISPLAY_NAME, "%s.ClipBandwidth", pszRegistryKey);
	    break;
	case 6:
	    SafeSprintf(szRegistryValue, MAX_DISPLAY_NAME, "%s.AverageBandwidth", pszRegistryKey);
	    plValue = &m_lAverageBandwidth;
	    break;
	case 7:
	    SafeSprintf(szRegistryValue, MAX_DISPLAY_NAME, "%s.CurrentBandwidth", pszRegistryKey);
	    plValue = &m_lCurrentBandwidth;
	    break;
	default:
	    break;
	}

	m_pRegistry->GetIntByName(szRegistryValue, lValue);
	if (plValue)
	{
	    if (m_bOnStop || lValue == 0)
	    {
		lValue = *plValue;
	    }
	    else
	    {
		*plValue = lValue;
	    }
	}
	if (m_splayer->bEnableAdviceSink || (m_splayer->bEnableVerboseMode && m_bOnStop))
	{
	    m_splayer->print2stdout("%s = %ld\n", szRegistryValue, lValue);
	}
    }
}

void HSPClientAdviceSink::GetAllStatistics(void)
{
    UINT32  unPlayerIndex = 0;
    UINT32  unSourceIndex = 0;
    UINT32  unStreamIndex = 0;

    const char*   pszRegistryPrefix = "Statistics";
    char    szRegistryName[MAX_DISPLAY_NAME] = {0}; /* Flawfinder: ignore */

    // display the content of whole statistic registry
    if (m_pRegistry)
    {
	// ok, let's start from the top (player)
	SafeSprintf(szRegistryName, MAX_DISPLAY_NAME, "%s.Player%ld", pszRegistryPrefix, m_lClientIndex);
	if (PT_COMPOSITE == m_pRegistry->GetTypeByName(szRegistryName))
	{
	    // display player statistic
	    GetStatistics(szRegistryName);

	    SafeSprintf(szRegistryName, MAX_DISPLAY_NAME, "%s.Source%ld", szRegistryName, unSourceIndex);
	    while (PT_COMPOSITE == m_pRegistry->GetTypeByName(szRegistryName))
	    {
		// display source statistic
		GetStatistics(szRegistryName);

		SafeSprintf(szRegistryName, MAX_DISPLAY_NAME, "%s.Stream%ld", szRegistryName, unStreamIndex);
		while (PT_COMPOSITE == m_pRegistry->GetTypeByName(szRegistryName))
		{
		    // display stream statistic
		    GetStatistics(szRegistryName);

		    unStreamIndex++;

		    SafeSprintf(szRegistryName, MAX_DISPLAY_NAME, "%s.Player%ld.Source%ld.Stream%ld", 
			pszRegistryPrefix, unPlayerIndex, unSourceIndex, unStreamIndex);
		}

		unSourceIndex++;

		SafeSprintf(szRegistryName, MAX_DISPLAY_NAME, "%s.Player%ld.Source%ld",
		    pszRegistryPrefix, unPlayerIndex, unSourceIndex);
	    }

	    unPlayerIndex++;

	    SafeSprintf(szRegistryName, MAX_DISPLAY_NAME, "%s.Player%ld", pszRegistryPrefix, unPlayerIndex);
	}
    }
}

/************************************************************************
 *	Method:
 *	    IHXClientAdviseSink::OnStatisticsChanged
 *	Purpose:
 *	    Called to advise the client that the presentation statistics
 *	    have changed. 
 */
STDMETHODIMP HSPClientAdviceSink::OnStatisticsChanged(void)
{
    char        szBuff[1024]; /* Flawfinder: ignore */
    HX_RESULT   res     = HXR_OK;
    UINT16      uPlayer = 0;

    //if(m_splayer->bEnableAdviceSink)
    {
       if(m_splayer->bEnableAdviceSink)
          m_splayer->print2stdout("OnStatisticsChanged():\n");
        
        SafeSprintf(szBuff, 1024, "Statistics.Player%u", uPlayer );
        while( HXR_OK == res )
        {
            res = DumpRegTree( szBuff, uPlayer );
            if ( HXR_OK == res )
            {
               //m_splayer->print2stderr("%d Title: %s\n", uPlayer, m_splayer->ppctrl[uPlayer]->md.title);
               //m_splayer->print2stderr("%d Author: %s\n", uPlayer, m_splayer->ppctrl[uPlayer]->md.artist);
               //m_splayer->print2stderr("%d Bitrate: %ld\n", uPlayer, m_splayer->ppctrl[uPlayer]->md.bitrate);
            }
            uPlayer++;
            SafeSprintf(szBuff, 1024, "Statistics.Player%u", uPlayer );
        }
    }

    return HXR_OK;
}

HX_RESULT HSPClientAdviceSink::DumpRegTree(const char* pszTreeName, UINT16 index )
{
    const char* pszName = NULL;
    ULONG32     ulRegID   = 0;
    HX_RESULT   res     = HXR_OK;
    INT32       nVal    = 0;
    IHXBuffer* pBuff   = NULL;
    IHXValues* pValues = NULL;

    int len;
    bool title = false;
    bool bw = false;
    bool author = false;

    //See if the name exists in the reg tree.
    res = m_pRegistry->GetPropListByName( pszTreeName, pValues);
    if( HXR_OK!=res || !pValues )
        return HXR_FAIL;

    //make sure this is a PT_COMPOSITE type reg entry.
    if( PT_COMPOSITE != m_pRegistry->GetTypeByName(pszTreeName))
        return HXR_FAIL;

    //Print out the value of each member of this tree.
    res = pValues->GetFirstPropertyULONG32( pszName, ulRegID );
    while( HXR_OK == res )
    {
       title = false;
       bw = false;
       author = false;

       len = strlen(pszName);
       len -= strlen("Title");
       if (len > 0 && !strcmp(&pszName[len], "Title"))
          title = true;
       len += strlen("Title");
       len -= strlen("Author");
       if (len > 0 && !strcmp(&pszName[len], "Author"))
          author = true;
       len += strlen("Author");
       len -= strlen("AverageBandwidth");
       if (len > 0 && !strcmp(&pszName[len], "AverageBandwidth"))
           bw = true;

        //We have at least one entry. See what type it is.
        HXPropType pt = m_pRegistry->GetTypeById(ulRegID);
        switch(pt)
        {
           case PT_COMPOSITE:
               DumpRegTree(pszName, index);
               break;
           case PT_INTEGER :
               nVal = 0;
               m_pRegistry->GetIntById( ulRegID, nVal );
               if(m_splayer->bEnableAdviceSink)
                  m_splayer->print2stdout("%s : %ld\n", pszName, nVal ); 
               if (bw)
                  m_splayer->ppctrl[index]->md.bitrate = nVal;
               break;
           case PT_INTREF :
               nVal = 0;
               m_pRegistry->GetIntById( ulRegID, nVal );
               if(m_splayer->bEnableAdviceSink)
                  m_splayer->print2stdout("%s : %ld\n", pszName, nVal ); 
               if (bw)
                  m_splayer->ppctrl[index]->md.bitrate = nVal;
               break;
           case PT_STRING :
               pBuff = NULL;
               m_pRegistry->GetStrById( ulRegID, pBuff );
               if(m_splayer->bEnableAdviceSink)
                  m_splayer->print2stdout("%s : \"", pszName ); 
               if( pBuff )
                  if(m_splayer->bEnableAdviceSink)
                     m_splayer->print2stdout("%s", (const char *)(pBuff->GetBuffer()) );
               if(m_splayer->bEnableAdviceSink)
                  m_splayer->print2stdout("\"\n" ); 

               if (title && pBuff)
               {
                  strncpy(m_splayer->ppctrl[index]->md.title, (const char *) (pBuff->GetBuffer()), 512);
                  m_splayer->ppctrl[index]->md.title[511] = '\0';
               }
               if (author && pBuff)
               {
                  strncpy(m_splayer->ppctrl[index]->md.artist, (const char *) (pBuff->GetBuffer()), 512);
                  m_splayer->ppctrl[index]->md.artist[511] = '\0';
               }
               HX_RELEASE(pBuff);
               break;
           case PT_BUFFER :
               if(m_splayer->bEnableAdviceSink)
                  m_splayer->print2stdout("%s %ld : BUFFER TYPE NOT SHOWN\n",
                        pszName, nVal ); 
               break;
           case PT_UNKNOWN:
               if(m_splayer->bEnableAdviceSink)
                  m_splayer->print2stdout("%s Unkown registry type entry\n", pszName );
               break;
           default:
               if(m_splayer->bEnableAdviceSink)
                  m_splayer->print2stdout("%s Unkown registry type entry\n", pszName );
               break;
        }
        res = pValues->GetNextPropertyULONG32( pszName, ulRegID);
    }

    HX_RELEASE( pValues );
    
    return HXR_OK;
}


/************************************************************************
 *	Method:
 *	    IHXClientAdviseSink::OnPreSeek
 *	Purpose:
 *	    Called by client engine to inform the client that a seek is
 *	    about to occur. The render is informed the last time for the 
 *	    stream's time line before the seek, as well as the first new
 *	    time for the stream's time line after the seek will be completed.
 *
 */
STDMETHODIMP HSPClientAdviceSink::OnPreSeek(	ULONG32	ulOldTime,
						ULONG32	ulNewTime)
{
#if !defined(__TCS__)
    if (m_splayer->bEnableAdviceSink)
    {
        m_splayer->print2stdout("OnPreSeek(%ld, %ld)\n", ulOldTime, ulNewTime);
    }
#endif

    return HXR_OK;
}


/************************************************************************
 *	Method:
 *	    IHXClientAdviseSink::OnPostSeek
 *	Purpose:
 *	    Called by client engine to inform the client that a seek has
 *	    just occurred. The render is informed the last time for the 
 *	    stream's time line before the seek, as well as the first new
 *	    time for the stream's time line after the seek.
 *
 */
STDMETHODIMP HSPClientAdviceSink::OnPostSeek(	ULONG32	ulOldTime,
						ULONG32	ulNewTime)
{
    if (m_splayer->bEnableAdviceSink)
    {
        m_splayer->print2stdout("OnPostSeek(%ld, %ld)\n", ulOldTime, ulNewTime);
    }

    return HXR_OK;
}


/************************************************************************
 *	Method:
 *	    IHXClientAdviseSink::OnStop
 *	Purpose:
 *	    Called by client engine to inform the client that a stop has
 *	    just occurred. 
 *
 */
STDMETHODIMP HSPClientAdviceSink::OnStop(void)
{
    HXTimeval now;

    if (m_splayer->bEnableAdviceSink)
    {
        m_splayer->print2stdout("OnStop()\n");
    }

    if (m_splayer->bEnableVerboseMode)
    {
        m_splayer->print2stdout("Player %ld stopped.\n", m_lClientIndex);
        m_bOnStop = true;
	GetAllStatistics();
    }

    // Find out the current time and subtract the beginning time to
    // figure out how many seconds we played
    now = m_pScheduler->GetCurrentSchedulerTime();
    m_ulStopTime = now.tv_sec;

    m_splayer->m_ulNumSecondsPlayed = m_ulStopTime - m_ulStartTime;

    m_position = m_duration = 0;
    return HXR_OK;
}

/************************************************************************
 *	Method:
 *	    IHXClientAdviseSink::OnPause
 *	Purpose:
 *	    Called by client engine to inform the client that a pause has
 *	    just occurred. The render is informed the last time for the 
 *	    stream's time line before the pause.
 *
 */
STDMETHODIMP HSPClientAdviceSink::OnPause(ULONG32 ulTime)
{
    if (m_splayer->bEnableAdviceSink)
    {
        m_splayer->print2stdout("OnPause(%ld)\n", ulTime);
    }

    return HXR_OK;
}


/************************************************************************
 *	Method:
 *	    IHXClientAdviseSink::OnBegin
 *	Purpose:
 *	    Called by client engine to inform the client that a begin or
 *	    resume has just occurred. The render is informed the first time 
 *	    for the stream's time line after the resume.
 *
 */
STDMETHODIMP HSPClientAdviceSink::OnBegin(ULONG32 ulTime)
{
    HXTimeval now;

#if !defined(__TCS__)
    if (m_splayer->bEnableAdviceSink)
    {
        m_splayer->print2stdout("OnBegin(%ld)\n", ulTime);
    }

    if (m_splayer->bEnableVerboseMode)
    {
        m_splayer->print2stdout("Player %ld beginning playback...\n", m_lClientIndex);
    }
#endif

    // Record the current time, so we can figure out many seconds we played
    now = m_pScheduler->GetCurrentSchedulerTime();
    m_ulStartTime = now.tv_sec;

    return HXR_OK;
}


/************************************************************************
 *	Method:
 *	    IHXClientAdviseSink::OnBuffering
 *	Purpose:
 *	    Called by client engine to inform the client that buffering
 *	    of data is occurring. The render is informed of the reason for
 *	    the buffering (start-up of stream, seek has occurred, network
 *	    congestion, etc.), as well as percentage complete of the 
 *	    buffering process.
 *
 */
STDMETHODIMP HSPClientAdviceSink::OnBuffering(ULONG32	ulFlags,
						UINT16	unPercentComplete)
{
    if (m_splayer->bEnableAdviceSink)
    {
        m_splayer->print2stdout("OnBuffering(%ld, %d)\n", ulFlags, unPercentComplete);
    }
    m_splayer->onBuffering(unPercentComplete);

    return HXR_OK;
}


/************************************************************************
 *	Method:
 *	    IHXClientAdviseSink::OnContacting
 *	Purpose:
 *	    Called by client engine to inform the client is contacting
 *	    hosts(s).
 *
 */
STDMETHODIMP HSPClientAdviceSink::OnContacting(const char* pHostName)
{
    if (m_splayer->bEnableAdviceSink)
    {
        m_splayer->print2stdout("OnContacting(\"%s\")\n", pHostName);
    }
    m_splayer->onContacting(pHostName);
    return HXR_OK;
}


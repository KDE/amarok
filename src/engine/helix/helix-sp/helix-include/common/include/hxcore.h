
/*
 *
 * This software is released under the provisions of the GPL version 2.
 * see file "COPYING".  If that file is not available, the full statement 
 * of the license can be found at
 *
 * http://www.fsf.org/licensing/licenses/gpl.txt
 *
 * Portions Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved.
 *
 */

#ifndef _HXCORE_H_
#define _HXCORE_H_

/*
 * Forward declarations of some interfaces defined or used here-in.
 */
typedef _INTERFACE	IUnknown			IUnknown;
typedef _INTERFACE	IHXStream			IHXStream;
typedef _INTERFACE	IHXStream2			IHXStream2;
typedef _INTERFACE	IHXStreamSource		IHXStreamSource;
typedef _INTERFACE	IHXPlayer			IHXPlayer;
typedef _INTERFACE	IHXClientEngine		IHXClientEngine;
typedef _INTERFACE	IHXScheduler			IHXScheduler;
typedef _INTERFACE	IHXClientAdviseSink		IHXClientAdviseSink;
typedef _INTERFACE	IHXValues      		IHXValues;
typedef _INTERFACE	IHXBuffer      		IHXBuffer;
typedef _INTERFACE	IHXPacket			IHXPacket;
// $Private:
typedef _INTERFACE	IHXPersistenceManager		IHXPersistenceManager;
typedef _INTERFACE	IHXRendererAdviseSink		IHXRendererAdviseSink;
typedef _INTERFACE	IHXLayoutSite			IHXLayoutSite;
typedef _INTERFACE	IHXProtocolValidator		IHXProtocolValidator;
typedef _INTERFACE	IHXUpdateProperties		IHXUpdateProperties;
typedef _INTERFACE	IHXUpdateProperties2		IHXUpdateProperties2;
typedef _INTERFACE	IHXPersistentComponentManager	IHXPersistentComponentManager;
typedef _INTERFACE	IHXPersistentComponent		IHXPersistentComponent;
typedef _INTERFACE	IHXPersistentRenderer		IHXPersistentRenderer;
typedef _INTERFACE	IHXCoreMutex			IHXCoreMutex;
typedef _INTERFACE	IHXMacBlitMutex		IHXMacBlitMutex;
// $EndPrivate.
typedef _INTERFACE	IHXRenderer			IHXRenderer;
typedef _INTERFACE	IHXPlayer2			IHXPlayer2;
typedef _INTERFACE	IHXRequest			IHXrequest;
typedef _INTERFACE	IHXPlayerNavigator		IHXPlayerNavigator;
typedef _INTERFACE	IHXGroupSink			IHXGroupSink;

typedef struct _PersistentComponentInfo	PersistentComponentInfo;
typedef struct _HXxEvent HXxEvent;


#ifdef _MAC_CFM
#pragma export on
#endif

#if defined _UNIX && !(defined _VXWORKS)
/* Includes needed for select() stuff */
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#ifdef _BEOS	// fd_set stuff
#include <net/socket.h>
#endif

#include "hxwin.h"

/* Used in renderer and advise sink interface */
enum BUFFERING_REASON
{
    BUFFERING_START_UP	= 0,
    BUFFERING_SEEK,
    BUFFERING_CONGESTION,
    BUFFERING_LIVE_PAUSE
};

/****************************************************************************
 * 
 *  Function:
 * 
 *	CreateEngine()
 * 
 *  Purpose:
 * 
 *	Function implemented by the RMA core to return a pointer to the
 *	client engine.  This function would be run by top level clients.
 */
STDAPI CreateEngine
		(
		    IHXClientEngine**  /*OUT*/ ppEngine
		);

/****************************************************************************
 * 
 *  Function:
 * 
 *	CloseEngine()
 * 
 *  Purpose:
 * 
 *	Function implemented by the RMA core to close the engine which
 *	was returned in CreateEngine().
 */
STDAPI CloseEngine
		(
		    IHXClientEngine*  /*IN*/ pEngine
		);

#ifdef _MAC_CFM
#pragma export off
#endif

/*
 * Definitions of Function Pointers to CreateEngine() and Close Engine().
 * These types are provided as a convenince to authors of top level clients.
 */
typedef HX_RESULT (HXEXPORT_PTR FPRMCREATEENGINE)(IHXClientEngine** ppEngine);
typedef HX_RESULT (HXEXPORT_PTR FPRMCLOSEENGINE) (IHXClientEngine*  pEngine);


/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXStream
 *
 *  Purpose:
 *
 *	Interface provided by the client engine to the renderers. This
 *	interface allows access to stream related information and properties.
 *
 *  IID_IHXStream:
 *
 *	{00000400-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXStream, 0x00000400, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXStream

DECLARE_INTERFACE_(IHXStream, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     * IHXStream methods
     */

    /************************************************************************
     *	Method:
     *	    IHXStream::GetSource
     *	Purpose:
     *	    Get the interface to the source object of which the stream is
     *	    a part of.
     *
     */
    STDMETHOD(GetSource)		(THIS_
					REF(IHXStreamSource*)	pSource) PURE;

    /************************************************************************
     *	Method:
     *	    IHXStream::GetStreamNumber
     *	Purpose:
     *	    Get the stream number for this stream relative to the source 
     *	    object of which the stream is a part of.
     *
     */
    STDMETHOD_(UINT16,GetStreamNumber)	    (THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXStream::GetStreamType
     *	Purpose:
     *	    Get the MIME type for this stream. NOTE: The returned string is
     *	    assumed to be valid for the life of the IHXStream from which it
     *	    was returned.
     *
     */
    STDMETHOD_(const char*,GetStreamType)   (THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXStream::GetHeader
     *	Purpose:
     *      Get the header for this stream.
     *
     */
    STDMETHOD_(IHXValues*,GetHeader)   (THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXStream::ReportQualityOfService
     *	Purpose:
     *	    Call this method to report to the playback context that the 
     *	    quality of service for this stream has changed. The unQuality
     *	    should be on a scale of 0 to 100, where 100 is the best possible
     *	    quality for this stream. Although the transport engine can 
     *	    determine lost packets and report these through the user
     *	    interface, only the renderer of this stream can determine the 
     *	    "real" perceived damage associated with this loss.
     *
     *	    NOTE: The playback context may use this value to indicate loss
     *	    in quality to the user interface. When the effects of a lost
     *	    packet are eliminated the renderer should call this method with
     *	    a unQuality of 100.
     *
     */
    STDMETHOD(ReportQualityOfService)	    (THIS_
					    UINT8   unQuality) PURE;

    /************************************************************************
     *	Method:
     *	    IHXStream::ReportRebufferStatus
     *	Purpose:
     *	    Call this method to report to the playback context that the
     *	    available data has dropped to a critically low level, and that
     *	    rebuffering should occur. The renderer should call back into this
     *	    interface as it receives additional data packets to indicate the
     *	    status of its rebuffering effort.
     *
     *	    NOTE: The values of unNeeded and unAvailable are used to indicate
     *	    the general status of the rebuffering effort. For example, if a
     *	    renderer has "run dry" and needs 5 data packets to play smoothly
     *	    again, it should call ReportRebufferStatus() with 5,0 then as
     *	    packet arrive it should call again with 5,1; 5,2... and eventually
     *	    5,5.
     *
     */
    STDMETHOD(ReportRebufferStatus)	    (THIS_
					    UINT8   unNeeded,
					    UINT8   unAvailable) PURE;

    /************************************************************************
     *	Method:
     *	    IHXStream::SetGranularity
     *	Purpose:
     *	    Sets the desired Granularity for this stream. The actual 
     *	    granularity will be the lowest granularity of all streams.
     *	    Valid to call before stream actually begins. Best to call during
     *	    IHXRenderer::OnHeader().
     */
    STDMETHOD(SetGranularity)		    (THIS_
					    ULONG32 ulGranularity) PURE;

    /************************************************************************
     *	Method:
     *	    IHXStream::GetRendererCount
     *	Purpose:
     *	    Returns the current number of renderer instances supported by
     *	    this stream instance.
     */
    STDMETHOD_(UINT16, GetRendererCount)(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXStream::GetRenderer
     *	Purpose:
     *	    Returns the Nth renderer instance supported by this stream.
     */
    STDMETHOD(GetRenderer)	(THIS_
				UINT16		nIndex,
				REF(IUnknown*)	pUnknown) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXStream2
 *
 *  Purpose:
 *
 *	Interface provided by the client engine to the renderers. This
 *	interface allows access to stream related information and properties.
 *
 *  IID_IHXStream:
 *
 *	{00000400-0901-11d1-8B06-00A024406D5a}
 *
 */
DEFINE_GUID(IID_IHXStream2, 0x00000400, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x5a);

#undef  INTERFACE
#define INTERFACE   IHXStream2

DECLARE_INTERFACE_(IHXStream2, IHXStream)
{
    /************************************************************************
     *	Method:
     *	    IHXStream2::ReportAudioRebufferStatus
     *	Purpose:
     *      For audio only, when it's called, the rebuffer will only occur when
     *      there aren't any packets in the transport and the amount of audio in
     *      audio device falls below the minimum startup audio pushdown(1000ms
     *      by default)
     *      
     *      Non-audio renderers should still call ReportRebufferStatus(), the 
     *      rebuffer will occur when the core drains out all the packets from
     *      the transport buffer
     *
     *      The rest semantic are the same between the 2 calls.
     */
    STDMETHOD(ReportAudioRebufferStatus)    (THIS_
					    UINT8   unNeeded,
					    UINT8   unAvailable) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXStreamSource
 *
 *  Purpose:
 *
 *	Interface provided by the client engine to the renderers. This
 *	interface allows access to source related information and properties.
 *
 *  IID_IHXStreamSource:
 *
 *	{00000401-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXStreamSource, 0x00000401, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXStreamSource

DECLARE_INTERFACE_(IHXStreamSource, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     * IHXStreamSource methods
     */

    /************************************************************************
     *	Method:
     *		IHXStreamSource::IsLive
     *	Purpose:
     *		Ask the source whether it is live
     *
     */
    STDMETHOD_ (HXBOOL,IsLive)	(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXStreamSource::GetPlayer
     *	Purpose:
     *	    Get the interface to the player object of which the source is
     *	    a part of.
     *
     */
    STDMETHOD(GetPlayer)	    (THIS_
				    REF(IHXPlayer*)	pPlayer) PURE;

    /************************************************************************
     *	Method:
     *	    IHXStreamSource::GetURL
     *	Purpose:
     *	    Get the URL for this source. NOTE: The returned string is
     *	    assumed to be valid for the life of the IHXStreamSource from which
     *	    it was returned.
     *
     */
    STDMETHOD_(const char*,GetURL)  (THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXStreamSource::GetStreamCount
     *	Purpose:
     *	    Returns the current number of stream instances supported by
     *	    this source instance.
     */
    STDMETHOD_(UINT16, GetStreamCount)(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXStreamSource::GetStream
     *	Purpose:
     *	    Returns the Nth stream instance supported by this source.
     */
    STDMETHOD(GetStream)	(THIS_
				UINT16		nIndex,
				REF(IUnknown*)	pUnknown) PURE;
};


/****************************************************************************
 * 
 *  Interface:
 *
 *  	IHXPlayer
 *
 *  Purpose:
 *
 *	Interface provided by the client engine to the renderers. This
 *	interface allows access to player related information, properties,
 *	and operations.
 *
 *  IID_IHXPlayer:
 *
 *	{00000402-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXPlayer, 0x00000402, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXPlayer

DECLARE_INTERFACE_(IHXPlayer, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     * IHXPlayer methods
     */

    /************************************************************************
     *	Method:
     *		IHXPlayer::GetClientEngine
     *	Purpose:
     *		Get the interface to the client engine object of which the
     *		player is a part of.
     *
     */
    STDMETHOD(GetClientEngine)	(THIS_
				REF(IHXClientEngine*)	pEngine) PURE;

    /************************************************************************
     *	Method:
     *		IHXPlayer::IsDone
     *	Purpose:
     *		Ask the player if it is done with the current presentation
     *
     */
    STDMETHOD_(HXBOOL,IsDone)	(THIS) PURE;

    /************************************************************************
     *	Method:
     *		IHXPlayer::IsLive
     *	Purpose:
     *		Ask the player whether it contains the live source
     *
     */
    STDMETHOD_(HXBOOL,IsLive)	(THIS) PURE;

    /************************************************************************
     *	Method:
     *		IHXPlayer::GetCurrentPlayTime
     *	Purpose:
     *		Get the current time on the Player timeline
     *
     */
    STDMETHOD_(ULONG32,GetCurrentPlayTime)	(THIS) PURE;

    /************************************************************************
     *	Method:
     *		IHXPlayer::OpenURL
     *	Purpose:
     *		Tell the player to begin playback of all its sources.
     *
     */
    STDMETHOD(OpenURL)		(THIS_
				    const char*	pURL) PURE;

    /************************************************************************
     *	Method:
     *		IHXPlayer::Begin
     *	Purpose:
     *		Tell the player to begin playback of all its sources.
     *
     */
    STDMETHOD(Begin)		(THIS) PURE;

    /************************************************************************
     *	Method:
     *		IHXPlayer::Stop
     *	Purpose:
     *		Tell the player to stop playback of all its sources.
     *
     */
    STDMETHOD(Stop)		(THIS) PURE;

    /************************************************************************
     *	Method:
     *		IHXPlayer::Pause
     *	Purpose:
     *		Tell the player to pause playback of all its sources.
     *
     */
    STDMETHOD(Pause)		(THIS) PURE;

    /************************************************************************
     *	Method:
     *		IHXPlayer::Seek
     *	Purpose:
     *		Tell the player to seek in the playback timeline of all its 
     *		sources.
     *
     */
    STDMETHOD(Seek)		(THIS_
				ULONG32			ulTime) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPlayer::GetSourceCount
     *	Purpose:
     *	    Returns the current number of source instances supported by
     *	    this player instance.
     */
    STDMETHOD_(UINT16, GetSourceCount)(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPlayer::GetSource
     *	Purpose:
     *	    Returns the Nth source instance supported by this player.
     */
    STDMETHOD(GetSource)	(THIS_
				UINT16		nIndex,
				REF(IUnknown*)	pUnknown) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPlayer::SetClientContext
     *	Purpose:
     *	    Called by the client to install itself as the provider of client
     *	    services to the core. This is traditionally called by the top 
     *	    level client application.
     */
    STDMETHOD(SetClientContext)	(THIS_
				IUnknown* pUnknown) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPlayer::GetClientContext
     *	Purpose:
     *	    Called to get the client context for this player. This is
     *	    set by the top level client application.
     */
    STDMETHOD(GetClientContext)	(THIS_
				REF(IUnknown*) pUnknown) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPlayer::AddAdviseSink
     *	Purpose:
     *	    Call this method to add a client advise sink.
     *
     */
    STDMETHOD(AddAdviseSink)	(THIS_
				IHXClientAdviseSink*	pAdviseSink) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPlayer::RemoveAdviseSink
     *	Purpose:
     *	    Call this method to remove a client advise sink.
     */
    STDMETHOD(RemoveAdviseSink)	(THIS_
				IHXClientAdviseSink*	pAdviseSink) PURE;
};


/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXClientEngine
 *
 *  Purpose:
 *
 *	Interface to the basic client engine. Provided to the renderers and
 *	other client side components.
 *
 *  IID_IHXClientEngine:
 *
 *	{00000403-0901-11d1-8B06-00A024406D59}
 */
DEFINE_GUID(IID_IHXClientEngine, 0x00000403, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXClientEngine

DECLARE_INTERFACE_(IHXClientEngine, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     * IHXClientEngine methods
     */

    /************************************************************************
     *	Method:
     *	    IHXClientEngine::CreatePlayer
     *	Purpose:
     *	    Creates a new IHXPlayer instance.
     *
     */
    STDMETHOD(CreatePlayer)	(THIS_
				REF(IHXPlayer*)    pPlayer) PURE;

    /************************************************************************
     *	Method:
     *	    IHXClientEngine::ClosePlayer
     *	Purpose:
     *	    Called by the client when it is done using the player...
     *
     */
    STDMETHOD(ClosePlayer)	(THIS_
				IHXPlayer*    pPlayer) PURE;

    /************************************************************************
     *	Method:
     *	    IHXClientEngine::GetPlayerCount
     *	Purpose:
     *	    Returns the current number of IHXPlayer instances supported by
     *	    this client engine instance.
     */
    STDMETHOD_(UINT16, GetPlayerCount)(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXClientEngine::GetPlayer
     *	Purpose:
     *	    Returns the Nth IHXPlayer instances supported by this client 
     *	    engine instance.
     */
    STDMETHOD(GetPlayer)	(THIS_
				UINT16		nPlayerNumber,
				REF(IUnknown*)	pUnknown) PURE;

    /************************************************************************
     *	Method:
     *	    IHXClientEngine::EventOccurred
     *	Purpose:
     *	    Clients call this to pass OS events to all players. HXxEvent
     *	    defines a cross-platform event.
     */
    STDMETHOD(EventOccurred)	(THIS_
				HXxEvent* /*IN*/ pEvent) PURE;

};

// $Private:
/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXClientEngineMapper
 *
 *  Purpose:
 *
 *	Interface to the basic client engine. Provided to the renderers and
 *	other client side components.
 *
 *  IID_IHXClientEngineMapper:
 *
 *	{0000040A-0901-11d1-8B06-00A024406D59}
 */
DEFINE_GUID(IID_IHXClientEngineMapper, 0x0000040A, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXClientEngineMapper

DECLARE_INTERFACE_(IHXClientEngineMapper, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     * IHXClientEngineMapper methods
     */

    /************************************************************************
     *	Method:
     *	    IHXClientEngineMapper::GetPlayerBySite
     *	Purpose:
     *	    Returns the IHXPlayer instance supported by this client 
     *	    engine instance that contains the specified IHXSite.
     */
    STDMETHOD(GetPlayerBySite)	(THIS_
				IHXSite*	pSite,
				REF(IUnknown*)	pUnknown) PURE;
};
// $EndPrivate:

#if defined _UNIX && !defined (_VXWORKS)
DEFINE_GUID(IID_IHXClientEngineSelector, 0x00000404, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXClientEngineSelector

DECLARE_INTERFACE_(IHXClientEngineSelector, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXClientEngine::Select
     *	Purpose:
     *      Top level clients under Unix should use this instead of
     *      select() to select for events.
     */
    STDMETHOD_(INT32, Select) (THIS_
			       INT32 n,
			       fd_set *readfds,
			       fd_set *writefds,
			       fd_set *exceptfds,
			       struct timeval* timeout) PURE;
};
#endif /* _UNIX */

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXClientEngineSetup
 *
 *  Purpose:
 *
 *	Interface to the basic client engine. Provided to the renderers and
 *	other client side components.
 *
 *  IID_IHXClientEngineSetup:
 *
 *	{00000405-0901-11d1-8B06-00A024406D59}
 */
DEFINE_GUID(IID_IHXClientEngineSetup, 0x00000405, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXClientEngineSetup

DECLARE_INTERFACE_(IHXClientEngineSetup, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     * IHXClientEngineSetup methods
     */

    /************************************************************************
     *	Method:
     *	    IHXClientEngineSetup::Setup
     *	Purpose:
     *      Top level clients use this interface to over-ride certain basic 
     *	    interfaces implemented by the core. Current over-ridable 
     *	    interfaces are: IHXPreferences, IHXHyperNavigate
     */
    STDMETHOD(Setup)		(THIS_
				IUnknown* pContext) PURE;
};


/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXInfoLogger
 *
 *  Purpose:
 *
 *	Interface to send any logging information back to the server.
 *	This information will appear in the server's access log.
 *
 *  IID_IHXInfoLogger:
 *
 *	{00000409-0901-11d1-8B06-00A024406D59}
 */
DEFINE_GUID(IID_IHXInfoLogger, 0x00000409, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXInfoLogger

DECLARE_INTERFACE_(IHXInfoLogger, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)		(THIS_
					REFIID riid,
					void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)		(THIS) PURE;

    STDMETHOD_(ULONG32,Release)		(THIS) PURE;

    /*
     *	IHXInfoLogger methods
     */

    /************************************************************************
     *	Method:
     *	    IHXInfoLogger::LogInformation
     *	Purpose:
     *	    Logs any user defined information in form of action and 
     *	    associated data.
     */
    STDMETHOD(LogInformation)		(THIS_				
					const char* /*IN*/ pAction,
					const char* /*IN*/ pData) PURE;
};


// $Private:
/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXPersistenceManager
 *
 *  Purpose:
 *
 *
 *  IID_IHXPersistenceManager:
 *
 *	{0000040B-0901-11d1-8B06-00A024406D59}
 */
DEFINE_GUID(IID_IHXPersistenceManager, 0x0000040B, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXPersistenceManager

DECLARE_INTERFACE_(IHXPersistenceManager, IUnknown)
{
   /*
    *  IUnknown methods
    */
   STDMETHOD(QueryInterface)       (THIS_
                   REFIID riid,
                   void** ppvObj) PURE;

   STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;

   STDMETHOD_(ULONG32,Release)     (THIS) PURE;

   /*
    *  IHXPersistenceManager methods
    */
   /************************************************************************
   *  Method:
   *      IHXPersistenceManager::AddPersistentComponent
   *  Purpose:
   *	   We currently allow ONLY IHXRenderer to register itself as a 
   *	   persistent component.
   *       Renderer registers itself as a persistent renderer if it wants
   *       to live across group boundaries.
   *       It will not be unloaded until
   *           a) Someone opens the next URL.
   *           b) All the groups within the current presentation have been 
   *              played.
   */
   STDMETHOD(AddPersistentComponent)    (THIS_
                                       IUnknown*  /*IN*/ pComponent) PURE;

   /************************************************************************
   *  Method:
   *      IHXPersistenceManager::RemovePersistentComponent
   *  Purpose:
   *       Remove an earlier registered persistent component.
   */
   STDMETHOD(RemovePersistentComponent) (THIS_
                                       IUnknown*  /*IN*/ pComponent) PURE;

   /************************************************************************
   *  Method:
   *      IHXPersistenceManager::GetPersistentComponent
   *  Purpose:
   *       Return an earlier registered persistent component.
   */
   STDMETHOD(GetPersistentComponent) (THIS_
                                     REF(IUnknown*)  /*OUT*/ pComponent) PURE;

};

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXDriverStreamManager
 *
 *  Purpose:
 *	Methods to notify/update driver stream renderer
 *
 *
 *  IID_IHXDriverStreamManager
 *
 *	{0000040C-0901-11d1-8B06-00A024406D59}
 */
DEFINE_GUID(IID_IHXDriverStreamManager, 0x0000040C, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXDriverStreamManager

DECLARE_INTERFACE_(IHXDriverStreamManager, IUnknown)
{
   /*
    *  IUnknown methods
    */
   STDMETHOD(QueryInterface)       (THIS_
                   REFIID riid,
                   void** ppvObj) PURE;

   STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;

   STDMETHOD_(ULONG32,Release)     (THIS) PURE;

   /*
    *  IHXDriverStreamManager methods
    */
   /************************************************************************
   *  Method:
   *      IHXDriverStreamManager::AddRendererAdviseSink
   *  Purpose:
   *	  Add a renderer advise sink 
   * 
   */
   STDMETHOD(AddRendererAdviseSink)      (THIS_
                               	 	 IHXRendererAdviseSink* pSink) PURE;

   /************************************************************************
   *  Method:
   *      IHXDriverStreamManager::SetDriverStreamManager
   *  Purpose:
   *	  Remove an advise sink
   * 
   */
   STDMETHOD(RemoveRendererAdviseSink)   (THIS_
                               	 	 IHXRendererAdviseSink* pSink) PURE;
};


/****************************************************************************
 * 
 *  Interface:
 *	IHXRendererAdviseSink
 *
 *  Purpose:
 *	Provides access to notifications of initialization/changes to
 *	renderers in the player.
 *
 *  IID_IHXRendererAdviseSink
 *
 *	{0000040D-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXRendererAdviseSink, 0x0000040D, 0x901, 
            0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXRendererAdviseSink

DECLARE_INTERFACE_(IHXRendererAdviseSink, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     * IHXRendererAdviseSink methods
     */

    STDMETHOD(TrackDurationSet)	(THIS_
				UINT32 ulGroupIndex,
				UINT32 ulTrackIndex,
				UINT32 ulDuration,
				UINT32 ulDelay,
				HXBOOL   bIsLive) PURE;

    STDMETHOD(RepeatedTrackDurationSet)	(THIS_
					const char*  pID,
					UINT32 ulDuration,					
				        HXBOOL   bIsLive) PURE;

    STDMETHOD(TrackUpdated)	(THIS_
				UINT32 ulGroupIndex,
				UINT32 ulTrackIndex,
				IHXValues* pValues) PURE;

   /************************************************************************
   *  Method:
   *      IHXRendererAdviseSink::RendererInitialized
   *  Purpose:
   *	  Notification of renderer initialization
   * 
   */
    STDMETHOD(RendererInitialized)	(THIS_
					IHXRenderer* pRenderer,
					IUnknown* pStream,
					IHXValues* pInfo) PURE;

   /************************************************************************
   *  Method:
   *      IHXRendererAdviseSink::RendererClosed
   *  Purpose:
   *	  Notification of renderer close
   * 
   */
    STDMETHOD(RendererClosed)		(THIS_
					IHXRenderer* pRenderer,
					IHXValues* pInfo) PURE;
};


/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXLayoutStream
 *
 *  Purpose:
 *
 *	Interface that allows access/updates to stream properties
 *
 *  IID_IHXLayoutStream:
 *
 *	{0000040E-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXLayoutStream, 0x0000040E, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXLayoutStream

DECLARE_INTERFACE_(IHXLayoutStream, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     * IHXLayoutStream methods
     */

    /************************************************************************
     *	Method:
     *	    IHXLayoutStream::GetProperty
     *	Purpose:
     *	    Get layout stream property
     *	    
     *
     */
    STDMETHOD(GetProperties)		(THIS_
					REF(IHXValues*) pValue) PURE;

    /************************************************************************
     *	Method:
     *	    IHXLayoutStream::SetProperty
     *	Purpose:
     *	    Set layout stream property
     *
     */
    STDMETHOD(SetProperties)		(THIS_
					IHXValues* pValue) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXRendererUpgrade
 *
 *  Purpose:
 *
 *	Interface that tells the player to upgrade a particular set of renderers
 *
 *  IID_IHXRendererUpgrade:
 *
 *	{00000410-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXRendererUpgrade, 0x00000410, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXRendererUpgrade

DECLARE_INTERFACE_(IHXRendererUpgrade, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     * IHXRendererUpgrade methods
     */

    /************************************************************************
     *	Method:
     *	    IHXRendererUpgrade::IsRendererAvailable
     *	Purpose:
     *	    Find out if the renderer is already loaded
     *	    
     *
     */
    STDMETHOD_(HXBOOL,IsRendererAvailable)	(THIS_
						const char* pMimeType) PURE;

    /************************************************************************
     *	Method:
     *	    IHXRendererUpgrade::ForceUpgrade
     *	Purpose:
     *	    Force an upgrade for any unloaded renderers
     *
     */
    STDMETHOD(ForceUpgrade)			(THIS) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXValidator
 *
 *  Purpose:
 *
 *	Interface that provides validation
 *
 *  IID_IHXValidator:
 *
 *	{00000412-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXValidator, 0x00000412, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXValidator

DECLARE_INTERFACE_(IHXValidator, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     * IHXValidator methods
     */

    /************************************************************************
     *	Method:
     *	    IHXValidator::ValidateProtocol
     *	Purpose:
     *	    Find out if the protocol is valid
     *	    
     *
     */
    STDMETHOD_(HXBOOL,ValidateProtocol)	(THIS_
					 char* pProtocol) PURE;

    /************************************************************************
     *	Method:
     *	    IHXValidator::ValidateMetaFile
     *	Purpose:
     *	    Find out if it is a meta file
     *	    
     *
     */
    STDMETHOD(ValidateMetaFile)	(THIS_
				 IHXRequest* pRequest,
				 IHXBuffer* pContent) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXPrivateStreamSource
 *
 *  Purpose:
 *		This interface is being added for the sole purpose of implementing 
 *		IsSaveAllowed on our stream source objects.  We need to get this in
 *		for alpha-3 of the player, since it would be very bad to put out a 
 *		player version that allowed recording of content that was supposed to
 *		be unrecordable.  This method should be moved into IHXStreamSource
 *		as soon as is convenient.
 *
 *  IHXPrivateStreamSource:
 *
 *	 {57DFD0E2-C76E-11d1-8B5C-006008065552}
 *
 */
 
DEFINE_GUID(IID_IHXPrivateStreamSource, 0x57dfd0e2, 0xc76e, 0x11d1, 0x8b, 0x5c, 
		0x0, 0x60, 0x8, 0x6, 0x55, 0x52);

#undef  INTERFACE
#define INTERFACE   IHXPrivateStreamSource

DECLARE_INTERFACE_(IHXPrivateStreamSource, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     * IHXPrivateStreamSource methods
     */


    STDMETHOD_ (HXBOOL,IsSaveAllowed)	(THIS) PURE;

};

// $EndPrivate.

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXPlayer2
 *
 *  Purpose:
 *
 *	Extra methods in addition to IHXPlayer
 *
 *  IID_IHXPlayer2:
 *
 *	{00000411-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXPlayer2, 0x00000411, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXPlayer2

DECLARE_INTERFACE_(IHXPlayer2, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IID_IHXPlayer2::SetMinimumPreroll
     *	Purpose:
     *	    Call this method to set the minimum preroll of this clip
     */
    STDMETHOD(SetMinimumPreroll) (THIS_
				UINT32	ulMinPreroll) PURE;

    /************************************************************************
     *	Method:
     *	    IID_IHXPlayer2::GetMinimumPreroll
     *	Purpose:
     *	    Call this method to get the minimum preroll of this clip
     */
    STDMETHOD(GetMinimumPreroll) (THIS_
				REF(UINT32) ulMinPreroll) PURE;

    /************************************************************************
     *	Method:
     *	    IID_IHXPlayer2::OpenRequest
     *	Purpose:
     *	    Call this method to open the IHXRequest
     */
    STDMETHOD(OpenRequest) (THIS_
			    IHXRequest* pRequest) PURE;

    /************************************************************************
     *	Method:
     *	    IID_IHXPlayer2::GetRequest
     *	Purpose:
     *	    Call this method to get the IHXRequest
     */
    STDMETHOD(GetRequest) (THIS_
			   REF(IHXRequest*) pRequest) PURE;
};

// $Private:
/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXUpdateProperties
 *
 *  Purpose:
 *
 *	update any offset related stuff
 *
 *  IID_IHXUpdateProperties:
 *
 *	{00000413-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXUpdateProperties, 0x00000413, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXUpdateProperties

DECLARE_INTERFACE_(IHXUpdateProperties, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXUpdateProperties::UpdatePacketTimeOffset
     *	Purpose:
     *	    Call this method to update the timestamp offset of cached packets
     */
    STDMETHOD(UpdatePacketTimeOffset) (THIS_
				       INT32 lTimeOffset) PURE;

    /************************************************************************
     *	Method:
     *	    IHXUpdateProperties::UpdatePlayTimes
     *	Purpose:
     *	    Call this method to update properties
     */
    STDMETHOD(UpdatePlayTimes)	      (THIS_
				       IHXValues* pProps) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXUpdateProperties
 *
 *  Purpose:
 *
 *	update any offset related stuff
 *
 *  IID_IHXUpdateProperties:
 *
 *	{00000413-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXUpdateProperties2, 0x00000413, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x5a);

#undef  INTERFACE
#define INTERFACE   IHXUpdateProperties2

DECLARE_INTERFACE_(IHXUpdateProperties2, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXUpdateProperties::UpdateHeader
     *	Purpose:
     *	    Call this method to update the stream header
     */
    STDMETHOD(UpdateHeader)     (THIS_
				 IHXValues* pProps) PURE;
};
// $EndPrivate.

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXPlayerNavigator
 *
 *  Purpose:
 *
 *	navigate player objects
 *
 *  IID_IHXPlayerNavigator:
 *
 *	{00000414-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXPlayerNavigator, 0x00000414, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXPlayerNavigator

DECLARE_INTERFACE_(IHXPlayerNavigator, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPlayerNavigator::AddChildPlayer
     *	Purpose:
     *	    Add child player to the current player
     */
    STDMETHOD(AddChildPlayer)	    (THIS_
				    IHXPlayer* pPlayer) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPlayerNavigator::RemoveChildPlayer
     *	Purpose:
     *	    Remove child player from the current player
     */
    STDMETHOD(RemoveChildPlayer)    (THIS_
				    IHXPlayer* pPlayer) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPlayerNavigator::GetNumChildPlayer
     *	Purpose:
     *	    Get number of the child players
     */
    STDMETHOD_(UINT16, GetNumChildPlayer)    (THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPlayerNavigator::GetChildPlayer
     *	Purpose:
     *	    Get Nth child player
     */
    STDMETHOD(GetChildPlayer)	    (THIS_
				    UINT16 uPlayerIndex,
				    REF(IHXPlayer*) pPlayer) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPlayerNavigator::SetParentPlayer
     *	Purpose:
     *	    Set the parent player
     */
    STDMETHOD(SetParentPlayer)	    (THIS_
				    IHXPlayer* pPlayer) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPlayerNavigator::RemoveParentPlayer
     *	Purpose:
     *	    Remove the parent player
     */
    STDMETHOD(RemoveParentPlayer)   (THIS_
				    IHXPlayer* pPlayer) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPlayerNavigator::GetParentPlayer
     *	Purpose:
     *	    Get the parent player
     */
    STDMETHOD(GetParentPlayer)	    (THIS_
				    REF(IHXPlayer*) pPlayer) PURE;
};

// $Private:
/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXPersistentComponentManager
 *
 *  Purpose:
 *
 *	persistent component manager
 *
 *  IID_IHXPersistentComponentManager
 *
 *	{00000415-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXPersistentComponentManager, 0x00000415, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXPersistentComponentManager

DECLARE_INTERFACE_(IHXPersistentComponentManager, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPersistentComponentManager::CreatePersistentComponent
     *	Purpose:
     *	    create persistent component
     */
    STDMETHOD(CreatePersistentComponent)    (THIS_
					    REF(IHXPersistentComponent*)   pPersistentComponent) PURE;


    /************************************************************************
     *	Method:
     *	    IHXPersistentComponentManager::AddPersistentComponent
     *	Purpose:
     *	    add persistent component
     */
    STDMETHOD(AddPersistentComponent)	    (THIS_
					    IHXPersistentComponent*	pPersistentComponent) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPersistentComponentManager::RemovePersistentComponent
     *	Purpose:
     *	    remove persistent component
     */
    STDMETHOD(RemovePersistentComponent)    (THIS_
					    UINT32 ulPersistentComponentID) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPersistentComponentManager::GetPersistentComponentInfo
     *	Purpose:
     *	    get persistent component information
     */
    STDMETHOD(GetPersistentComponent)	    (THIS_
					    UINT32			    ulPersistentComponentID,
					    REF(IHXPersistentComponent*)   pPersistentComponent) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPersistentComponentManager::AttachPersistentComponentLayout
     *	Purpose:
     *	    get persistent component information
     */
    STDMETHOD(AttachPersistentComponentLayout)	(THIS_
						IUnknown*   pLSG,
						IHXValues* pProps) PURE;	
};

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXPersistentComponent
 *
 *  Purpose:
 *
 *	persistent component
 *
 *  IID_IHXPersistentComponent
 *
 *	{00000416-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXPersistentComponent, 0x00000416, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXPersistentComponent

DECLARE_INTERFACE_(IHXPersistentComponent, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPersistentComponent::Init
     *	Purpose:
     *	    initialize persistent component
     */
    STDMETHOD(Init)			(THIS_
                               		IHXPersistentRenderer* pPersistentRenderer) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPersistentComponent::AddRendererAdviseSink
     *	Purpose:
     *	    add renderer advise sink
     */
    STDMETHOD(AddRendererAdviseSink)	(THIS_
                               		IHXRendererAdviseSink* pSink) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPersistentComponent::RemoveRendererAdviseSink
     *	Purpose:
     *	    remove renderer advise sink
     */
    STDMETHOD(RemoveRendererAdviseSink)	(THIS_
                               		IHXRendererAdviseSink* pSink) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPersistentComponent::AddGroupSink
     *	Purpose:
     *	    add renderer advise sink
     */
    STDMETHOD(AddGroupSink)		(THIS_
                               		IHXGroupSink* pSink) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPersistentComponent::RemoveGroupSink
     *	Purpose:
     *	    remove renderer advise sink
     */
    STDMETHOD(RemoveGroupSink)		(THIS_
                               		IHXGroupSink* pSink) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPersistentComponent::GetPersistentRenderer
     *	Purpose:
     *	    get persistent renderer
     */
    STDMETHOD(GetPersistentRenderer)	(THIS_
                               		REF(IHXPersistentRenderer*) pPersistentRenderer) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPersistentComponent::GetPersistentProperties
     *	Purpose:
     *	    get persistent component properties
     */
    STDMETHOD(GetPersistentProperties)	(THIS_
                               		REF(IHXValues*) pProperties) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 *
 *	IID_IHXClientStatisticsGranularity
 *
 *  Purpose:
 *
 *	Enables users to set the Granularity at which statistics are 
 *	gathered. This allows machines under high load to still run
 *	efficiently.
 *	
 *
 *  IID_IHXClientStatisticsGranularity
 *
 *	{00000416-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXClientStatisticsGranularity, 0x00000417, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXClientStatisticsGranularity

DECLARE_INTERFACE_(IHXClientStatisticsGranularity, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXClientStatisticsGranularity::SetStatsGranularity
     *	Purpose:
     *	    Set the granularity
     */
    STDMETHOD(SetStatsGranularity) (THIS_ ULONG32 ulGranularity) PURE;
};

// $EndPrivate.

// $Private:

/****************************************************************************
 * 
 *  Interface:
 *
 *	IID_IHXSourceBufferingStats
 *
 *  Purpose:
 *
 *	Enables users to get the current buffering status of the 
 *	given source. 
 *	
 *
 *      IID_IHXSourceBufferingStats
 *
 *	{00000418-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXSourceBufferingStats, 0x00000418, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXSourceBufferingStats

DECLARE_INTERFACE_(IHXSourceBufferingStats, IUnknown)
{
    /************************************************************************
     *	Method:
     *	    IHXSourceBufferingStats::GetCurrentBuffering
     *	Purpose:
     *	    Get the current buffering information
     */

    STDMETHOD(GetCurrentBuffering)  (THIS_ UINT16  uStreamNumber,
				     REF(INT64)  llLowestTimestamp, 
				     REF(INT64)  llHighestTimestamp,
				     REF(UINT32) ulNumBytes,
				     REF(HXBOOL)   bDone) PURE;
};


/****************************************************************************
 * 
 *  Interface:
 *
 *	IID_IHXSourceBufferingStats2
 *
 *  Purpose:
 *
 *	Enables users to get the current buffering status of the 
 *	given source. 
 *	
 *
 *      IID_IHXSourceBufferingStats2
 *
 *	{00000418-0901-11d1-8B06-00A024406D5A}
 *
 */
DEFINE_GUID(IID_IHXSourceBufferingStats2, 0x00000418, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x5a);

#undef  INTERFACE
#define INTERFACE   IHXSourceBufferingStats2

DECLARE_INTERFACE_(IHXSourceBufferingStats2, IHXSourceBufferingStats)
{
    /************************************************************************
     *	Method:
     *	    IHXSourceBufferingStats2::GetCurrentBuffering
     *	Purpose:
     *	    Get the amount of buffering in the transport
     */
    STDMETHOD(GetCurrentBuffering)  (THIS_ UINT16  uStreamNumber,
				     REF(INT64)  llLowestTimestamp, 
				     REF(INT64)  llHighestTimestamp,
				     REF(UINT32) ulNumBytes,
				     REF(HXBOOL)   bDone) PURE;

    /************************************************************************
     *	Method:
     *	    IHXSourceBufferingStats2::GetTotalBuffering
     *	Purpose:
     *	    Get the total amount of data buffered for a stream.
     *      This includes what is in the transport buffer and in
     *      the renderer
     */

    STDMETHOD(GetTotalBuffering)  (THIS_ UINT16  uStreamNumber,
				   REF(INT64)  llLowestTimestamp, 
				   REF(INT64)  llHighestTimestamp,
				   REF(UINT32) ulNumBytes,
				   REF(HXBOOL)   bDone) PURE;
};

// $EndPrivate.

/****************************************************************************
 * 
 *  Interface:
 *
 *	IID_IHXSourceLatencyStats
 *
 *  Purpose:
 *
 *	
 *
 *      IHXSourceLatencyStats
 *
 *	{7A4D7872-E5A9-11D8-ABE7-000A95BEFE6C}
 *
 */
DEFINE_GUID(IID_IHXSourceLatencyStats, 0x7A4D7872, 0xE5A9, 0x11D8, 0xAB, 0xE7, 0x00, 
			0x0A, 0x95, 0xBE, 0xFE, 0x6C);

#undef  INTERFACE
#define INTERFACE   IHXSourceLatencyStats

DECLARE_INTERFACE_(IHXSourceLatencyStats, IUnknown)
{
    /************************************************************************
     *	Method:
     *	    IHXSourceLatencyStats::SetLiveSyncOffset
     *	Purpose:
     *	    set the live sync start time
     */
    STDMETHOD(SetLiveSyncOffset)  (THIS_ UINT32  ulLiveSyncStartTime) PURE;

    /************************************************************************
     *	Method:
     *	    IHXSourceLatencyStats::NewPacketTimeStamp
     *	Purpose:
     *      call this for each arriving packet!
     */

    STDMETHOD(NewPacketTimeStamp)  (THIS_ UINT32  ulDueTimeStamp) PURE;
    
    /************************************************************************
     *	Method:
     *	    IHXSourceLatencyStats::GetLatencyStats
     *	Purpose:
     *      call this for each arriving packet!
     */

    STDMETHOD(GetLatencyStats)  (THIS_
                                     REF(UINT32) ulAverageLatency,
                                     REF(UINT32) ulMinimumLatency,
                                     REF(UINT32) ulMaximumLatency ) PURE;
    
    
    /************************************************************************
     *	Method:
     *	    IHXSourceLatencyStats::ResetLatencyStats
     *	Purpose:
     *      call this for each arriving packet!
     */

    STDMETHOD(ResetLatencyStats)  (THIS_ ) PURE;
    
    
};


// $Private:

/****************************************************************************
 * 
 *  Interface:
 *
 *	IID_IHXPlayerPresentation
 *
 *  Purpose:
 *
 *	Control over the player's current presentation
 *	
 *      IID_IHXPlayerPresentation
 *
 *	{6DE011A7-EF05-417b-9367-6FE0E54302D3}
 *
 */
DEFINE_GUID(IID_IHXPlayerPresentation, 0x6de011a7, 0xef05, 0x417b, 0x93, 0x67, 0x6f, 0xe0, 0xe5, 0x43, 0x2, 0xd3);

#undef  INTERFACE
#define INTERFACE   IHXPlayerPresentation

DECLARE_INTERFACE_(IHXPlayerPresentation, IUnknown)
{
    /************************************************************************
     *	Method:
     *	    IHXPlayerPresentation::ClosePresentation
     *	Purpose:
     *	    Call this method to close the player's current presentation.  This will free
     *	    all resources associated with the current presentation.
     */
    STDMETHOD(ClosePresentation)    (THIS) PURE;
};

// $EndPrivate.



// $Private:

/****************************************************************************
 * 
 *  Interface:
 *
 *	IID_IHXCoreMutex
 *
 *  Purpose:
 *
 *	Access the core mutex
 *	
 *      IID_IHXCoreMutex
 *
 *	{6DE011A7-EF05-417b-9367-6FE0E44404D4}
 *
 */
DEFINE_GUID(IID_IHXCoreMutex, 0x6de011a7, 0xef05, 0x417b, 0x93, 0x67, 0x6f, 0xe0, 0xe4, 0x44, 0x4, 0xd4);

#undef  INTERFACE
#define INTERFACE   IHXCoreMutex

DECLARE_INTERFACE_(IHXCoreMutex, IUnknown)
{
    /************************************************************************
     *	Method:
     *	    IHXCoreMutex::LockCoreMutex
     *	Purpose:
     *      Call this method to lock the client engine's core mutex.
     */
    STDMETHOD(LockCoreMutex)    (THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXCoreMutex::UnlockCoreMutex
     *	Purpose:
     *      Call this method to unlock the client engine's core mutex.
     */
    STDMETHOD(UnlockCoreMutex)    (THIS) PURE;
};


/****************************************************************************
 * 
 *  Interface:
 *
 *	IID_IHXMacBlitMutex
 *
 *  Purpose:
 *
 *	Access the Mac blitting mutex
 *
 *	Used for all Mac drawing, in both the core and the tlc. This
 *	mutex prevents quickdraw (and/or the image compression manager)
 *	from re-entering which causes crashes on OS X.
 *	
 *      IID_IHXCoreMutex
 *
 *	{294e6de4-fbc6-4c06-bb94-95a969373b4d}
 *
 */
DEFINE_GUID(IID_IHXMacBlitMutex, 0x294e6de4,  0xfbc6,  0x4c06,  0xbb,  0x94,  0x95, 0xa9,  0x69,  0x37,  0x3b,  0x4d);

#undef  INTERFACE
#define INTERFACE   IHXMacBlitMutex

DECLARE_INTERFACE_(IHXMacBlitMutex, IUnknown)
{
    /************************************************************************
     *	Method:
     *	    IHXMacBlitMutex::LockMacBlitMutex
     *	Purpose:
     *      Call this method to lock the client engine's core mutex.
     */
    STDMETHOD(LockMacBlitMutex)    (THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXMacBlitMutex::UnlockMacBlitMutex
     *	Purpose:
     *      Call this method to unlock the client engine's core mutex.
     */
    STDMETHOD(UnlockMacBlitMutex)    (THIS) PURE;
};

// $EndPrivate.



#endif /* _HXCORE_H_ */


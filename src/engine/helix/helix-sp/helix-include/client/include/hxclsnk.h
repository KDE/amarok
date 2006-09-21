
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

#ifndef _HXCLSNK_H_
#define _HXCLSNK_H_

/*
 * Forward declarations of some interfaces defined or used here-in.
 */
typedef _INTERFACE   IHXClientAdviseSink       IHXClientAdviseSink;
typedef _INTERFACE   IHXRequest		IHXRequest;


/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXClientAdviseSink
 * 
 *  Purpose:
 * 
 *	Interface supplied by client to core to receive notifications of
 *	status changes.
 * 
 *  IID_IHXClientAdviseSink:
 * 
 *	{00000B00-0901-11d1-8B06-00A024406D59}
 * 
 */

DEFINE_GUID(IID_IHXClientAdviseSink, 0x00000B00, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXClientAdviseSink

DECLARE_INTERFACE_(IHXClientAdviseSink, IUnknown)
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
     *	IHXClientAdviseSink methods
     */

    /************************************************************************
     *	Method:
     *	    IHXClientAdviseSink::OnPosLength
     *	Purpose:
     *	    Called to advise the client that the position or length of the
     *	    current playback context has changed.
     */
    STDMETHOD(OnPosLength)		(THIS_
					UINT32	  ulPosition,
					UINT32	  ulLength) PURE;

    /************************************************************************
     *	Method:
     *	    IHXClientAdviseSink::OnPresentationOpened
     *	Purpose:
     *	    Called to advise the client a presentation has been opened.
     */
    STDMETHOD(OnPresentationOpened)	(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXClientAdviseSink::OnPresentationClosed
     *	Purpose:
     *	    Called to advise the client a presentation has been closed.
     */
    STDMETHOD(OnPresentationClosed)	(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXClientAdviseSink::OnStatisticsChanged
     *	Purpose:
     *	    Called to advise the client that the presentation statistics
     *	    have changed. 
     */
    STDMETHOD(OnStatisticsChanged)	(THIS) PURE;

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
    STDMETHOD(OnPreSeek)	(THIS_
				ULONG32		    ulOldTime,
				ULONG32		    ulNewTime) PURE;

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
    STDMETHOD(OnPostSeek)	(THIS_
				ULONG32		    ulOldTime,
				ULONG32		    ulNewTime) PURE;

    /************************************************************************
     *	Method:
     *	    IHXClientAdviseSink::OnStop
     *	Purpose:
     *	    Called by client engine to inform the client that a stop has
     *	    just occurred. 
     *
     */
    STDMETHOD(OnStop)		(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXClientAdviseSink::OnPause
     *	Purpose:
     *	    Called by client engine to inform the client that a pause has
     *	    just occurred. The render is informed the last time for the 
     *	    stream's time line before the pause.
     *
     */
    STDMETHOD(OnPause)		(THIS_
				ULONG32		    ulTime) PURE;

    /************************************************************************
     *	Method:
     *	    IHXClientAdviseSink::OnBegin
     *	Purpose:
     *	    Called by client engine to inform the client that a begin or
     *	    resume has just occurred. The render is informed the first time 
     *	    for the stream's time line after the resume.
     *
     */
    STDMETHOD(OnBegin)		(THIS_
				ULONG32		    ulTime) PURE;

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
    STDMETHOD(OnBuffering)	(THIS_
				ULONG32		    ulFlags,
				UINT16		    unPercentComplete) PURE;

    /************************************************************************
     *	Method:
     *	    IHXClientAdviseSink::OnContacting
     *	Purpose:
     *	    Called by client engine to inform the client is contacting
     *	    hosts(s).
     *
     */
    STDMETHOD(OnContacting)	(THIS_
				 const char*	pHostName) PURE;
};


// $Private:

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXClientRequestSink
 *
 *  Purpose:
 *
 *	Enables top level clients to get notified of new URLs
 *	
 *
 *      IID_IHXClientRequestSink
 *
 *	{00000B01-0901-11d1-8B06-00A024406D59}
 * 
 */

DEFINE_GUID(IID_IHXClientRequestSink, 0x00000B01, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXClientRequestSink

DECLARE_INTERFACE_(IHXClientRequestSink, IUnknown)
{
    /************************************************************************
     *	Method:
     *	    IHXClientRequestSink::OnNewRequest
     *	Purpose:
     *	    Inform TLC of the new request. The TLC may choose to 
     *	    modify RequestHeaders at this time.
     */

    STDMETHOD(OnNewRequest)  (THIS_ IHXRequest* pNewRequest) PURE;
};

// $EndPrivate.

#endif /* _HXCLSNK_H_ */

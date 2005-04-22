
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

#ifndef _HXVSRC_H
#define _HXVSRC_H

typedef _INTERFACE	IHXStreamSource		IHXStreamSource;
typedef _INTERFACE	IHXFileObject			IHXFileObject;

// Interfaces definded in this file
typedef _INTERFACE	IHXFileViewSource		IHXFileViewSource;
typedef _INTERFACE	IHXFileViewSourceResponse	IHXFileViewSourceResponse;
typedef _INTERFACE	IHXViewSourceCommand		IHXViewSourceCommand;
typedef _INTERFACE	IHXViewSourceURLResponse	IHXViewSourceURLResponse;

// $Private:
typedef _INTERFACE	IHXClientViewSource		IHXClientViewSource;
typedef _INTERFACE	IHXClientViewSourceSink	IHXClientViewSourceSink;
// $EndPrivate.



/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXFileViewSource
 * 
 *  IID_IHXFileViewSource:
 * 
 *	{00003500-0901-11d1-8B06-00A024406D59}
 * 
 */

enum SOURCE_TYPE
{
    RAW_SOURCE,
    HTML_SOURCE
};

DEFINE_GUID(IID_IHXFileViewSource, 0x00003500, 0x901, 0x11d1, 0x8b, 0x6,
	    0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXFileViewSource

DECLARE_INTERFACE_(IHXFileViewSource, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;
    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /************************************************************************
     *  IHXFileViewSource
     */
    STDMETHOD(InitViewSource)		(THIS_
	IHXFileObject*		    /*IN*/ pFileObject,
	IHXFileViewSourceResponse* /*IN*/ pResp,
	SOURCE_TYPE		    /*IN*/ sourceType,
	IHXValues*		    /*IN*/ pOptions) PURE;
    STDMETHOD(GetSource)    (THIS) PURE;
    STDMETHOD(Close)	    (THIS) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXFileViewSourceResponse
 * 
 *  IID_IHXFileViewSourceResponse:
 * 
 *	{00003501-0901-11d1-8B06-00A024406D59}
 * 
 */

DEFINE_GUID(IID_IHXFileViewSourceResponse, 0x00003501, 0x901, 0x11d1, 0x8b,
	    0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXFileViewSourceResponse

DECLARE_INTERFACE_(IHXFileViewSourceResponse, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;
    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /************************************************************************
     *  IHXFileViewSourceResoponse
     */
    STDMETHOD(InitDone)		(THIS_ HX_RESULT status	) PURE;
    STDMETHOD(SourceReady)	(THIS_ HX_RESULT status,
	IHXBuffer* pSource ) PURE;
    STDMETHOD(CloseDone) (THIS_ HX_RESULT) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXViewSourceCommand
 * 
 *  IID_IHXViewSourceCommand:
 * 
 *	{00003504-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXViewSourceCommand, 0x00003504, 0x901, 0x11d1, 0x8b, 0x6,
	    0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXViewSourceCommand

DECLARE_INTERFACE_(IHXViewSourceCommand, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;
    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /************************************************************************
     *  IHXViewSourceCommand
     */
    STDMETHOD_(HXBOOL, CanViewSource)	(THIS_
					IHXStreamSource*		pStream) PURE;
    STDMETHOD(DoViewSource)		(THIS_
					IHXStreamSource*		pStream) PURE;
    STDMETHOD(GetViewSourceURL) (THIS_
					IHXStreamSource*		pSource,
					IHXViewSourceURLResponse*      pResp) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXViewSourceURLResponse
 * 
 *  IID_IHXViewSourceURLResponse:
 * 
 *	{00003505-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXViewSourceURLResponse, 0x00003505, 0x901, 0x11d1, 0x8b, 0x6,
	    0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXViewSourceURLResponse

DECLARE_INTERFACE_(IHXViewSourceURLResponse, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;
    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /************************************************************************
     *  IHXViewSourceURLResponse
     */
    STDMETHOD(ViewSourceURLReady)	(THIS_	
					const char* /*out*/ pUrl) PURE;
};

// $Private:
/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXClientViewSource
 * 
 *  IID_IHXClientViewSource:
 * 
 *	{00003502-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXClientViewSource, 0x00003502, 0x901, 0x11d1, 0x8b, 0x6,
	    0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXClientViewSource

DECLARE_INTERFACE_(IHXClientViewSource, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;
    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /************************************************************************
     *  IHXClientViewSource
     */
    STDMETHOD(DoViewSource)		(THIS_
					IUnknown*		      /*IN*/ pPlayerContext,
					IHXStreamSource*	      /*IN*/ pSource) PURE;
    STDMETHOD_(HXBOOL, CanViewSource)	(THIS_
					IHXStreamSource*	      /*IN*/ pSource) PURE;

    STDMETHOD(GetViewSourceURL)		(THIS_
					IUnknown*                        pPlayerContext,
					IHXStreamSource*                pSource,
					IHXViewSourceURLResponse*       pResp) PURE;

};

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXClientViewSourceSink
 * 
 *  IID_IHXClientViewSourceSink:
 * 
 *	{00003503-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXClientViewSourceSink, 0x00003503, 0x901, 0x11d1, 0x8b, 0x6,
	    0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXClientViewSourceSink

DECLARE_INTERFACE_(IHXClientViewSourceSink, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;
    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /************************************************************************
     *  IHXClientViewSourceSink
     */
    STDMETHOD(RegisterViewSourceHdlr)	(THIS_	
					IHXClientViewSource*	/*in*/	pViewSourceHdlr) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXClientViewRights
 * 
 *  IID_IHXClientViewRights:
 * 
 *	{00003506-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXClientViewRights, 0x00003506, 0x901, 0x11d1, 0x8b, 0x6,
	    0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXClientViewRights

DECLARE_INTERFACE_(IHXClientViewRights, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;
    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /************************************************************************
     *  IHXClientViewRights
     */
    STDMETHOD(ViewRights)		(THIS_
					IUnknown*		      /*IN*/ pPlayerContext) PURE;
    STDMETHOD_(HXBOOL, CanViewRights)	(THIS) PURE;

};
// $EndPrivate.

#endif

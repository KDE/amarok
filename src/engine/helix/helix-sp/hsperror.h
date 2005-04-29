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
 *
 */

#ifndef _HSPERROR_
#define _HSPERROR_

struct IUnknown;
struct IHXErrorMessages;
struct IHXPlayer;
class HelixSimplePlayer;

class HSPErrorSink : public IHXErrorSink
{
public:

    HSPErrorSink(IUnknown* pUnknown, HelixSimplePlayer *pSplay);
    virtual ~HSPErrorSink();

    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    /*
     *  IHXErrorSink methods
     */

    /************************************************************************
     *	Method:
     *	    IHXErrorSink::ErrorOccurred
     *	Purpose:
     *	    After you have registered your error sink with an IHXErrorSinkControl 
     *	    (either in the server or player core) this method will be called to 
     *	    report an error, event, or status message.
     *
     *	    The meaning of the arguments is exactly as described in
     *	    hxerror.h
     */
    STDMETHOD(ErrorOccurred)	(THIS_
				const UINT8	unSeverity,  
				const ULONG32	ulHXCode,
				const ULONG32	ulUserCode,
				const char*	pUserString,
				const char*	pMoreInfoURL
				);

protected:
   LONG32 m_lRefCount;
   IHXPlayer* m_pPlayer;
   HelixSimplePlayer *m_splayer;

    void   ConvertErrorToString (const ULONG32 ulHXCode, char* pszBuffer, UINT32 ulBufLen);
};
#endif

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
#include "hxcomm.h"
#include "hxerror.h"
#include "hxcore.h"
#include "hxbuffer.h"

#include "hsperror.h"

#include <stdio.h>
#include "print.h"

#include "hxausvc.h"
#include "helix-sp.h"
#include "utils.h"

HSPErrorSink::HSPErrorSink(IUnknown* pUnknown, HelixSimplePlayer *pSplay) 
    : m_lRefCount(0),
      m_pPlayer(NULL),
      m_splayer(pSplay)
{
    IHXClientEngine* pEngine = NULL;
    pUnknown->QueryInterface(IID_IHXClientEngine, (void**)&pEngine );
    if( pEngine )
    {
        IUnknown* pTmp = NULL;
        pEngine->GetPlayer(0, pTmp);
        m_pPlayer = (IHXPlayer*)pTmp;
    }
    
    HX_RELEASE( pEngine );
    HX_ASSERT(m_pPlayer);
}

HSPErrorSink::~HSPErrorSink()
{
    HX_RELEASE(m_pPlayer);
}

// *** IUnknown methods ***

/////////////////////////////////////////////////////////////////////////
//  Method:
//  IUnknown::QueryInterface
//  Purpose:
//  Implement this to export the interfaces supported by your 
//  object.
//
STDMETHODIMP HSPErrorSink::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXErrorSink*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXErrorSink))
    {
        AddRef();
        *ppvObj = (IHXErrorSink*) this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//  IUnknown::AddRef
//  Purpose:
//  Everyone usually implements this the same... feel free to use
//  this implementation.
//
STDMETHODIMP_(ULONG32) HSPErrorSink::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//  IUnknown::Release
//  Purpose:
//  Everyone usually implements this the same... feel free to use
//  this implementation.
//
STDMETHODIMP_(ULONG32) HSPErrorSink::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

/*
 *  IHXErrorSink methods
 */

STDMETHODIMP 
HSPErrorSink::ErrorOccurred(const UINT8 unSeverity,  
                            const ULONG32   ulHXCode,
                            const ULONG32   ulUserCode,
                            const char* pUserString,
                            const char* pMoreInfoURL
                            )
{
    char HXDefine[256]; /* Flawfinder: ignore */

    // Store the code, so we can return it from main()
    m_splayer->m_Error = ulHXCode;

    ConvertErrorToString(ulHXCode, HXDefine, 256);

    m_splayer->STDOUT("Report(%d, %ld, \"%s\", %ld, \"%s\", \"%s\")\n",
                      unSeverity,
                      ulHXCode,
                      (pUserString && *pUserString) ? pUserString : "(NULL)",
                      ulUserCode,
                      (pMoreInfoURL && *pMoreInfoURL) ? pMoreInfoURL : "(NULL)",
                      HXDefine);

    return HXR_OK;
}

void
HSPErrorSink::ConvertErrorToString(const ULONG32 ulHXCode, char* pszBuffer, UINT32 ulBufLen)
{
    IHXErrorMessages* pErrMsg = NULL;

    if( !pszBuffer)
        return;
    
    pszBuffer[0]='\0';


    HX_ASSERT(m_pPlayer);
    if( m_pPlayer)
    {
        m_pPlayer->QueryInterface(IID_IHXErrorMessages, (void**)&pErrMsg);
        if( pErrMsg )
        {
            IHXBuffer* pMessage = pErrMsg->GetErrorText(ulHXCode);
            if( pMessage )
            {
                SafeStrCpy( pszBuffer, (const char*)pMessage->GetBuffer(), (int)ulBufLen);
                pMessage->Release();
            }
        }
    }
 
    HX_RELEASE(pErrMsg);
 
    if( strlen(pszBuffer)==0 )
    {
        SafeSprintf( pszBuffer, (int) ulBufLen, "Can't convert error code %p - please find corresponding HXR code in common/include/hxresult.h", ulHXCode );
    }

}


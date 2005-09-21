
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
 *
 */
#include "hxcomm.h"
#include "hxcore.h"
#include "hxbuffer.h"
#include "hxmangle.h"

#include "hxclsnk.h"
#include "hxerror.h"
#include "hxprefs.h"

#include "hspadvisesink.h"
#include "hsperror.h"
#include "hspauthmgr.h"
#include "hspcontext.h"

#include "hxausvc.h"
#include "helix-sp.h"
#include "utils.h"
#include "print.h"

extern BOOL bEnableAdviceSink;

HSPEngineContext::HSPEngineContext(IHXCommonClassFactory *pCommonClassFactory) : m_lRefCount(0), m_CommonClassFactory(pCommonClassFactory)
{
}

HSPEngineContext::~HSPEngineContext()
{
    Close();
}

void HSPEngineContext::Close()
{
   // you dont own the common class factory, so dont even think about it...
}

// *** IUnknown methods ***

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::QueryInterface
//  Purpose:
//	Implement this to export the interfaces supported by your 
//	object.
//
STDMETHODIMP HSPEngineContext::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPreferences))
    {
	AddRef();
	*ppvObj = (IHXPreferences*)this;
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
STDMETHODIMP_(ULONG32) HSPEngineContext::AddRef()
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
STDMETHODIMP_(ULONG32) HSPEngineContext::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}


// *** IHXPreference methods ***
void HSPEngineContext::Init(IUnknown* /*pUnknown*/)
{
   // nothing to do yet...
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IHXPreferences::ReadPref
//  Purpose:
//	Read a Preference from the registry.
//
STDMETHODIMP
HSPEngineContext::ReadPref(const char* pref_key, IHXBuffer*& buffer)
{
    HX_RESULT hResult	= HXR_OK;
    
    //m_splayer->STDERR("in engine context, key is <%s>\n", pref_key);
    if ((stricmp(pref_key, "OpenAudioDeviceOnPlayback") == 0))
    {
       unsigned char *outbuf;
       IHXBuffer *ibuf;

       m_CommonClassFactory->CreateInstance(CLSID_IHXBuffer, (void **) &ibuf);
       if (ibuf)
       {
          ibuf->SetSize(2);
          outbuf = ibuf->GetBuffer();
          strcpy((char *)outbuf, "0");
          buffer = ibuf;

          //m_splayer->STDERR("value = %d\n",atol((const char*) buffer->GetBuffer()));
       }
    }
    else
    {
	hResult = HXR_NOTIMPL;
    }

    return hResult;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IHXPreferences::WritePref
//  Purpose:
//	Write a Preference to the registry.
//
STDMETHODIMP
HSPEngineContext::WritePref(const char* /*pref_key*/, IHXBuffer* /*buffer*/)
{
   //m_splayer->STDERR("In EngineContext, WritePref, key %s\n", pref_key);
   return HXR_OK; // for now, no one allowed to change it
}



HSPClientContext::HSPClientContext(LONG32 lClientIndex, HelixSimplePlayer *pSplay)
    : m_lRefCount(0)
    , m_lClientIndex(lClientIndex)
    , m_pClientSink(NULL)
    , m_pErrorSink(NULL)
    , m_pAuthMgr(NULL)
    , m_pDefaultPrefs(NULL)
    , m_splayer(pSplay)
{
}


HSPClientContext::~HSPClientContext()
{
    Close();
}

void HSPClientContext::Init(IUnknown*	    pUnknown,
                            IHXPreferences* pPreferences,
                            char*	    pszGUID)
{
   //char* pszCipher = NULL;

	
    m_pClientSink	= new HSPClientAdviceSink(pUnknown, m_lClientIndex, m_splayer);
    m_pErrorSink	= new HSPErrorSink(pUnknown, m_splayer);
    m_pAuthMgr          = new HSPAuthenticationManager(m_splayer);

    if (m_pClientSink)
    {
	m_pClientSink->AddRef();
    }
    
    if (m_pErrorSink)
    {
	m_pErrorSink->AddRef();
    }

    if(m_pAuthMgr)
    {
	m_pAuthMgr->AddRef();
    }

    if (pPreferences)
    {
	m_pDefaultPrefs = pPreferences;
	m_pDefaultPrefs->AddRef();
    }

    if (pszGUID && *pszGUID)
    {
	// Encode GUID
       // TODO: find/implement Cipher
	//pszCipher = Cipher(pszGUID);
	//SafeStrCpy(m_pszGUID,  pszCipher, 256);
    }
    else
    {
	m_pszGUID[0] = '\0';
    }
}

void HSPClientContext::Close()
{
    HX_RELEASE(m_pClientSink);
    HX_RELEASE(m_pErrorSink);
    HX_RELEASE(m_pAuthMgr);
    HX_RELEASE(m_pDefaultPrefs);
}



// *** IUnknown methods ***

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::QueryInterface
//  Purpose:
//	Implement this to export the interfaces supported by your 
//	object.
//
STDMETHODIMP HSPClientContext::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPreferences))
    {
	AddRef();
	*ppvObj = (IHXPreferences*)this;
	return HXR_OK;
    }
    else if (m_pClientSink && 
	     m_pClientSink->QueryInterface(riid, ppvObj) == HXR_OK)
    {
	return HXR_OK;
    }
    else if (m_pErrorSink && 
	     m_pErrorSink->QueryInterface(riid, ppvObj) == HXR_OK)
    {
	return HXR_OK;
    }
    else if(m_pAuthMgr &&
	    m_pAuthMgr->QueryInterface(riid, ppvObj) == HXR_OK)
    {
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
STDMETHODIMP_(ULONG32) HSPClientContext::AddRef()
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
STDMETHODIMP_(ULONG32) HSPClientContext::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}


// *** IHXPreference methods ***

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IHXPreferences::ReadPref
//  Purpose:
//	Read a Preference from the registry.
//
STDMETHODIMP
HSPClientContext::ReadPref(const char* pref_key, IHXBuffer*& buffer)
{
    HX_RESULT hResult	= HXR_OK;
    //char*     pszCipher = NULL;
    
    if ((stricmp(pref_key, CLIENT_GUID_REGNAME) == 0) &&
	(*m_pszGUID))
    {
	// Create a Buffer 
       //TODO: Implement an IHXBuffer

//       buffer = new CHXBuffer();
//       buffer->AddRef();

       // Copy the encoded GUID into the buffer
//       buffer->Set((UCHAR*)m_pszGUID, strlen(m_pszGUID) + 1);
    }
    else if (m_pDefaultPrefs)
    {
	hResult = m_pDefaultPrefs->ReadPref(pref_key, buffer);
    }
    else
    {
	hResult = HXR_NOTIMPL;
    }

    return hResult;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IHXPreferences::WritePref
//  Purpose:
//	Write a Preference to the registry.
//
STDMETHODIMP
HSPClientContext::WritePref(const char* pref_key, IHXBuffer* buffer)
{
    if (m_pDefaultPrefs)
    {
	return m_pDefaultPrefs->WritePref(pref_key, buffer);
    }
    else	
    {
	return HXR_OK;
    }
}



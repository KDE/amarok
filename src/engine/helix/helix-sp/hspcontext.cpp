
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

extern BOOL bEnableAdviceSink;

HSPEngineContext::HSPEngineContext(HelixSimplePlayer *splayer, IHXCommonClassFactory *pCommonClassFactory) : m_lRefCount(0), m_CommonClassFactory(pCommonClassFactory), m_splayer(splayer)
{
}

HSPEngineContext::~HSPEngineContext()
{
    Close();
}

void HSPEngineContext::Close()
{
   // you don't own the common class factory, so don't even think about it...
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
    unsigned char *outbuf;
    IHXBuffer *ibuf;


    m_splayer->print2stderr("in engine context, key is <%s>\n", pref_key);
    if (0 == (stricmp(pref_key, "OpenAudioDeviceOnPlayback")))
    {
       m_CommonClassFactory->CreateInstance(CLSID_IHXBuffer, (void **) &ibuf);
       if (ibuf)
       {
          ibuf->SetSize(2);
          outbuf = ibuf->GetBuffer();
          strcpy((char *)outbuf, "0");
          buffer = ibuf;
          //m_splayer->print2stderr("value = %d\n",atol((const char*) buffer->GetBuffer()));
       }
    }
    else if (0 == (stricmp(pref_key, "SoundDriver")))
    {
       m_CommonClassFactory->CreateInstance(CLSID_IHXBuffer, (void **) &ibuf);
       if (ibuf)
       {
          ibuf->SetSize(2);
          outbuf = ibuf->GetBuffer();

          // 0 = OSS
          // 1 = OldOSSsupport
          // 2 = ESound
          // 3 = Alsa
          // 4 = USound

          if (m_splayer->getOutputSink() == HelixSimplePlayer::ALSA)
             strcpy((char *)outbuf, "3"); // set SoundDriver = kALSA (ie 3) for Alsa native support
          else if (m_splayer->getOutputSink() == HelixSimplePlayer::OSS)
             strcpy((char *)outbuf, "0"); // set SoundDriver = kOSS (ie 0) for OSS
          buffer = ibuf;

          if (m_splayer->getOutputSink() == HelixSimplePlayer::ALSA || m_splayer->getOutputSink() == HelixSimplePlayer::OSS)
             m_splayer->print2stderr("Setting Sound System to %s\n", m_splayer->getOutputSink() == HelixSimplePlayer::ALSA ? "ALSA" : "OSS");
          else
             m_splayer->print2stderr("Setting Sound System to UNKNOWN: %d\n", m_splayer->getOutputSink());
       }
    }
    // maybe also need to allow setting of "AlsaMixerDeviceName"?
    else if (0 == (stricmp(pref_key, "AlsaMixerElementName")))
    {
       m_splayer->setAlsaCapableCore(); // this just lets everyone know that this helix core is Alsa-capable
       m_CommonClassFactory->CreateInstance(CLSID_IHXBuffer, (void **) &ibuf);
       if (ibuf)
       {
          ibuf->SetSize(11);
          outbuf = ibuf->GetBuffer();
          strcpy((char *)outbuf, "PC Speaker");
          buffer = ibuf;
          m_splayer->print2stderr("Setting Mixer Element to use default mixer\n");
       }
    }
    else if (0 == (stricmp(pref_key, "AlsaMixerDeviceName")))
    {
       m_splayer->setAlsaCapableCore(); // this just lets everyone know that this helix core is Alsa-capable
       m_CommonClassFactory->CreateInstance(CLSID_IHXBuffer, (void **) &ibuf);
       if (ibuf)
       {
          ibuf->SetSize(8);
          outbuf = ibuf->GetBuffer();
          strcpy((char *)outbuf, "default");
          buffer = ibuf;
          m_splayer->print2stderr("Setting Mixer Device to use the \"default\" mixer\n");
       }
    }
    else if (0 == (stricmp(pref_key, "AlsaPCMDeviceName")))
    {
       m_splayer->setAlsaCapableCore(); // this just lets everyone know that this helix core is Alsa-capable
       m_CommonClassFactory->CreateInstance(CLSID_IHXBuffer, (void **) &ibuf);
       if (ibuf)
       {
          int len = strlen(m_splayer->getDevice());
          m_splayer->print2stderr("Setting Sound Device to \"%s\", %d\n", m_splayer->getDevice(), len);
          ibuf->SetSize(len + 1);
          outbuf = ibuf->GetBuffer();
          strcpy((char *)outbuf, m_splayer->getDevice());
          buffer = ibuf;
          m_splayer->print2stderr("Setting Sound Device to \"%s\"\n", m_splayer->getDevice());
       }
    }
    else if (0 == (stricmp(pref_key, "ThreadedAudio")))
    {
       m_CommonClassFactory->CreateInstance(CLSID_IHXBuffer, (void **) &ibuf);
       if (ibuf)
       {
          ibuf->SetSize(2);
          outbuf = ibuf->GetBuffer();
          strcpy((char *)outbuf, "1");
          buffer = ibuf;

          m_splayer->print2stderr("setting ThreadedAudio to value = %ld\n",atol((const char*) buffer->GetBuffer()));
       }
    }
    else if (0 == (stricmp(pref_key, "UseCoreThread")))
    {
       m_CommonClassFactory->CreateInstance(CLSID_IHXBuffer, (void **) &ibuf);
       if (ibuf)
       {
          ibuf->SetSize(2);
          outbuf = ibuf->GetBuffer();
          strcpy((char *)outbuf, "1");
          buffer = ibuf;

          m_splayer->print2stderr("setting initial UseCoreThread to value = %ld\n",atol((const char*) buffer->GetBuffer()));
       }
    }
    else if (0 == (stricmp(pref_key, "NetworkThreading")))
    {
       m_CommonClassFactory->CreateInstance(CLSID_IHXBuffer, (void **) &ibuf);
       if (ibuf)
       {
          ibuf->SetSize(2);
          outbuf = ibuf->GetBuffer();
          strcpy((char *)outbuf, "1");
          buffer = ibuf;

          m_splayer->print2stderr("setting initial NetworkTheading to value = %ld\n",atol((const char*) buffer->GetBuffer()));
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
   //m_splayer->print2stderr("In EngineContext, WritePref, key %s\n", pref_key);
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



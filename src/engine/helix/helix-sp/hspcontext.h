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
#ifndef _HSPCONTEXT_
#define _HSPCONTEXT_

struct IUnknown;
struct IHXPreferences;
struct IHXVolume;
class IHXCommonClassFactory;
class HSPClientAdviceSink;
class HSPErrorMessages;
class HSPAuthenticationManager;
class HelixSimplePlayer;

class HSPEngineContext : public IHXPreferences
{
public:
   HSPEngineContext(HelixSimplePlayer *splayer, IHXCommonClassFactory *pCommonClassFactory);
   virtual ~HSPEngineContext();
   void Init(IUnknown*	       /*IN*/ pUnknown);
   void Close();
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);
    STDMETHOD_(ULONG32,Release)	(THIS);

    /*
     * IHXPreferences methods
     */
    STDMETHOD(ReadPref)		(THIS_ const char* pref_key, 
				 IHXBuffer*& buffer);
    STDMETHOD(WritePref)	(THIS_ const char* pref_key,
				 IHXBuffer* buffer);

private:
   LONG32			 m_lRefCount;
   IHXCommonClassFactory        *m_CommonClassFactory;
   HelixSimplePlayer            *m_splayer;
};

class HSPClientContext : public IHXPreferences
{
private:
    LONG32			    m_lRefCount;
    LONG32                          m_lClientIndex;

    HSPClientAdviceSink*	    m_pClientSink;
    HSPErrorSink*		    m_pErrorSink;
    HSPAuthenticationManager*       m_pAuthMgr;
    IHXPreferences*		    m_pDefaultPrefs;
    char			    m_pszGUID[256];
    HelixSimplePlayer              *m_splayer;

public:

    HSPClientContext(LONG32 lClientIndex, HelixSimplePlayer *pSplay);
    virtual ~HSPClientContext();

    unsigned long position() { return m_pClientSink ? m_pClientSink->position() : 0; }
    unsigned long duration() { return m_pClientSink ? m_pClientSink->duration() : 0; }

    void Init(IUnknown*	       /*IN*/ pUnknown,
	      IHXPreferences* /*IN*/ pPreferences,
	      char*	       /*IN*/ pszGUID);
    void Close();

    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);
    STDMETHOD_(ULONG32,Release)	(THIS);

    /*
     * IHXPreferences methods
     */
    STDMETHOD(ReadPref)		(THIS_ const char* pref_key, 
				 IHXBuffer*& buffer);
    STDMETHOD(WritePref)	(THIS_ const char* pref_key,
				 IHXBuffer* buffer);
};

#endif

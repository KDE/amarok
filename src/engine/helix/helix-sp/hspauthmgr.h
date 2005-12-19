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

#ifndef _HSPAUTHMGR_H_
#define _HSPAUTHMGR_H_

#include "hxauth.h"

class HelixSimplePlayer;

class HSPAuthenticationManager : public IHXAuthenticationManager
{
private:
    INT32 m_lRefCount;
    BOOL  m_bSentPassword;
    HelixSimplePlayer *m_splayer;

public:
    HSPAuthenticationManager(HelixSimplePlayer *pSplay);
    virtual ~HSPAuthenticationManager();
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(UINT32,AddRef) (THIS);
    STDMETHOD_(UINT32,Release) (THIS);

    STDMETHOD(HandleAuthenticationRequest) (IHXAuthenticationManagerResponse* pResponse);
};
#endif

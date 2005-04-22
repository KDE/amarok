
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
#include "hxcom.h"
#include "hxauth.h"
#include "hspauthmgr.h"
#include <ctype.h>
#include "print.h"


#include "globals.h"
#include "hxausvc.h"
#include "helix-sp.h"
#include "utils.h"


HSPAuthenticationManager::HSPAuthenticationManager() :
    m_lRefCount(0),
    m_bSentPassword(FALSE)
{
}

HSPAuthenticationManager::~HSPAuthenticationManager()
{
}

STDMETHODIMP
HSPAuthenticationManager::QueryInterface(REFIID riid, void**ppvObj)
{
    if(IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXAuthenticationManager*)this;
        return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXAuthenticationManager))
    {
        AddRef();
        *ppvObj = (IHXAuthenticationManager*)this;
        return HXR_OK;
    }
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(UINT32)
HSPAuthenticationManager::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(UINT32)
HSPAuthenticationManager::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP
HSPAuthenticationManager::HandleAuthenticationRequest(IHXAuthenticationManagerResponse* pResponse)
{
    char      username[1024] = ""; /* Flawfinder: ignore */
    char      password[1024] = ""; /* Flawfinder: ignore */
    HX_RESULT res = HXR_FAIL;
    
    if( !m_bSentPassword )
    {
        res = HXR_OK;
        if (HelixSimplePlayer::GetGlobal()->bEnableVerboseMode)
            STDOUT("\nSending Username and Password...\n");

        SafeStrCpy(username,  HelixSimplePlayer::GetGlobal()->g_pszUsername, 1024);
        SafeStrCpy(password,  HelixSimplePlayer::GetGlobal()->g_pszPassword, 1024);

        //strip trailing whitespace
        char* c;
        for(c = username + strlen(username) - 1; 
            c > username && isspace(*c);
            c--)
            ;
        *(c+1) = 0;
    
        for(c = password + strlen(password) - 1; 
            c > password && isspace(*c);
            c--)
            ;
        *(c+1) = 0;
        
        m_bSentPassword = TRUE;
    }

    if (HelixSimplePlayer::GetGlobal()->bEnableVerboseMode && FAILED(res) )
        STDOUT("\nInvalid Username and/or Password.\n");
    
    pResponse->AuthenticationRequestDone(res, username, password);
    return res;
}


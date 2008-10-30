/***************************************************************************
 *   Copyright 2005-2008 Last.fm Ltd.                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

// see http://msdn.microsoft.com/en-us/magazine/cc301850.aspx for
// more about Ndis and wmi and getting these events

// Link to wbemuuid.lib to resolve IWbemObjectSink and IWbemClassObject
// interface definitions.


// This brings in CoInitializeSecurity from objbase.h
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif

#include "NdisEvents_win.h"
#include "WmiSink_win.h"
#include <atlcom.h>
#include <objbase.h>
#include <crtdbg.h>

NdisEvents::NdisEvents()
    : m_pSink(0)
{}

NdisEvents::~NdisEvents()
{
    if (m_pSink)
	    m_pSink->disconnect();
	if (m_pServices && m_pSink)
		m_pServices->CancelAsyncCall(m_pSink);
	// and reference counting will take care of the WmiSink object
}

HRESULT
NdisEvents::registerForNdisEvents()
{
	HRESULT hr;

	CSecurityDescriptor sd;
	sd.InitializeFromThreadToken();
	hr = CoInitializeSecurity(sd, -1, NULL, NULL,
		RPC_C_AUTHN_LEVEL_PKT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
	_ASSERT(SUCCEEDED(hr));
	if (FAILED(hr))
		return hr;

    hr = m_pLocator.CoCreateInstance(CLSID_WbemLocator);
	if (FAILED(hr))
		return hr;

    // Connect to the root\wmi namespace with the current user.
    hr = m_pLocator->ConnectServer(CComBSTR("ROOT\\WMI"),		// strNetworkResource
									NULL,				// strUser
									NULL,				// strPassword
									NULL,				// strLocale  
									0,					// lSecurityFlags
									CComBSTR(""),       // strAuthority               
									NULL,				// pCtx
									&m_pServices
									);
    if (FAILED(hr))
		return hr;

	m_pSink = new WmiSink(this);

	//////////////////////////

	// other notifications we're not interested in right now include...
	// MSNdis_NotifyAdapterArrival  \DEVICE\<guid>
	// MSNdis_NotifyAdapterRemoval
	// MSNdis_StatusLinkSpeedChange
	// MSNdis_NotifyVcArrival
	// MSNdis_NotifyVcRemoval
	// MSNdis_StatusResetStart
	// MSNdis_StatusResetEnd
	// MSNdis_StatusProtocolBind
	// MSNdis_StatusProtocolUnbind
	// MSNdis_StatusMediaSpecificIndication

	CComBSTR wql("WQL");
	CComBSTR query("SELECT * FROM MSNdis_StatusMediaDisconnect");
	hr = m_pServices->ExecNotificationQueryAsync(wql, query, 0, 0, m_pSink);

	query = "SELECT * FROM MSNdis_StatusMediaConnect";
	hr = m_pServices->ExecNotificationQueryAsync(wql, query, 0, 0, m_pSink);

    return S_OK;
}


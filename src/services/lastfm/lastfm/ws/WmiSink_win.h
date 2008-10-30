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

#ifndef WMISINK_WIN_H
#define WMISINK_WIN_H

#include "WbemCli.h"

// Sink object for WMI NDIS notifications
class WmiSink : public IWbemObjectSink
{
    UINT m_cRef;

public:
	WmiSink(class NdisEvents *callback);
	~WmiSink();

    // IUnknown members
    STDMETHODIMP         QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

	// IWbemObjectSink
    STDMETHODIMP Indicate(long, IWbemClassObject**);
    STDMETHODIMP SetStatus(long, HRESULT, BSTR, IWbemClassObject *);

	void disconnect();

private:
	class NdisEvents *m_callback;
};

#endif
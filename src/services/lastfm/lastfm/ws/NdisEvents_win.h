/***************************************************************************
 *   Copyright 2005-2008 Last.fm Ltd.                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that itw ill be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef NDIS_EVENTS_H
#define NDIS_EVENTS_H

#include <windows.h>
#include <atlbase.h>
#include <WbemCli.h>

class NdisEvents
{
public:
    NdisEvents();
    ~NdisEvents();
    HRESULT registerForNdisEvents();

	virtual void onConnectionUp(BSTR name) = 0;
	virtual void onConnectionDown(BSTR name) = 0;

private:
    CComPtr<IWbemLocator> m_pLocator;
    CComPtr<IWbemServices> m_pServices;
    class WmiSink *m_pSink;
};

#endif


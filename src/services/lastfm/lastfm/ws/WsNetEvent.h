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

#ifndef WS_NET_EVENTS_H
#define WS_NET_EVENTS_H

#include "lastfm/DllExportMacro.h"
#include <QObject>


#ifdef WIN32
	#include "NdisEvents_win.h"

	class WsNetEventAdapter : public QObject, public NdisEvents
	{
		Q_OBJECT;

		// WmiSink callbacks:
		virtual void onConnectionUp(BSTR name)
		{
			emit connectionUp(QString::fromUtf16(name));
		}

		virtual void onConnectionDown(BSTR name)
		{
			emit connectionDown(QString::fromUtf16(name));
		}

	public:
		WsNetEventAdapter(QObject *parent) : QObject(parent)
		{
			registerForNdisEvents();
		}

	signals:
		void connectionUp(QString connectionName);
		void connectionDown(QString connectionName);
	};

#else
	class WsNetEventAdapter : public QObject
	{
		Q_OBJECT

	public:
		WsNetEventAdapter(QObject *parent)
			:QObject(parent)
		{}

	signals:
		void connectionUp(QString connectionName);
		void connectionDown(QString connectionName);
	};
#endif


class LASTFM_WS_DLLEXPORT WsNetEvent : public QObject
{
	Q_OBJECT

	WsNetEventAdapter *m_adapter;

public:
	WsNetEvent(QObject *parent = 0);

signals:
	void connectionUp(QString connectionName);
	void connectionDown(QString connectionName);
};

#endif

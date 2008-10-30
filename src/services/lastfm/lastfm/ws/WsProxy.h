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

#ifndef WS_PROXY_H
#define WS_PROXY_H

#include <QCoreApplication>
#include <QNetworkProxy>
#include "WsAutoProxy.h"

#ifdef WIN32
#include <windows.h>
#include <winhttp.h>


/** @brief memory managing wrapper for WINHTTP_CURRENT_USER_IE_PROXY_CONFIG
  * @author <doug@last.fm>
  */

class IeSettings : public WINHTTP_CURRENT_USER_IE_PROXY_CONFIG
{
public:
	IeSettings();
	~IeSettings();
};


/** @brief opens Internet Options control panel applet to Connections tab
* @author <doug@last.fm>
*
*/

class Win_SettingsWindow
{
	HMODULE m_hMod;
	BOOL (WINAPI *m_proc)(HWND);

public:
	Win_SettingsWindow();
	~Win_SettingsWindow();
	bool open(HWND parent);
};

#endif


/** @brief useful proxy functions
  * @author <doug@last.fm>
  */
class WsProxy : QObject
{
	Q_OBJECT

#ifdef WIN32
	Win_SettingsWindow m_settingsWindow;
	WsAutoProxy m_autoProxy;
#endif

public:
	WsProxy(QObject *parent = 0);

	bool getProxyFor(const QString &url, const QByteArray &userAgent, QNetworkProxy &out);
	void openSettingsWindow();
};

#endif

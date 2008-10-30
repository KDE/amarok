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

#include <QtGui/QApplication>
#include <QtGui/QWidget>
#include <QtGui/QDesktopWidget>
#include <QUrl>
#include "WsProxy.h"


#ifdef WIN32
Win_SettingsWindow::Win_SettingsWindow()
	:m_hMod(0)
	,m_proc(0)
{
}

Win_SettingsWindow::~Win_SettingsWindow()
{
	if (m_hMod)
		FreeLibrary(m_hMod);
}

bool 
Win_SettingsWindow::open(HWND parent)
{
	if (!m_hMod && (m_hMod = LoadLibraryA("InetCpl.cpl")))
		m_proc = (BOOL (WINAPI *)(HWND)) GetProcAddress(m_hMod, "LaunchConnectionDialog");
	return m_proc ? m_proc(parent) : false;
}


///////////////////////////////////////


IeSettings::IeSettings()
{
	if (!WinHttpGetIEProxyConfigForCurrentUser(this)) {
		fAutoDetect = FALSE;
		lpszAutoConfigUrl =	lpszProxy = lpszProxyBypass = 0;
	}
}

IeSettings::~IeSettings()
{
	if (lpszAutoConfigUrl) GlobalFree(lpszAutoConfigUrl);
	if (lpszProxy) GlobalFree(lpszProxy);
	if (lpszProxyBypass) GlobalFree(lpszProxyBypass);
}


#endif


///////////////////////////////////////


WsProxy::WsProxy(QObject *parent)
:QObject(parent)
{
}


bool 
WsProxy::getProxyFor(const QString &url, const QByteArray &userAgent, QNetworkProxy &out)
{
#ifdef WIN32
	IeSettings s;
	if (s.fAutoDetect) {
		return m_autoProxy.getProxyFor(url, userAgent, out, QString::fromUtf16(s.lpszAutoConfigUrl));
	} else if (s.lpszProxy) {
		// manual proxy
		QUrl url(QString::fromUtf16(s.lpszProxy));
		out.setHostName(url.host());
		out.setPort(url.port());
		return true;
	}
#endif

	// direct
	return false;
}


void WsProxy::openSettingsWindow()
{
#ifdef WIN32
	QWidget *w = QApplication::desktop();
	m_settingsWindow.open(w ? w->winId() : NULL);
#endif
}
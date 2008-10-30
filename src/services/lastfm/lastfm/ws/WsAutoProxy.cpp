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

#include "WsAutoProxy.h"
#include "WsRequestBuilder.h"
#include <QNetworkProxy>
#include <QString>
#include <QUrl>

#ifdef WIN32
#include <AtlBase.h>
#include <AtlConv.h>
#endif


static bool
parsePacServer(const QString &s, QNetworkProxy &p)
{
	// remove optional leading "scheme=" portion
	int start = s.indexOf('=');
	QUrl url(s.mid(start+1), QUrl::TolerantMode);

	if (url.isValid())
	{
		p.setHostName(url.host());
		p.setPort(url.port());
		return true;
	}
	return false;
}


static QList<QNetworkProxy>
parsePacResult(const QString &pacResult)
{
	// msdn says: "The proxy server list contains one or more of the 
	// following strings separated by semicolons or whitespace."
	// ([<scheme>=][<scheme>"://"]<server>[":"<port>])

	QList<QNetworkProxy> result;
	QStringList proxies = pacResult.split(QRegExp("[\\s;]"), QString::SkipEmptyParts);
	foreach(const QString &s, proxies)
	{
		QNetworkProxy proxy;
		if (parsePacServer(s, proxy))
		{
			result << proxy;
		}
	}
	return result;
}


////////////////


WsAutoProxy::WsAutoProxy()
{
#ifdef WIN32
	m_hSession = 0;
#endif
}

WsAutoProxy::~WsAutoProxy()
{
#ifdef WIN32
	if (m_hSession)
		WinHttpCloseHandle(m_hSession);
#endif
}

bool
WsAutoProxy::getProxyFor(const QString &url, const QByteArray &userAgent, QNetworkProxy &out, const QString &pacUrl)
{
	bool result = false;
#ifdef WIN32
	if (!m_hSession)
	{
		m_hSession = WinHttpOpen(CA2W(userAgent), WINHTTP_ACCESS_TYPE_NO_PROXY, 0, 0, 0/*|WINHTTP_FLAG_ASYNC*/);
	}
	if (m_hSession)
	{
		WINHTTP_PROXY_INFO info;
		WINHTTP_AUTOPROXY_OPTIONS opts;
		memset(&opts, 0, sizeof(opts));
		if (pacUrl.length()) 
		{
			opts.dwFlags = WINHTTP_AUTOPROXY_CONFIG_URL;
			opts.lpszAutoConfigUrl = pacUrl.utf16();
		} 
		else
		{
			opts.dwFlags = WINHTTP_AUTOPROXY_AUTO_DETECT;
			opts.dwAutoDetectFlags = WINHTTP_AUTO_DETECT_TYPE_DHCP | WINHTTP_AUTO_DETECT_TYPE_DNS_A;
		}
		opts.fAutoLogonIfChallenged = TRUE;
		
		if (WinHttpGetProxyForUrl(m_hSession, url.utf16(), &opts, &info))
		{
			if (info.lpszProxy) 
			{
				QList<QNetworkProxy> proxies = parsePacResult(QString::fromUtf16(info.lpszProxy));
				if (!proxies.empty())
				{
					out = proxies.at(0);
					result = true;
				}
				GlobalFree(info.lpszProxy);
			}
			if (info.lpszProxyBypass)
			{
				GlobalFree(info.lpszProxyBypass);
			}
		}
	}
#elif defined(Q_WS_MAC)
	// todo
#elif defined(Q_WS_X11)
	// todo
#endif
	return result;
}
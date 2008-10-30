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

#include "WsAccessManager.h"
#include "WsKeys.h"
#include "WsProxy.h"
#include <QtNetwork>

WsProxy *WsAccessManager::m_proxy = 0;


WsAccessManager::WsAccessManager(QObject *parent)
               : QNetworkAccessManager(parent)
{
    // parented to qApplication as this is a application-lifetime object
	if (!m_proxy) m_proxy = new WsProxy( qApp );
    if (!Ws::UserAgent) Ws::UserAgent = qstrdup( QCoreApplication::applicationName().toAscii() );
}

void
WsAccessManager::applyProxy(const QNetworkRequest &request)
{
	if (m_proxy)
	{
		QNetworkProxy p;
		bool gotProxy = m_proxy->getProxyFor(
			request.url().toString(), 
			request.rawHeader("user-agent"), p);
		if (gotProxy)
			QNetworkAccessManager::setProxy(p);
	}
}

QNetworkReply *
WsAccessManager::monitor(QNetworkReply *reply)
{
	// todo: a convenient place to connect to all network replies
	return reply;
}

QNetworkReply *
WsAccessManager::head(const QNetworkRequest &request)
{
	applyProxy(request);
	return monitor(QNetworkAccessManager::head(request));
}

QNetworkReply *
WsAccessManager::get(const QNetworkRequest &request)
{
	applyProxy(request);
	return monitor(QNetworkAccessManager::get(request));
}

QNetworkReply *
WsAccessManager::post(const QNetworkRequest &request, QIODevice *data)
{
	applyProxy(request);
	return monitor(QNetworkAccessManager::post(request, data));
}

QNetworkReply *
WsAccessManager::post(const QNetworkRequest &request, const QByteArray &data)
{
	applyProxy(request);
	return monitor(QNetworkAccessManager::post(request, data));
}

QNetworkReply *
WsAccessManager::put(const QNetworkRequest &request, QIODevice *data)
{
	applyProxy(request);
	return monitor(QNetworkAccessManager::put(request, data));
}

QNetworkReply *
WsAccessManager::put(const QNetworkRequest &request, const QByteArray &data)
{
	applyProxy(request);
	return monitor(QNetworkAccessManager::put(request, data));
}

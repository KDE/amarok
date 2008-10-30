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

#ifndef LASTFM_WS_ACCESS_MANAGER_H
#define LASTFM_WS_ACCESS_MANAGER_H

#include <lastfm/DllExportMacro.h>
#include <QtNetwork/QNetworkAccessManager>


class LASTFM_WS_DLLEXPORT WsAccessManager : public QNetworkAccessManager
{
    Q_OBJECT

	static class WsProxy *m_proxy;

 	/** called for every request since we support PAC, it's worth noting that 
	  * this function calls QNetworkAccessManager::setProxy */
	void applyProxy(const QNetworkRequest &);
	QNetworkReply *monitor(QNetworkReply *);

public:
	WsAccessManager(QObject *parent = 0);

    QNetworkReply *head(const QNetworkRequest &request);
    QNetworkReply *get(const QNetworkRequest &request);
    QNetworkReply *post(const QNetworkRequest &request, QIODevice *data);
    QNetworkReply *post(const QNetworkRequest &request, const QByteArray &data);
    QNetworkReply *put(const QNetworkRequest &request, QIODevice *data);
    QNetworkReply *put(const QNetworkRequest &request, const QByteArray &data);
};

#endif

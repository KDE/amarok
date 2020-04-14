/****************************************************************************************
 * Copyright (c) 2006 Ian Monroe <ian@monroe.nu>                                        *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef DAAPCONTENTFETCHER_H
#define DAAPCONTENTFETCHER_H

#include <QByteArray>
#include <QObject>

class QIODevice;
class QNetworkReply;

namespace Daap {

/**
   Inspired by a daapsharp class of the same name. Basically it adds all the silly headers
   that DAAP needs
	@author Ian Monroe <ian@monroe.nu>
*/
class ContentFetcher : public QObject
{
    Q_OBJECT

    public:
        ContentFetcher( const QString & hostname, quint16 port, const QString& password, QObject * parent = nullptr, const char * name = 0 );
        ~ContentFetcher() override;

        void getDaap( const QString & command, QIODevice* musicFile = 0 );
        QByteArray results();

    private Q_SLOTS:
        void onFinished();

    Q_SIGNALS:
        void httpError( const QString& );
        void finished();
        void loginRequired();

    private:
        QNetworkReply *m_reply;
        QByteArray m_lastResult;
        QString m_hostname;
        quint16 m_port;
        QByteArray m_authorize;
        bool m_selfDestruct;
        static int s_requestId; //!< Apple needs this for some reason
};



}

#endif

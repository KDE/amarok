/****************************************************************************************
 * Copyright (c) 2010 Sergey Ivanov <123kash@gmail.com>                                 *
 * Copyright (c) 2013 Vedant Agarwala <vedant.kota@gmail.com>                           *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef WEBREQUESTSHANDLER_H
#define WEBREQUESTSHANDLER_H

#include "core/meta/forward_declarations.h"
#include "AudioToQStringDecoder.h"
#include "network/NetworkAccessManagerProxy.h"

#include <QTimer>

namespace TagGuessing
{
    class WebRequestsHandler : public QObject
    {
        Q_OBJECT
    public:
        WebRequestsHandler( QObject *parent,
                            const QString &host,
                            const int port,
                            const QString &pathPrefix,
                            const QString &clientId,
                            const QString &clientVersion);
        virtual ~WebRequestsHandler();
    public slots:
        virtual void run( const Meta::TrackList &tracks );

    signals:
        void trackFound( const Meta::TrackPtr &track, const QString &puid );
        void progressStep();
        void done();

    protected slots:
        virtual void sendNewRequest();
        virtual void gotReply( QNetworkReply *reply ) = 0;
        virtual void replyError( QNetworkReply::NetworkError code );
        virtual void trackDecoded( const Meta::TrackPtr track, const QString fingerprint );
        virtual void decodingDone( ThreadWeaver::Job *_decoder ); // TODO check viability of being a non-zero virtual
        virtual void parsingDone( ThreadWeaver::Job *_parser ) = 0;

    protected:
        virtual QNetworkRequest compileRequest( const QString &fingerprint, const Meta::TrackPtr track );
        virtual void checkDone() = 0;

        QString m_host;
        int m_port;
        QString m_pathPrefix;
        QString m_clientId;
        QString m_clientVersion;

        QNetworkAccessManager *net;
        QList < QPair < Meta::TrackPtr, QNetworkRequest > > m_requests;
        QMap < QNetworkReply *, Meta::TrackPtr > m_replyes;

        bool decodingComplete;

        QTimer *_timer;
    };
}

#endif // WEBREQUESTSHANDLER_H

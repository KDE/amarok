/****************************************************************************************
 * Copyright (c) 2010 Sergey Ivanov <123kash@gmail.com>                                 *
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

#ifndef MUSICDNSFINDER_H
#define MUSICDNSFINDER_H

#include "Version.h"
#include "core/meta/forward_declarations.h"
#include "musicbrainz/MusicDNSAudioDecoder.h"
#include "musicbrainz/MusicDNSXmlParser.h"
#include "network/NetworkAccessManagerProxy.h"

#include <QTimer>
#include <ThreadWeaver/Job>

#define AMAROK_MUSICDNS_CLIENT_ID QStringLiteral("0c6019606b1d8a54d0985e448f3603ca")

class MusicDNSFinder: public QObject
{
    Q_OBJECT
    public:
        explicit MusicDNSFinder( QObject *parent = nullptr,
                        const QString &host = QStringLiteral("ofa.musicdns.org"),
                        const int port = 80,
                        const QString &pathPrefix = QStringLiteral("/ofa/1"),
                        const QString &clietnId = AMAROK_MUSICDNS_CLIENT_ID,
                        const QString &clientVersion = QStringLiteral(AMAROK_VERSION)  );
    public Q_SLOTS:
        void run( const Meta::TrackList &tracks );

    Q_SIGNALS:
        void trackFound( const Meta::TrackPtr &track, const QString &puid );
        void progressStep();
        void done();

    private Q_SLOTS:

        void sendNewRequest();
        void gotReply( QNetworkReply *reply );
        void replyError( QNetworkReply::NetworkError code );

        void trackDecoded( const Meta::TrackPtr track, const QString fingerprint );
        void decodingDone( ThreadWeaver::JobPointer _decoder );

        void parsingDone( ThreadWeaver::JobPointer _parser );

    private:
        QNetworkRequest compileRequest( const QString &fingerprint, const Meta::TrackPtr track );
        void checkDone();

        QString mdns_host;
        int mdns_port;
        QString mdns_pathPrefix;
        QString mdns_clientId;
        QString mdns_clientVersion;

        QNetworkAccessManager *net;
        QList < QPair < Meta::TrackPtr, QNetworkRequest > > m_requests;
        QMap < QNetworkReply *, Meta::TrackPtr > m_replyes;

        QMap < MusicDNSXmlParser *, Meta::TrackPtr > m_parsers;

        bool decodingComplete;

        QTimer *_timer;
};

#endif // MUSICDNSFINDER_H

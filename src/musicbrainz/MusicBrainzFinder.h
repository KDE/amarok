/****************************************************************************************
 * Copyright (c) 2010 Sergey Ivanov <123kash@gmail.com>                                 *
 * Copyright (c) 2013 Alberto Villa <avilla@FreeBSD.org>                                *
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

#ifndef MUSICBRAINZFINDER_H
#define MUSICBRAINZFINDER_H

#include "Version.h"
#include "core/meta/forward_declarations.h"
#include "musicbrainz/MusicBrainzXmlParser.h"
#include "network/NetworkAccessManagerProxy.h"

typedef QPair<Meta::TrackPtr, QVariantMap> TrackInfo;

class MusicBrainzFinder : public QObject
{
    Q_OBJECT

    public:
        explicit MusicBrainzFinder( QObject *parent = nullptr,
                                    const QString &host = QStringLiteral("musicbrainz.org"),
                                    const int port = 80,
                                    const QString &pathPrefix = QStringLiteral("/ws/2"),
                                    const QString &username = QString(),
                                    const QString &password = QString() );

        bool isRunning() const;

    Q_SIGNALS:
        void progressStep();
        void trackFound( const Meta::TrackPtr track, const QVariantMap tags );
        void done();

    public Q_SLOTS:
        void run( const Meta::TrackList &tracks );

        void lookUpByPUID( const Meta::TrackPtr &track, const QString &puid );

    private Q_SLOTS:
        void sendNewRequest();
        void gotAuthenticationRequest( const QNetworkReply *reply, QAuthenticator *authenticator );
        void gotReplyError( QNetworkReply::NetworkError code );
        void gotReply( QNetworkReply *reply );

        void parsingDone( ThreadWeaver::JobPointer _parser );

    private:
        QVariantMap guessMetadata( const Meta::TrackPtr &track ) const;

        QNetworkRequest compileRequest( QUrl &url );
        QNetworkRequest compileTrackRequest( const Meta::TrackPtr &track );
        QNetworkRequest compilePUIDRequest( const QString &puid );
        QNetworkRequest compileReleaseGroupRequest( const QString &releaseGroupID );

        void sendTrack( const Meta::TrackPtr &track, QVariantMap tags );
        void checkDone();

        QTimer *m_timer;

        QString mb_host;
        int mb_port;
        QString mb_pathPrefix;
        QString mb_username;
        QString mb_password;

        QNetworkAccessManager *net;
        QList<QPair<Meta::TrackPtr, QNetworkRequest> > m_requests;
        QMap<QNetworkReply *, Meta::TrackPtr> m_replies;
        QMap<MusicBrainzXmlParser *, Meta::TrackPtr> m_parsers;

        QMap<Meta::TrackPtr, QVariantMap> m_parsedMetadata;

        QMap<QString, QVariantMap> mb_releaseGroups;
        QMap<QString, QList<TrackInfo> > mb_queuedTracks;
};

#endif // MUSICBRAINZFINDER_H

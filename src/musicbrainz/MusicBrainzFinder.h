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

#ifndef MUSICBRAINZFINDER_H
#define MUSICBRAINZFINDER_H

#include "core/meta/Meta.h"
#include "MusicBrainzMetaClasses.h"
#include "MusicBrainzXmlParser.h"
#include "NetworkAccessManagerProxy.h"
#include "shared/Version.h"

class MusicBrainzFinder : public QObject
{
    Q_OBJECT

    public:

        MusicBrainzFinder( QObject *parent = 0,  const QString &host = "musicbrainz.org",
                           const int port = 80, const QString &pathPrefix = "/ws/1",
                           const QString &username = QString(), const QString &password = QString() );

        ~MusicBrainzFinder();

        bool isRunning();

    signals:
        void progressStep();
        void trackFound( const Meta::TrackPtr track, const QVariantMap tags );

        void done();

    public slots:
        void run(  const Meta::TrackList &tracks );

        void lookUpByPUID( const Meta::TrackPtr &track, const QString &puid );

    private slots:

        void sendNewRequest();
        void gotReply( QNetworkReply *reply );
        void authenticationRequest( QNetworkReply *reply, QAuthenticator *authenticator );
        void replyError( QNetworkReply::NetworkError code );

        void parsingDone( ThreadWeaver::Job *_parser );

    private:
        QNetworkRequest compileRequest( const Meta::TrackPtr &track );
        QNetworkRequest compileReleaseRequest( const QString &releasId );
        QNetworkRequest compilePUIDRequest( const QString &puid );
        QNetworkRequest compileIDRequest( const QString &id );

        QMap< QString, QString > guessMetadata( const Meta::TrackPtr &track );

        void sendTrack( MusicBrainzTrack &track );

        QString mb_host;
        int mb_port;
        QString mb_pathPrefix;
        const QString mb_username;
        const QString mb_password;

        QMap < Meta::TrackPtr, QMap < QString, QString > > m_parsedmetadata;

        QNetworkAccessManager *net;
        QList < QPair < Meta::TrackPtr, QNetworkRequest > > m_requests;
        QMap < QNetworkReply *, Meta::TrackPtr > m_replyes;
        QMap < MusicBrainzXmlParser *, Meta::TrackPtr > m_parsers;

        MusicBrainzTrackList mb_tracks;
        MusicBrainzArtistList mb_artists;
        MusicBrainzReleaseList mb_releases;

        QTimer *_timer;
};

#endif // MUSICBRAINZFINDER_H

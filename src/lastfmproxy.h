/***************************************************************************
 * copyright            : (C) 2006 Chris Muehlhaeuser <chris@chris.de>     *
 * copyright            : (C) 2006 Seb Ruiz <me@sebruiz.net>               *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROK_LASTFMPROXY_H
#define AMAROK_LASTFMPROXY_H

#include "metabundle.h"

#include <qobject.h>
#include <qurl.h>
#include <qvaluelist.h>

class QHttp;
class QHttpResponseHeader;
class QTimer;

class LastFmProxy : public QObject
{
    Q_OBJECT

    public:
        enum DataType { Artist, Album, Track };

        LastFmProxy();

        void handshake( const QString& username, const QString& password );
        void changeStation( QString url );

        QString currentUsername() { return m_username;  }
        QString currentPassword() { return m_password;  }
        QString currentStation()  { return m_station;   }
        QString session()         { return m_session;   }
        QUrl    streamUrl()       { return m_streamUrl; }

        void friends( QString username );
        void neighbours( QString username );

        void recentTracks( QString username );
        void userTags( QString username );

        void recommend( int type, QString username, QString artist, QString token = QString() );

        void recommendArtist( QString username, QString artist )
        {    recommend( LastFmProxy::Artist, username, artist ); }

        void recommendAlbum( QString username, QString artist, QString album )
        {    recommend( LastFmProxy::Album, username, artist, album ); }

        void recommendTrack( QString username, QString artist, QString track )
        {    recommend( LastFmProxy::Track, username, artist, track ); }

        /**
            Verify with server that a supplied user/pass combo is valid. Password
            should be MD5 hashed.
         **/
        void verifyUser( const QString& user, const QString& pass );

    public slots:
        void requestMetaData();
        void enableScrobbling( bool enabled );

        void love();
        void skip();
        void ban();

    signals:
        void actionStarted();
        void actionFinished();

        void handshakeResult( int result );
        void streamingUrl( const QUrl& url );
        void stationChanged( QString url, QString name );
        void songQueued();

        void metaDataResult( const MetaBundle &bundle );
        void enableScrobblingDone();

        void loveDone();
        void skipDone();
        void banDone();

        void friendsResult( const QString& username, const QStringList& friends );
        void neighboursResult( const QString& username, const QStringList& friends );

        void recentTracksResult( const QString& username, QValueList< QPair<QString, QString> > songs );
        void userTagsResult( const QString& username, const QStringList& tags );

    private:
        QString m_username;
        QString m_password;
        QString m_station;
        QString m_session;
        QString m_baseHost;
        QString m_basePath;
        QUrl m_streamUrl;
        bool m_connected;
        bool m_subscriber;

        QHttp *m_lastHttp;

        QString parameter( QString keyName, QString data );
        QStringList parameterArray( QString keyName, QString data );
        QStringList parameterKeys( QString keyName, QString data );

    private slots:
        void handshakeHeaderReceived( const QHttpResponseHeader &resp );
        void handshakeFinished( int id, bool error );

        void changeStationFinished( int id, bool error );
        void metaDataFinished( int id, bool error );
        void enableScrobblingFinished( int id, bool error );

        void loveFinished( int id, bool error );
        void skipFinished( int id, bool error );
        void banFinished( int id, bool error );

        void friendsFinished( int id, bool error );
        void neighboursFinished( int id, bool error );

        void recentTracksFinished( int id, bool error );
        void userTagsFinished( int id, bool error );

        void recommendFinished( int id, bool error );
};

#endif /*AMAROK_LASTFMPROXY_H*/

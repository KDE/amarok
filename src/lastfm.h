/***************************************************************************
 * copyright            : (C) 2006 Chris Muehlhaeuser <chris@chris.de>     *
 *                      : (C) 2006 Seb Ruiz <me@sebruiz.net>               *
 *                      : (C) 2006 Ian Monroe <ian@monroe.nu>              *
 *                      : (C) 2006 Mark Kretschmann <markey@web.de>        *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROK_LASTFM_H
#define AMAROK_LASTFM_H

#include "metabundle.h"

#include <qobject.h>
#include <qserversocket.h>
#include <qurl.h>
#include <qvaluelist.h>

#include <kconfigdialog.h>

class KLineEdit;
class KAction;
class KProcIO;
class KURL;
class QSocket;
class QTimer;

namespace LastFm
{
    class WebService;

    class Controller : public QObject
    {
        Q_OBJECT

        public:
            static Controller* instance();

            KURL        getNewProxy( QString genreUrl );

            bool        isPlaying() const { return m_service != 0; }
            WebService* getService() const { return m_service; }
            QString     getGenreUrl() const { return m_genreUrl; }

            static bool checkCredentials();

        public slots:
            void playbackStopped();
            void ban();
            void love();
            void skip();

        private:
            Controller();
            void setActionsEnabled( bool enable );

            static Controller *s_instance;
            QPtrList<KAction> m_actionList;
            QString     m_genreUrl;
            WebService* m_service;
    };

    class WebService : public QObject
    {
        Q_OBJECT

        public:
            enum DataType { Artist, Album, Track };

            WebService( QObject* parent );
            ~WebService();

            bool handshake( const QString& username, const QString& password );

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
            {    recommend( WebService::Artist, username, artist ); }

            void recommendAlbum( QString username, QString artist, QString album )
            {    recommend( WebService::Album, username, artist, album ); }

            void recommendTrack( QString username, QString artist, QString track )
            {    recommend( WebService::Track, username, artist, track ); }

            /**
                Verify with server that a supplied user/pass combo is valid. Password
                should be MD5 hashed.
            **/
            void verifyUser( const QString& user, const QString& pass );

            KProcIO* getServer() { return m_server; }
            QString proxyUrl() { return m_proxyUrl; }

        public slots:
            void requestMetaData();
            void enableScrobbling( bool enabled );

            void love();
            void skip();
            void ban();

        signals:
            void actionStarted();
            void actionFinished();

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

            QString m_proxyUrl;
            KProcIO *m_server;
            MetaBundle m_metaBundle;

            QString parameter( QString keyName, QString data ) const;
            QStringList parameterArray( QString keyName, QString data ) const;
            QStringList parameterKeys( QString keyName, QString data ) const;

        private slots:
            void readProxy();
            void metaDataFinished( int id, bool error );
            void fetchImageFinished( int id, bool error );
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

    class Bundle
    {
        public:
            Bundle() {};
            Bundle( const Bundle& bundle);
            QString imageUrl() const { return m_imageUrl; }
            void setImageUrl( const QString& imageUrl ) { m_imageUrl = imageUrl; }

            QString artistUrl() const {  return m_artistUrl; }
            void setArtistUrl( const QString& theValue )  {  m_artistUrl = theValue; }

            QString albumUrl() const {  return m_albumUrl; }
            void setAlbumUrl( const QString& theValue )  {  m_albumUrl = theValue; }

            QString titleUrl() const {  return m_titleUrl; }
            void setTitleUrl( const QString& theValue )  {  m_titleUrl = theValue; }

        private:
            QString m_imageUrl;
            QString m_albumUrl;
            QString m_artistUrl;
            QString m_titleUrl;
    };

    // We must implement this because QServerSocket has one pure virtual method.
    // It's just used for finding a free port.
    class MyServerSocket : public QServerSocket
    {
        public:
            MyServerSocket() : QServerSocket( Q_UINT16( 0 ) ) {}

        private:
            void newConnection( int ) {}

    };

    class LoginDialog : public KDialogBase
    {
        Q_OBJECT
        public:
            LoginDialog( QWidget *parent );

        protected slots:
            void slotOk();

        private:
            KLineEdit *m_userLineEdit;
            KLineEdit *m_passLineEdit;

    };

}

#endif /*AMAROK_LASTFM_H*/

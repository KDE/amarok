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

#include <qhttp.h>
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

namespace KIO { class Job; }

/* AmarokHttp is a hack written so that lastfm code could easily use something proxy aware.
   DO NOT use this class for anything else, use KIO directly instead. */
class AmarokHttp : public QObject
{
    Q_OBJECT

    public:
    AmarokHttp ( const QString & hostname, Q_UINT16 port = 80, QObject* parent = 0 );
    int get ( const QString & path );
    QHttp::State state() const;
    QByteArray readAll ();
    QHttp::Error error();

    signals:
    void requestFinished ( int id, bool error );

    protected slots:
    void slotData(KIO::Job*, const QByteArray& );
    void slotResult(KIO::Job*);

    protected:
    QString m_hostname;
    Q_UINT16 m_port;
    QString  m_path;
    QHttp::State m_state;
    QHttp::Error m_error;
    bool m_done;
    QByteArray m_result;
};


namespace LastFm
{
    class WebService;

    class Controller : public QObject
    {
        Q_OBJECT

        public:
            static Controller* instance();

            KURL        getNewProxy( QString genreUrl, bool useProxy );

            bool        isPlaying() const { return m_service != 0; }
            WebService* getService() const { return m_service; }
            QString     getGenreUrl() const { return m_genreUrl; }

            static bool    checkCredentials();
            static QString createCustomStation();
            static QString stationDescription( QString url = QString::null );   // necessary for translation

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

            WebService( QObject* parent, bool useProxy );
            ~WebService();

            bool handshake( const QString& username, const QString& password );

            bool    changeStation( QString url );
            QString currentUsername() const { return m_username;  }
            QString currentPassword() const { return m_password;  }
            QString currentStation()  const { return m_station;   }
            QString session()         const { return m_session;   }
            QUrl    streamUrl()       const { return m_streamUrl; }

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
            enum errorCode { E_NOCONTENT    = 1, E_NOMEMBERS = 2, E_NOFANS = 3, E_NOAVAIL = 4, E_NOSUBSCRIBER = 5,
                             E_NONEIGHBOURS = 6, E_NOSTOPPED = 7, E_OTHER  = 0 };

            void        showError( int code, QString message = QString::null );

            bool m_useProxy;

            QString     parameter( const QString keyName, const QString data )      const;
            QStringList parameterArray( const QString keyName, const QString data ) const;
            QStringList parameterKeys( const QString keyName, const QString data )  const;

            QString m_username;     // login username
            QString m_password;     // login password
            QString m_station;      // the url of the station
            QString m_session;      // session id that last.fm provides
            QString m_baseHost;     // who are we connecting to?
            QString m_basePath;     // where are we connecting to!
            QUrl    m_streamUrl;    // last.fm webserver for direct connection (proxy connects to this)
            bool    m_subscriber;   // self explanatory

            KProcIO* m_server;

            QString    m_proxyUrl;
            MetaBundle m_metaBundle;

        private slots:
            void readProxy();
            void metaDataFinished( int id, bool error );
            void fetchImageFinished( KIO::Job* );
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

            void detach(); // for being able to apply QDeepCopy<>

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

    class CustomStationDialog : public KDialogBase
    {
        Q_OBJECT

        public:
            CustomStationDialog( QWidget *parent );

            QString text() const;

        private:
            KLineEdit *m_edit;
    };
}

#endif /*AMAROK_LASTFM_H*/

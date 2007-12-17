/***************************************************************************
 *   Copyright (C) 2007 by                                                 *
 *      Max Howell, Last.fm Ltd. <max@last.fm>                             *
 *      Christian Muehlhaeuser, Last.fm Ltd <chris@last.fm>                *
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

#ifndef SCROBBLER_H
#define SCROBBLER_H

#include <QList>

#include "CachedHttp.h"
#include "TrackInfo.h"

class ScrobbleCache;
class ScrobblerHandshakeRequest;
class ScrobblerNowPlayingRequest;
class ScrobblerPostRequest;


///////////////////////////////////////////////////////////////////////////////>
class Scrobbler : public QObject
{
    Q_OBJECT

    void hardFailure();

private slots:
    void onHandshakeReturn( const QString& );
    void onNowPlayingReturn( const QString& );
    void onSubmissionReturn( const QString& );
    void onHandshakeHeaderReceived( const QHttpResponseHeader& );

public:
    struct Init
    {
        QString username;
        QString password;
        QString client_version;
    };

    explicit Scrobbler( const Init& );

    enum Status
    {
        Connecting,
        Handshaken,
        Scrobbling,
        TracksScrobbled,
        TracksNotScrobbled,
        StatusMax
    };

    enum Error
    {
        ErrorBadSession = StatusMax,
        ErrorBannedClient,
        ErrorBadAuthorisation,
        ErrorBadTime,
        ErrorNotInitialized,
        NoError
    };

    bool canSubmit() const { return m_session_id.length() && m_submitted_tracks.isEmpty(); }
    bool canAnnounce() const { return m_session_id.length(); }

    QString username() const { return m_init.username; }
    Init init() const { return m_init; }

    /** @returns the number of tracks being scrobbled, 0 if empty cache or 
      * scrobbler is busy or scrobbler not handshaken yet, to test for that
      * you can examine the result of canSubmit() */
    int submit( const ScrobbleCache& );

    /** Will update the now-playing status at the Last.fm website */
    void announce( const TrackInfo& );

    /** resets the running count of the number of scrobbled tracks */
    void resetScrobbleCount() { m_scrobbled = 0; }
    uint scrobbled() const { return m_scrobbled; }

    Error lastError() const { return m_lastError; }
    static QString errorDescription( Scrobbler::Error error );

signals:
    void handshaken( Scrobbler* );
    void scrobbled( const QList<TrackInfo>& );
    void invalidated( int = NoError );

private:
    ScrobblerHandshakeRequest* m_handshake;
    ScrobblerNowPlayingRequest* m_now_playing;
    ScrobblerPostRequest* m_submission;

    Init m_init;
    QString m_session_id;
    uint m_hard_failures;
    uint m_scrobbled;
    Error m_lastError;

    class QTimer* m_timer;

    QList<TrackInfo> m_submitted_tracks;
};
///////////////////////////////////////////////////////////////////////////////>


///////////////////////////////////////////////////////////////////////////////>
class ScrobblerManager : public QObject
{
    Q_OBJECT

    /** one scrobbler per user */
    QList<Scrobbler*> m_scrobblers;

public:
    ScrobblerManager( QObject* parent = 0 );
    ~ScrobblerManager();

    void handshake( const Scrobbler::Init& );

    bool canScrobble( const QString& username ) const { return scrobblerForUser( username ); }
    Scrobbler::Error lastError( const QString& username ) const { return ( scrobblerForUser( username ) ? scrobblerForUser( username )->lastError() : Scrobbler::ErrorNotInitialized ); }

public slots:
    void scrobble( TrackInfo );
    void nowPlaying( const TrackInfo& );
    void scrobble( const class ScrobbleCache& );

signals:
    /** the controller should show status in an appropriate manner */
    void status( int code, QVariant data = QVariant() );

private slots:
    /** a scrobbler is now invalidate, delete it and handshake a new one */
    void onInvalidated( int errorcode );
    void onHandshaken( Scrobbler* );
    void onScrobbled( const QList<TrackInfo>& );

private:
    Scrobbler* scrobblerForUser( const QString& username ) const;
};
///////////////////////////////////////////////////////////////////////////////>


///////////////////////////////////////////////////////////////////////////////>
class ScrobblerHttp : public CachedHttp
{
    Q_OBJECT

protected:
    ScrobblerHttp( QObject* parent );

    int m_id;
    QTimer *m_retry_timer;

private slots:
    void onRequestFinished( int id, bool error );

signals:
    void done( const QString& data );

protected slots:
    virtual void request() = 0;

public:
    void resetRetryTimer();
    int retry();

    int id() const { return m_id; }
};
///////////////////////////////////////////////////////////////////////////////>


///////////////////////////////////////////////////////////////////////////////>
class ScrobblerHandshakeRequest : public ScrobblerHttp
{
    Scrobbler::Init m_init;

    virtual void request();

public:
    ScrobblerHandshakeRequest( QObject* parent ) : ScrobblerHttp( parent )
    {}

    virtual void request( const Scrobbler::Init& init)
    {
        m_init = init;
        request();
    }
};
///////////////////////////////////////////////////////////////////////////////>


///////////////////////////////////////////////////////////////////////////////>
class ScrobblerPostRequest : public ScrobblerHttp
{
    QString m_host;
    QString m_path;
    QByteArray m_data;

protected:
    virtual void request();

public:
    ScrobblerPostRequest( QObject* parent ) : ScrobblerHttp( parent )
    {}

    void setUrl( const QUrl& url );
    void request( const QByteArray& data );
};
///////////////////////////////////////////////////////////////////////////////>


///////////////////////////////////////////////////////////////////////////////>
/** class exists to allow us to delay 5 seconds before we announce new tracks
  * because if a user skips many tracks in quick succession, eg searching for a
  * good track in iTunes shuffle, it breaks facebook and teh intertubes */
class ScrobblerNowPlayingRequest : public ScrobblerPostRequest
{
    QTimer* m_timer;

    virtual void request();

public:
    ScrobblerNowPlayingRequest( QObject* );
};
///////////////////////////////////////////////////////////////////////////////>


///////////////////////////////////////////////////////////////////////////////>
class ScrobbleCache
{
protected:
    QString m_path;
    QString m_username;
    QList<TrackInfo> m_tracks;

    ScrobbleCache() //used by mediaDeviceCache()
    {}

    /** forces only one of each track for a specific timestamp */
    void merge( const TrackInfo& );

    void read();  /// reads from m_path into m_tracks
    void write(); /// writes m_tracks to m_path

    /// m_username needs to be set by the caller
    static ScrobbleCache fromFile( const QString& );

public:
    explicit ScrobbleCache( const QString& username );

    /** note this is unique for TrackInfo::sameAs() and equal timestamps 
      * obviously playcounts will not be increased for the same timestamp */
    void append( const TrackInfo& );
    void append( const QList<TrackInfo>& );

    /** returns the number of tracks left in the queue */
    int remove( const QList<TrackInfo>& );

    QList<TrackInfo> tracks() const { return m_tracks; }
    QString path() const { return m_path; }
    QString username() const { return m_username; }

    /** makes a timestamped backup of current cache */
    void backup();

    /** returns all backup file paths for all users */
    static QStringList pathsForCacheBackups();

    /** returns the media device scrobble cache for user */
    static ScrobbleCache mediaDeviceCache( const QString& username );

private:
    bool operator==( const ScrobbleCache& ); //undefined
};
///////////////////////////////////////////////////////////////////////////////>


///////////////////////////////////////////////////////////////////////////////>
class ItunesScrobbleHistory : private ScrobbleCache
{
public:
    ItunesScrobbleHistory();

    void append( const TrackInfo& );

    /** removes all tracks that were scrobbled before time_t */
    void prune( time_t );
};
///////////////////////////////////////////////////////////////////////////////>


namespace The
{
    ScrobblerManager& scrobbler(); //defined in container.cpp
}

#endif /* SCROBBLER_H */

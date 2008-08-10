/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Christian Muehlhaeuser, Last.fm Ltd <chris@last.fm>                *
 *      Erik Jaelevik, Last.fm Ltd <erik@last.fm>                          *
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

#include "container.h"
#include "Scrobbler-12.h"
#include "MooseCommon.h"
#include "logger.h"

#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QDomDocument>
#include <QDomElement>
#include <QFile>
#include <QTimer>


/** used to determine save location for scrobble caches 
  * NOTE you'll need to define this somewhere */
QString savePath( QString );

/** returns the 32 character HEX representation of the supplied data
  * NOTE you'll need to define this somewhere */
QString MD5Digest( const char* );


/** used to sort tracks into chronological order pre scrobbling submission */
static bool trackInfoLessThan( const TrackInfo &t1, const TrackInfo &t2)
{
    return t1.timeStamp() < t2.timeStamp();
}


///////////////////////////////////////////////////////////////////////////////>
ScrobblerManager::ScrobblerManager( QObject* parent )
        : QObject( parent )
{}


ScrobblerManager::~ScrobblerManager()
{
    QDateTime a_while_ago = QDateTime::currentDateTime().addDays( -7 );

    foreach (QString const path, ScrobbleCache::pathsForCacheBackups())
        if (QFileInfo( path ).created() < a_while_ago)
        {
            qDebug() << "Deleting expired backup:" << path;
            QFile::remove( path );
        }

//////
    qDeleteAll( m_scrobblers );
}


void
ScrobblerManager::handshake( const Scrobbler::Init& init )
{
    Q_DEBUG_BLOCK << init.username;

    Scrobbler* scrobbler = scrobblerForUser( init.username );

    if (!scrobbler) {
        scrobbler = new Scrobbler( init );

        connect( scrobbler, SIGNAL(handshaken( Scrobbler* )), SLOT(onHandshaken( Scrobbler* )) );
        connect( scrobbler, SIGNAL(scrobbled( QList<TrackInfo> )), SLOT(onScrobbled( QList<TrackInfo> )) );
        connect( scrobbler, SIGNAL(invalidated( int )), SLOT(onInvalidated( int )) );

        m_scrobblers += scrobbler;

        emit status( Scrobbler::Connecting );
    }
}


void //public
ScrobblerManager::scrobble( TrackInfo track )
{
    Q_DEBUG_BLOCK << track.toString();
    Q_ASSERT( !track.isEmpty() );

    ScrobbleCache cache( track.username() );
    cache.append( track );

    //HACK skip hack, see Container::skip()
    //NOTE this won't break scrobbling if the listener ever includes the rating
    // in its trackinfo object but it will delay that scrobble until a track
    // with no rating is submitted
    // we don't scrobble as the Player* classes may also submit this track
    // and we need to wait and see if they do and not scrobble both
    // just this one
    if ( !track.isSkippedLovedOrBanned() )
        scrobble( cache );
}


void //private
ScrobblerManager::scrobble( const ScrobbleCache& cache )
{
    Q_DEBUG_BLOCK << cache.username();
    Q_ASSERT( cache.username().length() );

    if (cache.tracks().isEmpty())
        qDebug() << "No tracks to scrobble";

    else if (qApp->closingDown())
        // if we are shutting down, save the submit for next time
        // we choose this method as otherwise we have to hang the app waiting for a 
        // timeout and this isn't safe --mxcl
        qDebug() << "Not actually submitting as we are shutting down";

    else if (Scrobbler* scrobbler = scrobblerForUser( cache.username() ))
    {
        int const N = scrobbler->submit( cache );

        if (N)
            emit status( Scrobbler::Scrobbling, N );
        else
            qDebug() << "Scrobbler not ready, will submit at earliest opportunity";
    }
    else
        qDebug() << "No scrobbler available for:" << cache.username();
}


void
ScrobblerManager::nowPlaying( const TrackInfo& track )
{
    Q_DEBUG_BLOCK << track.toString();

    Scrobbler* scrobbler = scrobblerForUser( track.username() );

    if (scrobbler && scrobbler->canAnnounce())
        //TODO cache the now playing if the scrobbler is not yet ready
        scrobbler->announce( track );
    else
        qDebug() << "No scrobbler found for user:" << track.username();
}


Scrobbler*
ScrobblerManager::scrobblerForUser( const QString& username ) const
{
    foreach (Scrobbler* s, m_scrobblers)
        if (s->username() == username)
            return s;

    return 0;
}


void
ScrobblerManager::onInvalidated( int code )
{
    Q_DEBUG_BLOCK << code;

    Scrobbler* scrobbler = (Scrobbler*)sender();
    scrobbler->deleteLater();
    int const N = m_scrobblers.removeAll( scrobbler );

    Q_ASSERT( N == 1 );

    switch (code)
    {
        case Scrobbler::ErrorBannedClient:
        case Scrobbler::ErrorBadAuthorisation:
        case Scrobbler::ErrorBadTime:
            // up to the Container to decide what to do
            break;

        case Scrobbler::ErrorBadSession:
        default:
            QString const username = scrobbler->username();
            handshake( scrobbler->init() ); // creates a new scrobbler
            break;
    }

    emit status( code );
}


void
ScrobblerManager::onScrobbled( const QList<TrackInfo>& tracks )
{
    Q_DEBUG_BLOCK << tracks.count() << "tracks were successfully scrobbled";
    Q_ASSERT( sender() );

//////
    Scrobbler* scrobbler = static_cast<Scrobbler*>(sender());
    ScrobbleCache cache( scrobbler->username() );

    if (tracks.count() > 2)
        //do a backup because nobody writes perfect code, least of all me!
        cache.backup();

    int remaining = cache.remove( tracks );

    if (remaining)
        scrobble( cache );
    else {
        // only show status on final submission batch, and only if something
        // was scrobbled (not skipped or banned)
        if (scrobbler->scrobbled() > 0)
            emit status( Scrobbler::TracksScrobbled, scrobbler->scrobbled() );

        scrobbler->resetScrobbleCount();
    }
}


void
ScrobblerManager::onHandshaken( Scrobbler* scrobbler )
{
    QString const username = scrobbler->username();

    Q_DEBUG_BLOCK << username;
    Q_ASSERT( username.length() );

    ScrobbleCache cache( username );
    scrobble( cache );

    emit status( Scrobbler::Handshaken, username );
}
///////////////////////////////////////////////////////////////////////////////>


///////////////////////////////////////////////////////////////////////////////>
ScrobbleCache::ScrobbleCache( const QString& username )
{
    Q_ASSERT( username.length() );

    m_path = MooseUtils::savePath( username + "_submissions.xml" );
    m_username = username;

    read();
}


void
ScrobbleCache::read()
{
    m_tracks.clear();

    QFile file( m_path );
    file.open( QFile::Text | QFile::ReadOnly );
    QTextStream stream( &file );
    stream.setCodec( "UTF-8" );

    QDomDocument xml;
    xml.setContent( stream.readAll() );

    for (QDomNode n = xml.documentElement().firstChild(); !n.isNull(); n = n.nextSibling())
        if (n.nodeName() == "item")
            m_tracks += TrackInfo( n.toElement() );
}


ScrobbleCache //static
ScrobbleCache::mediaDeviceCache( const QString& username )
{
    ScrobbleCache cache = fromFile( username + "_mediadevice.xml" );
    cache.m_username = username;
    return cache;
}


ScrobbleCache //static private
ScrobbleCache::fromFile( const QString& filename )
{
    ScrobbleCache cache;
    cache.m_path = MooseUtils::savePath( filename );
    cache.read();

    //NOTE we don't set username, that's up to the caller

    return cache;
}


void
ScrobbleCache::backup()
{
    QString timestamp = QDateTime::currentDateTime().toString( "yyyyMMddhhmm" );
        QString filename = QFileInfo( m_path ).baseName() + '.' + timestamp + ".backup.xml";
    ScrobbleCache backup = ScrobbleCache::fromFile( filename );

    // append in case we made a backup this minute already
    backup.append( tracks() );
}


QStringList //static
ScrobbleCache::pathsForCacheBackups()
{
    QStringList paths;

    QDir d = MooseUtils::savePath( "" );
    foreach (QString path, d.entryList( QStringList("*_submissions.*.backup.xml"), QDir::Files ))
        paths += d.filePath( path );

    return paths;
}


void
ScrobbleCache::write()
{
    if (m_tracks.isEmpty())
    {
        QFile::remove( m_path );
        qDebug() << m_path << "is now empty";
    }
    else {
        QDomDocument xml;
        QDomElement e = xml.createElement( "submissions" );
        e.setAttribute( "product", "Audioscrobbler" );
        e.setAttribute( "version", "1.2" );

        foreach (TrackInfo i, m_tracks)
            e.appendChild( i.toDomElement( xml ) );

        xml.appendChild( e );

        QFile file( m_path );
        file.open( QIODevice::WriteOnly | QIODevice::Text );

        QTextStream stream( &file );
        stream.setCodec( "UTF-8" );
        stream << "<?xml version='1.0' encoding='utf-8'?>\n";
        stream << xml.toString( 2 );

        qDebug() << "Wrote" << m_tracks.count() << "tracks to" << m_path;
    }
}


void
ScrobbleCache::append( const TrackInfo& track )
{
    append( QList<TrackInfo>() << track );
}


void
ScrobbleCache::append( const QList<TrackInfo>& tracks )
{
    foreach (const TrackInfo& track, tracks)
        merge( track );
    write();
}


//HACK
void
ScrobbleCache::merge( const TrackInfo& track )
{
    // we can't scrobble empties
    if (track.isEmpty()) {
        LOGL( 3, "Will not cache an empty track" );
        return;
    }

    if (QDateTime::fromTime_t(track.timeStamp()) < QDateTime::fromString( "2003-01-01", Qt::ISODate ))
    {
        LOGL( 3, "Won't scrobble track from before the date Audioscrobbler project was founded!" );
        return;
    }

    QMutableListIterator<TrackInfo> i( m_tracks );
    while (i.hasNext())
    {
        TrackInfo& cachedtrack = i.next();

        if (track.sameAs( cachedtrack ) && track.timeStamp() == cachedtrack.timeStamp())
        {
            // This will make sure no information gets lost
            cachedtrack.merge( track );
            return;
        }
    }

    m_tracks += track;

}
//HACK


int
ScrobbleCache::remove( const QList<TrackInfo>& toremove )
{
    qDebug() << m_tracks.count() << "toremove count:" << toremove.count();

    QMutableListIterator<TrackInfo> i( m_tracks );
    while (i.hasNext()) {
        TrackInfo t = i.next();
        for (int x = 0; x < toremove.count(); ++x)
            if (toremove[x].timeStamp() == t.timeStamp() && toremove[x].sameAs( t ))
            {
                qDebug() << "Removing" << t.toString();
                i.remove();
            }
    }

    qDebug() << m_tracks.count();

    write();

    // yes we return # remaining, rather # removed, but this is an internal 
    // function and the behaviour is documented so it's alright imo --mxcl
    return m_tracks.count();
}
///////////////////////////////////////////////////////////////////////////////>


///////////////////////////////////////////////////////////////////////////////>
ItunesScrobbleHistory::ItunesScrobbleHistory()
{
    m_path = MooseUtils::savePath( "iTunesScrobbleHistory.xml" );
    read();
}


void
ItunesScrobbleHistory::append( const TrackInfo& candidate )
{
    QMutableListIterator<TrackInfo> i( m_tracks );
    while (i.hasNext())
    {
        TrackInfo& track = i.next();
        if (track.sameAs( candidate )) 
        {
            int n = track.playCount() + 1;

            track = candidate; //thus timestamp is set
            track.setPlayCount( n );
            write();
            return;
        }
    }

    // if we get here then that track isn't in the itunes history file

    ScrobbleCache::append( candidate );
}


void
ItunesScrobbleHistory::prune( const time_t remove_if_before )
{
    QMutableListIterator<TrackInfo> i( m_tracks );
    while (i.hasNext())
        if (i.next().timeStamp() < remove_if_before)
            i.remove();

    write();
}
///////////////////////////////////////////////////////////////////////////////>


///////////////////////////////////////////////////////////////////////////////>
Scrobbler::Scrobbler( const Scrobbler::Init& init )
{
    m_lastError = Scrobbler::ErrorNotInitialized;
    m_hard_failures = m_scrobbled = 0;
    m_init = init;

    m_handshake = new ScrobblerHandshakeRequest( this );
    m_now_playing = new ScrobblerNowPlayingRequest( this );
    m_submission = new ScrobblerPostRequest( this );

    // the QueuedConnection is required as Http causes a crash if you spawn a new
    // event loop in a slot connected to any of its dataAvailable type functions
    connect( m_handshake, SIGNAL(done( QString )), SLOT(onHandshakeReturn( QString )), Qt::QueuedConnection );
    connect( m_now_playing, SIGNAL(done( QString )), SLOT(onNowPlayingReturn( QString )), Qt::QueuedConnection );
    connect( m_submission, SIGNAL(done( QString )), SLOT(onSubmissionReturn( QString )), Qt::QueuedConnection );

    connect( m_handshake, SIGNAL(responseHeaderReceived( QHttpResponseHeader )), SLOT(onHandshakeHeaderReceived( QHttpResponseHeader )) );

    connect( this, SIGNAL(invalidated( int )), m_handshake, SLOT(abort()) );
    connect( this, SIGNAL(invalidated( int )), m_now_playing, SLOT(abort()) );
    connect( this, SIGNAL(invalidated( int )), m_submission, SLOT(abort()) );

//////
    qDebug() << "Initiating Scrobbler handshake for:" << m_init.username;

    m_handshake->setHost( "post.audioscrobbler.com" );
    m_handshake->request( init );
}


void
Scrobbler::onHandshakeHeaderReceived( const QHttpResponseHeader& header )
{
    if (header.statusCode() != 200)
        hardFailure();
}


void
Scrobbler::hardFailure()
{
    Q_ASSERT( sender() );

    ScrobblerHttp* http = (ScrobblerHttp*)sender();
    qDebug() << "Scrobbler HTTP error:" << http->id();
    http->abort();

    m_lastError = Scrobbler::ErrorNotInitialized;
    if (http != m_handshake && ++m_hard_failures >= 3)
    {
        qDebug() << "Three hard failures. Invalidating this Scrobbler.";

        // ScrobblerManager will delete us and salvage submissions (if any)
        emit invalidated();
    }
    else {
        int const interval = http->retry();

        qDebug() << "Scrobbler hard failure. Retrying in" << interval / 1000 << "seconds.";
    }
}


void
Scrobbler::onHandshakeReturn( const QString& result )
{
    Q_DEBUG_BLOCK << result.trimmed();

    QStringList const results = result.split( '\n' );
    QString const code = results.value( 0 );

//////
    if (code == "OK" && results.count() >= 4)
    {
        m_lastError = Scrobbler::NoError;
        m_session_id = results[1];
        m_now_playing->setUrl( results[2] );
        m_submission->setUrl( results[3] );

        // reset hard failure state
        m_hard_failures = 0;
        m_handshake->resetRetryTimer();

        emit handshaken( this );

        // save memory *shrug*
        delete m_handshake;
        m_handshake = 0;
    }
    else if (code == "BANNED")
    {
        m_lastError = Scrobbler::ErrorBannedClient;
        emit invalidated( Scrobbler::ErrorBannedClient );
    }
    else if (code == "BADAUTH")
    {
        m_lastError = Scrobbler::ErrorBadAuthorisation;
        emit invalidated( Scrobbler::ErrorBadAuthorisation );
    }
    else if (code == "BADTIME")
    {
        m_lastError = Scrobbler::ErrorBadTime;
        emit invalidated( Scrobbler::ErrorBadTime );
    }
    else
        hardFailure();
}


/** @returns number of tracks submitted */
int
Scrobbler::submit( const ScrobbleCache& cache )
{
    if ( !canSubmit() )
        return 0;

    QList<TrackInfo> tracks = cache.tracks();

    // we need to put the tracks in chronological order or the Scrobbling Service
    // rejects the ones that are later than previously submitted tracks
    // this is only relevent if the cache is greater than 50 in size as then
    // submissions are done in batches, but better safe than sorry
    qSort( tracks.begin(), tracks.end(), trackInfoLessThan );
    tracks = tracks.mid( 0, 50 );

//////
    Q_DEBUG_BLOCK << (tracks.count() == 1 ? tracks[0].toString() : QString("%1 tracks").arg( tracks.count() ) );
    Q_ASSERT( m_submitted_tracks.isEmpty() );
    Q_ASSERT( m_session_id.size() );
    Q_ASSERT( tracks.size() <= 50 );

//////
    QString data = "s=" + m_session_id;
    bool portable = false;
    int n = 0;

    foreach (TrackInfo const i, tracks)
    {
        QString const N = QString::number( n++ );
        #define e( x ) QUrl::toPercentEncoding( x )
        data += "&a[" + N + "]=" + e(i.artist()) +
                "&t[" + N + "]=" + e(i.track()) +
                "&i[" + N + "]=" + QString::number( i.timeStamp() ) +
                "&o[" + N + "]=" + i.sourceString() + i.authCode() +
                "&r[" + N + "]=" + i.ratingCharacter() +
                "&l[" + N + "]=" + e(QString::number( i.duration() )) +
                "&b[" + N + "]=" + e(i.album()) +
                "&n[" + N + "]=" + //position in album if known, and we don't generally
                "&m[" + N + "]=" + i.mbId();
        #undef e

        if (i.source() == TrackInfo::MediaDevice)
            portable = true;
    }

    if (portable)
        data += "&portable=1";

//////
    m_submission->request( data.toUtf8() );
    m_submitted_tracks = tracks;

    return tracks.count();
}


void
Scrobbler::onSubmissionReturn( const QString& result )
{
    Q_DEBUG_BLOCK << "[id:" << ((ScrobblerHttp*)sender())->id() << "]" << result.trimmed();
    Q_ASSERT( m_submitted_tracks.count() );

//////
    QString const code = result.split( '\n' ).value( 0 );

    if (code == "OK")
    {
        m_lastError = Scrobbler::NoError;

        foreach (const TrackInfo& track, m_submitted_tracks)
            if (track.isLoved() || track.isScrobbled())
                m_scrobbled++; //displayed to user after a complete cache is submitted

        m_hard_failures = 0;
        m_submission->resetRetryTimer();

        // we must clear so that if more submissions are required they can proceed
        // as canSubmit() returns false if there are submitted tracks in the queue
        QList<TrackInfo> cp = m_submitted_tracks;
        m_submitted_tracks.clear();
        emit scrobbled( cp );
    }
    else {
        if (code == "BADSESSION")
        {
            m_lastError = Scrobbler::ErrorBadSession;
            emit invalidated( Scrobbler::ErrorBadSession );
        }
        else
            hardFailure();
    }
}


void
Scrobbler::announce( const TrackInfo& track )
{
    Q_DEBUG_BLOCK << track.toString();
    Q_ASSERT( m_session_id.size() );

    if (track.isEmpty()) {
        LOGL( 3, "Empty track, not announcing to now playing" );
        return;
    }

//////
    #define e( x ) QUrl::toPercentEncoding( x )
    QString data =  "s=" + e(m_session_id)
                 + "&a=" + e(track.artist())
                 + "&t=" + e(track.track())
                 + "&b=" + e(track.album())
                 + "&l=" + e(QString::number( track.duration() ))
                 + "&n=" //track number
                 + "&m=" + e(track.mbId());
    #undef e

    m_now_playing->ScrobblerPostRequest::request( data.toUtf8() );
}


void
Scrobbler::onNowPlayingReturn( const QString& result )
{
    qDebug() << "onNowPlayingReturn: [id:" << ((ScrobblerHttp*)sender())->id() << "]" << result.trimmed();

//////
    QString const code = result.split( '\n' ).value( 0 );

    if (code == "OK")
    {
        m_lastError = Scrobbler::NoError;
        m_hard_failures = 0;
        m_now_playing->resetRetryTimer();
    }
    else if (code == "BADSESSION")
    {
        m_lastError = Scrobbler::ErrorBadSession;
        emit invalidated( Scrobbler::ErrorBadSession );
    }
    else
        hardFailure();
}


QString
Scrobbler::errorDescription( Scrobbler::Error error )
{
    switch ( error )
    {
        case Scrobbler::ErrorBadSession:
            return i18n( "Bad session" );

        case Scrobbler::ErrorBannedClient:
            return i18n( "Client too old" );

        case Scrobbler::ErrorBadAuthorisation:
            return i18n( "Wrong username / password" );

        case Scrobbler::ErrorBadTime:
            return i18n( "Wrong timezone" );

        case Scrobbler::ErrorNotInitialized:
            return i18n( "Could not reach server" );

        default:
            return i18n( "OK" );
    }
}



///////////////////////////////////////////////////////////////////////////////>


///////////////////////////////////////////////////////////////////////////////>
ScrobblerHttp::ScrobblerHttp( QObject* parent )
            : CachedHttp( parent ),
              m_id( 0 )
{
    m_retry_timer = new QTimer( this );
    m_retry_timer->setSingleShot( true );
    connect( m_retry_timer, SIGNAL(timeout()), SLOT(request()) );
    resetRetryTimer();

    connect( this, SIGNAL(requestFinished( int, bool )), SLOT(onRequestFinished( int, bool )) );
}


void
ScrobblerHttp::onRequestFinished( int id, bool error )
{
    if (error && this->error() == QHttp::Aborted)
        return;

    if (id == m_id)
    {
        if (error)
            qDebug() << this;

        m_id = 0;
        emit done( error ? QString() : QString( readAll() ) );
    }
}


void
ScrobblerPostRequest::setUrl( const QUrl& url )
{
    m_path = url.path();
    m_host = url.host();
    setHost( m_host, url.port() );
}


int
ScrobblerHttp::retry()
{
    int const i = m_retry_timer->interval();
    if (i < 120 * 60 * 1000)
        m_retry_timer->setInterval( i * 2 );

    m_retry_timer->start();

    return m_retry_timer->interval();
}


void
ScrobblerHttp::resetRetryTimer()
{
    m_retry_timer->setInterval( 15 * 1000 );
}
///////////////////////////////////////////////////////////////////////////////>


///////////////////////////////////////////////////////////////////////////////>
void
ScrobblerHandshakeRequest::request()
{
    QString timestamp = QString::number( QDateTime::currentDateTime().toTime_t() );
    QString auth_token = UnicornUtils::md5Digest( (m_init.password + timestamp).toUtf8() );

    QString query_string = QString() +
                            "?hs=true" +
                            "&p=1.2" + //protocol version
                            "&c=ark" + //Amarok
                            "&v=" + m_init.client_version +
                            "&u=" + QString(QUrl::toPercentEncoding( m_init.username )) +
                            "&t=" + timestamp +
                            "&a=" + auth_token;

    m_id = get( '/' + query_string );

    qDebug() << "GET:" << query_string;
}
///////////////////////////////////////////////////////////////////////////////>


///////////////////////////////////////////////////////////////////////////////>
void
ScrobblerPostRequest::request()
{
    QHttpRequestHeader header( "POST", m_path );
    header.setValue( "Host", m_host );
    header.setContentType( "application/x-www-form-urlencoded" );

    qDebug() << "POST:" << m_data;

    m_id = CachedHttp::request( header, m_data );
}


void
ScrobblerPostRequest::request( const QByteArray& data )
{
    Q_DEBUG_BLOCK << "[id:" << m_id << ']' << data;

    m_data = data;
    request();
}
///////////////////////////////////////////////////////////////////////////////>


///////////////////////////////////////////////////////////////////////////////>
ScrobblerNowPlayingRequest::ScrobblerNowPlayingRequest( QObject* parent )
        : ScrobblerPostRequest( parent )
{
    m_timer = new QTimer( this );
    m_timer->setInterval( 5000 );
    m_timer->setSingleShot( true );
    connect( m_timer, SIGNAL(timeout()), SLOT(request()) );
}


void
ScrobblerNowPlayingRequest::request()
{
    if (sender())
        ScrobblerPostRequest::request();
    else
        m_timer->start();
}
///////////////////////////////////////////////////////////////////////////////>

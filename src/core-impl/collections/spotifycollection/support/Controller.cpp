/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Leo Franchi            <lfranchi@kde.org>
 *   Copyright 2012 Ryan Feng                    <odayfans@gmail.com>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */
#include "Controller.h"
#include "../SpotifyMeta.h"
#include "core/support/Debug.h"
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QtEndian>
#include <QTimer>
#include <KLocale>
#include "core/support/Amarok.h"
#include "core/support/Components.h"
#include "core/interfaces/Logger.h"

#define ShowMessage(x) Amarok::Components::logger()->shortMessage( x )

namespace Spotify
{

static Spotify::Controller* GlobalControllerInstance = 0;
static Spotify::Controller* InitGlobalController( const QString& resolverPath = QString() )
{
    if( !GlobalControllerInstance )
    {
        GlobalControllerInstance = new Spotify::Controller( resolverPath );
    }

    return GlobalControllerInstance;
}

} // namespace Spotify

namespace The
{
    Spotify::Controller* SpotifyController( const QString& resolverPath )
    {
        if( Spotify::GlobalControllerInstance )
        {
            return Spotify::GlobalControllerInstance;
        }
        else
        {
            return Spotify::InitGlobalController( resolverPath );
        }
    }
} // namespace The

namespace Spotify
{
Controller::Controller( const QString& exec )
: QObject(0)
, m_filePath( exec )
, m_msgSize( 0 )
, m_timeout ( 5 )
, m_ready( false )
, m_stopped( true )
, m_loaded( false )
, m_deleting( false )
, m_configSent( false )
, m_queryCounter( 0 )
{
    qDebug() << Q_FUNC_INFO << "Spotify Controller created: " << exec;

    connect( &m_proc, SIGNAL( readyReadStandardError() ), SLOT( readStderr() ) );
    connect( &m_proc, SIGNAL( readyReadStandardOutput() ), SLOT( readStdout() ) );
    connect( &m_proc, SIGNAL( finished( int, QProcess::ExitStatus ) ), SLOT( procExited( int, QProcess::ExitStatus ) ) );

}

Controller::~Controller()
{
    disconnect( &m_proc, SIGNAL( finished( int, QProcess::ExitStatus ) ), this, SLOT( procExited( int, QProcess::ExitStatus ) ) );
    m_deleting = true;
    unload();
}

void
Controller::unload()
{
    DEBUG_BLOCK
    QVariantMap msg;
    msg[ "_msgtype" ] = "quit";
    sendMessage( msg );

    bool finished = m_proc.waitForFinished( 2500 );

    if( !finished || m_proc.state() == QProcess::Running )
    {
        qDebug() << "External resolver didn't exit after wating 2s for it to die, killing forcefully";
#ifdef Q_OS_WIN
        m_proc.kill();
#else
        m_proc.terminate();
#endif
    }

    m_loaded = false;
}

void
Controller::start()
{
    DEBUG_BLOCK
    debug() << "Starting Spotify resolver...";
    if( !m_loaded )
    {
        debug() << "Resolver not loaded...";
        return;
    }

    m_stopped = false;
    if( m_ready )
    {
        //TODO: Notify Amarok to enable Spotify plugin
    } else if ( !m_configSent ) {
        sendConfig();
    }
    m_stopped = false;

    // Resolve all queries in query queue
    if( !m_queryQueue.isEmpty() )
    {
        foreach( Spotify::QueryPtr queryPtr, m_queryQueue )
        {
            resolve( queryPtr.data() );
        }
    }

    m_queryQueue.clear();
}

void
Controller::sendConfig()
{
    DEBUG_BLOCK
    // Sending config is optional, currently config only contains proxy information
    // Only if the Spotify resolver didn't send out settings message(not react after startProcess),
    // this config will be sent.
    if( m_configSent ){
        return;
    }

    QVariantMap config;
    config["_msgtype"] = "config";
    // Don't use proxy
    config["proxytype"] = "none";
    m_configSent = true;
    sendMessage(config);
}

void
Controller::reload()
{
    DEBUG_BLOCK
    unload();
    startProcess();
}

bool
Controller::loaded() const
{
    return m_loaded;
}

bool
Controller::running() const
{
    return !m_stopped;
}

void
Controller::sendMessage( const QVariantMap& map )
{
    DEBUG_BLOCK

    if( !m_loaded )
    {
        debug() << "Spotify resolver not loaded";
        return;
    }

    QByteArray data = m_serializer.serialize( map );
    debug() << "Sending message: \n" << QString(data);
    sendRaw( data );
}

void Controller::readStderr()
{
    debug() << "SCRIPT_STDERR" << filePath() << m_proc.readAllStandardError();
}

void
Controller::login(const QString &username, const QString &password, const bool highQuality)
{
    DEBUG_BLOCK

    QString msg = i18n( "Trying to login to Spotify..." );
    ShowMessage( msg );
    QVariantMap map;
    map["_msgtype"] = "login";
    map["username"] = username;
    map["password"] = password;
    map["highQuality"] = highQuality;

    sendMessage(map);
}

void
Controller::readStdout()
{
    DEBUG_BLOCK
    if( m_msgSize == 0 )
    {
        if( m_proc.bytesAvailable() < 4 )
            return;
        quint32 len;
        m_proc.read( (char*) &len, 4 );
        m_msgSize = qFromBigEndian( len );
    }

    if( m_msgSize > 0 )
    {
        m_msg.append( m_proc.read( m_msgSize - m_msg.length() ) );
    }

    if( m_msgSize == (quint32) m_msg.length() )
    {
        handleMsg( m_msg );
        m_msgSize = 0;
        m_msg.clear();

        if( m_proc.bytesAvailable() )
        {
            QTimer::singleShot( 0, this, SLOT( readStdout() ) );
        }
    }
    else
    {
        warning() << "Message header doesn't match with message length";
    }
}

Spotify::Query*
Controller::makeQuery( Collections::SpotifyCollection* collection, const QString &title, const QString &artist, const QString &album, const QString &genre )
{
    DEBUG_BLOCK
    QString qid = QString::number( m_queryCounter++ );
    return new Query( collection, qid, title, artist, album, genre );
}

void
Controller::resolve( Spotify::Query* query )
{
    DEBUG_BLOCK
    if( m_stopped )
    {
        debug() << "Resolver not started yet, resolve query later";
        // Plugin or resolve not started yet, put the query to queue and resolve later
        m_queryQueue.append( Spotify::QueryPtr( query ) );
        return;
    }

    Spotify::QueryPtr queryPtr( query );
    // Ignore current query if the query string is empty
    if( queryPtr->getFullQueryString().isEmpty() )
    {
        return;
    }

    QVariantMap map;
    map["_msgtype"] = "rq";
    map["qid"] = queryPtr->qid();
    // Use fulltext search
    map["fulltext"] = queryPtr->getFullQueryString();

    // Enqueue query
    m_queryCache[ queryPtr->qid() ] = queryPtr;

    connect( queryPtr.data(), SIGNAL( queryDone( const QString& ) ),
             this, SLOT( removeQueryFromCache( const QString& ) ) );

    sendMessage( map );
}

void
Controller::removeQueryFromCache( const QString& qid )
{
    m_queryCache[ qid ].clear();
    m_queryCache.remove( qid );
}

void
Controller::sendRaw( const QByteArray& msg )
{
    DEBUG_BLOCK
    if( !m_proc.isOpen() )
        return;

    quint32 len;
    qToBigEndian( msg.length(), (uchar*) &len );
    m_proc.write( (const char*) &len, 4 );
    m_proc.write( msg );
}

void
Controller::handleMsg( const QByteArray& msg )
{
    debug() << "Received message: " << msg;
    DEBUG_BLOCK
    // Drop all messages during desctruction
    if( m_deleting )
        return;

    bool ok;
    QVariant v = m_parser.parse( msg, &ok );
    if( !ok || v.type() != QVariant::Map )
    {
        Q_ASSERT(false);
        return;
    }

    QVariantMap m = v.toMap();
    QString msgtype = m.value( "_msgtype" ).toString();

    // msgType switch
    if( msgtype == "settings" )
    {
        doSetup( m );
        return;
    }
    if( msgtype == "results" )
    {
        // Search results
        handleSearchResults( m );
    }
    else if( msgtype == "loginResponse" )
    {
        handleLoginResponse( m );
    }
    else if( msgtype == "spotifyError" )
    {
        handleSpotifyError( m );
    }
    else if( msgtype == "userChanged" )
    {
        handleUserchanged( m );
    }
    else if( msgtype == "playlist" )
    {
        handlePlaylistReceived( m );
    }
    else if( msgtype == "playlistRenamed" )
    {
        handlePlaylistRenamed( m );
    }
    else if( msgtype == "tracksAdded" )
    {
        handleTracksAdded( m );
    }
    else if( msgtype == "tracksMoved" )
    {
        handleTracksMoved( m );
    }
    else if( msgtype == "tracksRemoved" )
    {
        handleTracksRemoved( m );
    }
    else if( msgtype == "playlistDeleted" )
    {
        handlePlaylistDeleted( m );
    }
    else if( msgtype == "allPlaylists" )
    {
        handleAllPlaylists( m );
    }
    else if( msgtype.isEmpty() )
        // A query response message
    {
        handleQueryResponse( m );
    }
    else
    {
        emit customMessage( msgtype, m );
    } // msgType switch
}

void
Controller::procExited( int code, QProcess::ExitStatus status )
{
    DEBUG_BLOCK
    m_ready = false;
    qDebug() << Q_FUNC_INFO << "RESOVER EXITED, code" << code << "status" << status << filePath();

    emit changed();

    if( m_stopped )
    {
        qDebug() << "*** Resolver stopped ";
        m_loaded = false;
        emit terminated();
        return;
    }
    else
    {
        qDebug() << "*** Resolver stoppped by accident, restarting...";
        startProcess();
        sendConfig();
    }

}

void
Controller::doSetup( const QVariantMap& m )
{
    DEBUG_BLOCK
    m_name = m.value( "name" ).toString();
    m_timeout = m.value( "timeout", 5 ).toUInt() * 1000;

    debug() << "RESOLVER" << filePath() << "READY," << "name" << m_name << "timeout" << m_timeout;

    m_ready = true;
    m_configSent = false;

    emit changed();
}

void
Controller::startProcess()
{
    DEBUG_BLOCK
    if( !QFile::exists( filePath() ) )
    {
        debug() << "*** Cannot find file" << filePath() << ", starting process failed";
        // TODO: Set error message
        return;
    }

    debug() << "Starting " << filePath();
    QFileInfo fi( filePath() );
    QString interpreter;
    QString runPath = filePath();

#ifdef Q_OS_WIN
    if( fi.suffix().toLower() != "exe" )
    {
        DWORD dwSize = MAX_PATH;

        wchar_t path[MAX_PATH] = { 0 };
        wchar_t *ext = (wchar_t *) ("." + fi.suffix()).utf16();

        HRESULT hr = AssocQueryStringW(
                (ASSOCF) 0,
                ASSOCSTR_EXECUTABLE,
                ext,
                L"open",
                path,
                &dwSize
        );

        if ( ! FAILED( hr ) )
        {
            interpreter = QString( "\"%1\"" ).arg(QString::fromUtf16((const ushort *) path));
        }
    }
    else
    {
        runPath = QString( "\"%1\"" ).arg( filePath() );
    }
#endif // Q_OS_WIN
    if( interpreter.isEmpty() )
    {
        m_proc.start( runPath );
    }
    else
    {
        m_proc.start( interpreter, QStringList() << filePath() );
    }

    m_loaded = true;

    sendConfig();

    emit spotifyReady();
}

void
Controller::stop()
{
    m_stopped = true;
}

void
Controller::handlePlaylistReceived( const QVariantMap& map )
{
    if( map["_msgtype"] != "playlist" ){
        return;
    }

    QString qid = map["qid"].toString();
    QString plId = map["id"].toString();
    QString plName = map["name"].toString();
    bool plSync = map["sync"].toBool();
    QVariantList tracks = map["tracks"].toList();

    // Get tracks in the playlist
    Meta::SpotifyTrackList trackList;
    foreach( const QVariant& v, tracks )
    {
        const QVariantMap trackMap = v.toMap();
        Meta::SpotifyTrackPtr track( new Meta::SpotifyTrack(
            trackMap["url"].toString(),
            trackMap["track"].toString(),
            trackMap["artist"].toString(),
            trackMap["album"].toString(),
            trackMap["genre"].toString(), // NOTE: Spotify doesn't give genre info currently
            trackMap["mimetype"].toString(),
            trackMap["score"].toDouble(),
            trackMap["duration"].toLongLong() * 1000,
            trackMap["bitrate"].toInt(),
            trackMap["size"].toInt(),
            trackMap["source"].toString()
        ) );
        trackList << track ;
    }

//    Playlists::SpotifyPlaylist* playlist =
//        new Plaulists::SpotifyPlaylist( plName, trackList, collection );
//    m_queryResults[qid] = playlist;

    // Notify the collection to retrive query result
    //emit queryDone( qid );
}

void
Controller::handlePlaylistRenamed( const QVariantMap& map )
{
}

void
Controller::handlePlaylistDeleted( const QVariantMap& map )
{
}

void
Controller::handleTracksAdded( const QVariantMap& map )
{
}

void
Controller::handleTracksDeleted( const QVariantMap& map )
{
}

void
Controller::handleTracksMoved( const QVariantMap& map )
{
}

void
Controller::handleTracksRemoved( const QVariantMap& map )
{
}

void
Controller::handleLoginResponse( const QVariantMap& map )
{
    bool success = map["success"].toBool();
    QString user = map["username"].toString();
    if( success )
    {
        ShowMessage( i18n( "Logged in to Spotify as %1" ).arg( user ) );
        emit loginSuccess( user );
    }
    else
    {
        ShowMessage( i18n( "Spotify login failed" ) );
        emit loginFailed();
    }
}

void
Controller::handleCredentialsReceived( const QVariantMap& map )
{
}

void
Controller::handleSettingsReceived( const QVariantMap& map )
{
}

void
Controller::handleAllPlaylists( const QVariantMap& map )
{
}

void
Controller::handleUserchanged( const QVariantMap& map )
{
}

void
Controller::handleSpotifyError( const QVariantMap& map )
{
}

void
Controller::handleQueryResponse( const QVariantMap& map )
{
}

void
Controller::handleSearchResults( const QVariantMap& map )
{
    DEBUG_BLOCK ;
    qDebug() << "Got search results from the resolver" ;

    QString qid = map["qid"].toString();
    if( m_queryCache.find( qid ) == m_queryCache.end() )
    {
        warning() << QString( "qid '%1' is missing in query cache, drop current results" ).arg( qid );
        return;
    }

    Spotify::QueryPtr queryPtr = m_queryCache[ qid ];
    if( queryPtr->qid() != qid )
    {
        return;
    }

    // Build track list
    Meta::SpotifyTrackList trackList;
    QVariantList list = map["results"].toList();

    foreach( const QVariant& v, list )
    {
        const QVariantMap trackMap = v.toMap();
        Meta::SpotifyTrackPtr track( new Meta::SpotifyTrack(
            trackMap["url"].toString(),
            trackMap["track"].toString(),
            trackMap["artist"].toString(),
            trackMap["album"].toString(),
            trackMap["genre"].toString(), // NOTE:Spotify doesn't give genre info currently
            trackMap["mimetype"].toString(),
            trackMap["score"].toDouble(),
            trackMap["duration"].toLongLong() * 1000,
            trackMap["bitrate"].toInt(),
            trackMap["size"].toInt(),
            trackMap["source"].toString()
        ) );

        trackList << track;
    }

    queryPtr->tracksAdded( trackList );
}

} // namespace Spotify

#include "Controller.moc"

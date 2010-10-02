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

#define DEBUG_PREFIX "MusicBrainzFinder"

#include "MusicBrainzFinder.h"

#include "core/meta/support/MetaConstants.h"
#include "core/meta/support/MetaUtility.h"
#include "core/support/Debug.h"
#include <QAuthenticator>
#include <QNetworkAccessManager>
#include <threadweaver/ThreadWeaver.h>

const QStringList tg_schemes( QStringList()
    //01 Artist - Title.ext
    << "^(%track%)\\W*-?\\W*(%artist%)\\W*-\\W*(%title%)\\.+(?:\\w{2,5})$"
    //01 Title.ext
    << "^(%track%)\\W*-?\\W*(%title%)\\.+(?:\\w{2,5})$"
    //Album - 01 - Artist - Title.ext
    << "^(%album%)\\W*-\\W*(%track%)\\W*-\\W*(%artist%)\\W*-\\W*(%title%)\\.+(?:\\w{2,5})$"
    //Artist - Album - 01 - Title.ext
    << "^(%artist%)\\W*-\\W*(%album%)\\W*-\\W*(%track%)\\W*-\\W*(%title%)\\.+(?:\\w{2,5})$"
    //Artist - 01 - Title.ext
    << "^(%artist%)\\W*-\\W*(%track%)\\W*-\\W*(%title%)\\.+(?:\\w{2,5})$"
    //Artist - Title.ext
    << "^(%artist%)\\W*-\\W*(%title%)\\.+(?:\\w{2,5})$"
);

/* Levenshtein distance algorithm implementation carefully pirated from wikibooks
 * (http://en.wikibooks.org/wiki/Algorithm_implementation/Strings/Levenshtein_distance)
 * and modified (a bit) to return similarity measure instead of distance.
 */
float similarity( const QString &s1, const QString &s2 )
{
    const uint len1 = s1.length(), len2 = s2.length();
    QVector < QVector < unsigned int > > d( len1 + 1, QVector < uint >( len2 + 1 ) );
    d[0][0] = 0;
    for( uint i = 1; i <= len1; ++i) d[i][0] = i;
    for( uint i = 1; i <= len2; ++i) d[0][i] = i;

    for( uint i = 1; i <= len1; ++i )
        for( uint j = 1; j <= len2; ++j )
            d[i][j] = qMin( qMin( d[i - 1][j] + 1, d[i][j - 1] + 1 ),
                            d[i - 1][j - 1] + ( s1[i - 1] == s2[j - 1] ? 0 : 1 ) );

    return ( float )( len1 - d[len1][len2] ) / len1;
}

MusicBrainzFinder::MusicBrainzFinder( QObject *parent, const QString &host,
                                     const int port, const QString &pathPrefix,
                                     const QString &username, const QString &password )
                 : QObject( parent )
                 , mb_host( host )
                 , mb_port( port )
                 , mb_pathPrefix( pathPrefix )
                 , mb_username( username )
                 , mb_password( password )
{
    DEBUG_BLOCK

    debug() << "Initiating MusicBrainz search:";
    debug() << "\tHost:\t\t" << mb_host;
    debug() << "\tPort:\t\t" << mb_port;
    debug() << "\tPath Prefix:\t" << mb_pathPrefix;
    debug() << "\tUsername:\t" << mb_username;
    debug() << "\tPassword:\t" << mb_password;

    net = The::networkAccessManager();

    _timer = new QTimer( this );
    _timer->setInterval( 1000 );

    connect( net, SIGNAL( finished( QNetworkReply * ) ), SLOT( gotReply( QNetworkReply * ) ) );
    connect( net, SIGNAL( authenticationRequired( QNetworkReply *, QAuthenticator * ) ),
            SLOT( authenticationRequest( QNetworkReply *, QAuthenticator * ) ) );
    connect( _timer, SIGNAL( timeout() ), SLOT( sendNewRequest() ) );
}

MusicBrainzFinder::~MusicBrainzFinder()
{
    if( _timer )
        delete _timer;
}

void
MusicBrainzFinder::lookUpByPUID( const Meta::TrackPtr &track, const QString &puid )
{
    m_requests.append( qMakePair( track, compilePUIDRequest( puid ) ) );

    if( !_timer->isActive() )
        _timer->start();
}

void
MusicBrainzFinder::run( const Meta::TrackList &tracks )
{
    QRegExp mb_trackId( "^amarok-sqltrackuid://mb-(\\w{8}-\\w{4}-\\w{4}-\\w{4}-\\w{12})$" );
    foreach( Meta::TrackPtr track, tracks )
    {
        if( !track->uidUrl().isEmpty() )
            if( mb_trackId.exactMatch( track->uidUrl() ) )
            {
                debug() << "Track " << track->fullPrettyName() << " already contains MusicBrainzID.";
                m_requests.append( qMakePair( track, compileIDRequest( mb_trackId.cap( 1 ) ) ) );
                continue;
            }
        m_requests.append( qMakePair( track, compileRequest( track ) ) );
    }
    _timer->start();
}

bool
MusicBrainzFinder::isRunning()
{
    return !( m_parsers.isEmpty() && m_requests.isEmpty() &&
              m_replyes.isEmpty() ) || _timer->isActive();
}

void
MusicBrainzFinder::authenticationRequest( QNetworkReply *reply, QAuthenticator *authenticator )
{
    if( reply->url().host() == mb_host )
    {
        authenticator->setUser( mb_username );
        authenticator->setPassword( mb_password );
    }
}

void
MusicBrainzFinder::sendNewRequest()
{
    DEBUG_BLOCK
    if( m_requests.isEmpty() )
    {
        if( m_parsers.isEmpty() &&  m_replyes.isEmpty() )
        {
            debug() << "There is no any queued requests. Stopping timer.";
            _timer->stop();
            emit done();
        }
        _timer->stop();
        return;
    }
    QPair < Meta::TrackPtr, QNetworkRequest > req = m_requests.takeFirst();
    QNetworkReply *reply = net->get( req.second );
    m_replyes.insert( reply, req.first );
    connect( reply, SIGNAL( error( QNetworkReply::NetworkError ) ),
             this, SLOT( replyError( QNetworkReply::NetworkError ) ) );

    if( !req.first.isNull() )
        emit statusMessage( "Looking for " + req.first->prettyName() );

    debug() << "Request sent: " << req.second.url().toString();
}

void
MusicBrainzFinder::gotReply( QNetworkReply *reply )
{
    DEBUG_BLOCK

    if( !m_replyes.contains( reply ) )
    {
        if( m_parsers.isEmpty() && m_requests.isEmpty() && m_replyes.isEmpty() )
        {
            debug() << "There is no any queued requests. Stopping timer.";
            _timer->stop();
            emit done();
        }
        return;
    }
    if( reply->error() != QNetworkReply::NoError )
    {
        debug() << "Request failed, remove it from queue.";
        m_replyes.remove( reply );

        if( m_parsers.isEmpty() && m_requests.isEmpty() && m_replyes.isEmpty() )
        {
            debug() << "There is no any queued requests. Stopping timer.";
            _timer->stop();
            emit done();
        }

        reply->deleteLater();
        return;
    }

    QString document( reply->readAll() );
    MusicBrainzXmlParser *parser = new MusicBrainzXmlParser( document );
    if( !m_replyes[ reply ].isNull() )
        m_parsers.insert( parser, m_replyes.take( reply ) );
    else
        m_replyes.remove( reply );

    connect( parser, SIGNAL( done( ThreadWeaver::Job * ) ), SLOT( parsingDone( ThreadWeaver::Job * ) ) );

    ThreadWeaver::Weaver::instance()->enqueue( parser );

    reply->deleteLater();
}

void
MusicBrainzFinder::replyError( QNetworkReply::NetworkError code )
{
    DEBUG_BLOCK
    QNetworkReply *reply = qobject_cast< QNetworkReply * >( sender() );
    if( !reply )
        return;

    if( !m_replyes.contains( reply ) )
        return;

    if( code == QNetworkReply::NoError )
        return;

    disconnect( reply, SIGNAL( error( QNetworkReply::NetworkError ) ),
                this, SLOT( replyError( QNetworkReply::NetworkError ) ) );

    debug() << "Error occurred during network request: " << reply->errorString();
    m_replyes.remove( reply );

    if( m_parsers.isEmpty() && m_requests.isEmpty() && m_replyes.isEmpty()  )
    {
        debug() << "There is no any queued requests. Stopping timer.";
        _timer->stop();
        emit done();

    }
    reply->deleteLater();
}

void
MusicBrainzFinder::parsingDone( ThreadWeaver::Job *_parser )
{
    DEBUG_BLOCK
    MusicBrainzXmlParser *parser = (MusicBrainzXmlParser *)_parser;
    disconnect( parser, SIGNAL( done( ThreadWeaver::Job * ) ), this, SLOT( parsingDone( ThreadWeaver::Job * ) ) );
    if( m_parsers.contains( parser ) )
    {
        Meta::TrackPtr track = m_parsers.take( parser );
        MusicBrainzTrack mbTrack;
        if( parser->type() == MusicBrainzXmlParser::TrackList && !parser->tracks.isEmpty() )
        {
            if( m_parsedmetadata.contains( track ) )
            {
                QMap < QString, QString > metadata = m_parsedmetadata[ track ];
                QString chosenTrack = parser->tracks.keys().first();
                int maxScore = 0;
                foreach( MusicBrainzTrack mbtrack, parser->tracks )
                {
                    int s = 0;
                    int releasePos = 0;
                    int releaseScore = 0;
                    if( metadata.contains( "album" ) )
                    {
                        int pos = 0;
                        foreach( QString releaseId, mbtrack.releases() )
                        {
                            int score = 0;
                            if( ( score =  12*similarity( metadata[ "album" ].toLower(),
                                                          parser->releases[ releaseId ].title().toLower() )
                                ) > releaseScore )
                            {
                                releaseScore = score;
                                releasePos = pos;
                            }
                            pos++;
                        }
                    }
                    if( metadata.contains( "artist" ) )
                        s += 16*similarity( metadata[ "artist" ].toLower(),
                                            parser->artists[ mbtrack.artistId() ].name().toLower() );

                    if( metadata.contains( "title" ) )
                        s += 20*similarity( metadata[ "title" ].toLower(),
                                            mbtrack.title().toLower() );

                    if( metadata.contains( "track" ) && ( mbtrack.releaseOffsets().count() > releasePos ) )
                        if( metadata[ "track" ].toInt()
                            == mbtrack.releaseOffsets().value( releasePos ) )
                            s += 22;
                    s += 18 - qMin( qAbs( track->length() - mbtrack.length() ), Q_INT64_C( 30000 ) )*18/30000;

                    if( s > maxScore )
                    {
                        maxScore = s;
                        chosenTrack = mbtrack.id();
                    }
                }
                mbTrack = parser->tracks[ chosenTrack ];
            }
            else
                mbTrack = parser->tracks.values().first();
        }
        else if( parser->type() == MusicBrainzXmlParser::Track && !parser->tracks.isEmpty() )
            mbTrack = parser->tracks.values().first();
        else
        {
            debug() << "Unexpected parsing result";
            parser->deleteLater();
            return;
        }

        mbTrack.setTrack( track );
        mb_tracks.insert( mbTrack.id(), mbTrack );

        if( mbTrack.releasesCount() )
        {
            if( !mb_releases.contains( mbTrack.releases().first() ) )
                m_requests.append( qMakePair( Meta::TrackPtr(), compileReleaseRequest( mbTrack.releases().first() ) ) );
            else
                sendTrack( mbTrack );
        }
        else
            sendTrack( mbTrack );
    }
    else if( parser->type() == MusicBrainzXmlParser::Release && !parser->releases.isEmpty() )
    {
        MusicBrainzRelease release = parser->releases.values().first();

        foreach( MusicBrainzArtist artist, parser->artists )
            if( !mb_artists.contains( artist.id() ) )
                mb_artists.insert( artist.id(), artist );

        if( !mb_releases.contains( release.id() ) )
            mb_releases.insert( release.id(), release );

        int pos = 0;
        foreach( QString trackId, release.tracks() )
        {
            pos++;
            if( !parser->tracks.contains( trackId ) )
                continue;

            MusicBrainzTrack mbtrack = parser->tracks[ trackId ];
            if( mb_tracks.contains( mbtrack.id() ) )
                if( !mb_tracks.value( mbtrack.id() ).track().isNull() )
                    mbtrack.setTrack( mb_tracks.value( mbtrack.id() ).track() );

            if( mbtrack.artistId().isEmpty() )
                mbtrack.setArtistId( release.artistId() );
            mbtrack.addRelease( release.id(), pos );
            
            mb_tracks.insert( mbtrack.id(), mbtrack );
            sendTrack( mbtrack );
        }

    }

    if( m_parsers.isEmpty() && m_requests.isEmpty() && m_replyes.isEmpty() )
    {
        debug() << "There is no any queued requests. Stopping timer.";
        _timer->stop();
        emit done();
    }
    parser->deleteLater();
}

QMap< QString, QString >
MusicBrainzFinder::guessMetadata( const Meta::TrackPtr &track )
{
    DEBUG_BLOCK
    debug() << "Trying to guess metadata from filename: " << track->playableUrl().fileName().toLower();
    QRegExp rx;
    QRegExp rxm( "%(\\w+)%" );
    QRegExp rxt( "(%\\w+%)" );
    QRegExp spaces( "(^\\s+|\\s+$)" );
    QStringList markers;
    QMap < QString, QString > metadata;

    if( !track->name().isEmpty() )
        metadata[ "title" ] = track->name();
    if( !track->artist().isNull() )
        if( !track->artist()->name().isEmpty() )
            metadata[ "artist" ] = track->artist()->name();
    if( !track->album().isNull() )
        if( !track->album()->name().isEmpty() )
            metadata[ "album" ] = track->album()->name();
    if( track->trackNumber() > 0 )
        metadata[ "track" ] = QString::number( track->trackNumber() );

    foreach(QString scheme, tg_schemes)
    {
        markers.clear();
        int pos = 0;
        while( ( pos = rxm.indexIn( scheme, pos ) ) != -1 )
        {
            markers << rxm.cap(1);
            pos += rxm.matchedLength();
        }
        rx.setPattern( scheme.replace( "%track%", "\\d{1,2}" ).replace( rxt, ".+" ) );
        if( !rx.exactMatch( track->playableUrl().fileName().toLower().replace( "_", " " ) ) )
            continue;
        for( int i = 0; i < markers.count(); i++ )
            metadata.insert( markers[i], rx.cap( i+1 ).remove( spaces ) );
        break;
    }

    debug() << "Guessed track info:";
    foreach( QString tag, metadata.keys() )
    {
        debug() << '\t' << tag << ":\t" << metadata[ tag ];
    }
    return metadata;
}

void
MusicBrainzFinder::sendTrack( MusicBrainzTrack &track )
{
    if( track.track().isNull() )
        return;

    QVariantMap tags;

    tags.insert( Meta::Field::TITLE, track.title() );
    tags.insert( Meta::Field::UNIQUEIDOWNER, "http://musicbrainz.org" );
    tags.insert( Meta::Field::UNIQUEID, track.id() );
    if( mb_artists.contains( track.artistId() ) )
        tags.insert( Meta::Field::ARTIST, mb_artists.value( track.artistId() ).name() );
    if( track.releasesCount() )
        if( mb_releases.contains( track.releases().first() ) )
        {
            tags.insert( Meta::Field::ALBUM, mb_releases.value( track.releases().first() ).title() );
            if( mb_releases.value( track.releases().first() ).year() > 0 )
                tags.insert( Meta::Field::YEAR,
                             QString::number( mb_releases.value( track.releases().first() ).year() ) );
            if( track.releaseOffsets().count() )
                tags.insert( Meta::Field::TRACKNUMBER, track.releaseOffsets().first() );
        }

    emit trackFound( track.track(), tags );
    emit statusMessage( track.track()->prettyName() + " found." );
}

QNetworkRequest
MusicBrainzFinder::compileRequest( const Meta::TrackPtr &track )
{
    DEBUG_BLOCK
    QString query = QString( "qdur:(%1)" ).arg( int( track->length()/2000 ) );
    QMap < QString, QString > metadata = guessMetadata( track );
    if( !metadata.isEmpty() )
    {
        if( metadata.contains( "title" ) )
            query += QString( " track:(%1)" ).arg( metadata[ "title" ] );
        if( metadata.contains( "artist" ) )
            query += QString( " artist:(%1)" ).arg( metadata[ "artist" ] );
        if( metadata.contains( "album" ) )
            query += QString ( " release:(%1)" ).arg( metadata[ "album" ] );

        m_parsedmetadata.insert( track, metadata );
    }
    else
        query += QString( " track:(%1)" ).arg( track->playableUrl().fileName().toLower().remove(
                                               QRegExp( "^.*(\\.+(?:\\w{2,5}))$" ) ) );
    QUrl url;
    url.setScheme( "http" );
    url.setHost( mb_host );
    url.setPort( mb_port );
    url.setPath( mb_pathPrefix+"/track/" );
    url.addQueryItem( "type", "xml" );
    url.addQueryItem( "limit", "10" );
    url.addQueryItem( "query", query );

    QNetworkRequest req( url);
    req.setRawHeader( "User-Agent" , "Amarok" );
    req.setRawHeader( "Connection", "Keep-Alive" );

    if( !_timer->isActive() )
        _timer->start();

    return req;
}

QNetworkRequest
MusicBrainzFinder::compileReleaseRequest( const QString &releasId )
{
    DEBUG_BLOCK
    QUrl url;
    url.setScheme( "http" );
    url.setHost( mb_host );
    url.setPort( mb_port );
    url.setPath( mb_pathPrefix+"/release/"+ releasId );
    url.addQueryItem( "type", "xml" );
    url.addQueryItem( "inc", "tracks+artist+release-events" );

    QNetworkRequest req( url );
    req.setRawHeader( "User-Agent" , "Amarok" );
    req.setRawHeader( "Connection", "Keep-Alive" );

    if( !_timer->isActive() )
        _timer->start();

    return req;
}

QNetworkRequest
MusicBrainzFinder::compilePUIDRequest( const QString &puid )
{
    DEBUG_BLOCK
    QUrl url;
    url.setScheme( "http" );
    url.setHost( mb_host );
    url.setPort( mb_port );
    url.setPath( mb_pathPrefix+"/track/" );
    url.addQueryItem( "type", "xml" );
    url.addQueryItem( "puid", puid );

    QNetworkRequest req( url );
    req.setRawHeader( "User-Agent" , "Amarok" );
    req.setRawHeader( "Connection", "Keep-Alive" );

    if( !_timer->isActive() )
        _timer->start();

    return req;
}

QNetworkRequest
MusicBrainzFinder::compileIDRequest( const QString &id )
{
    DEBUG_BLOCK
    QUrl url;
    url.setScheme( "http" );
    url.setHost( mb_host );
    url.setPort( mb_port );
    url.setPath( mb_pathPrefix+"/track/"+ id );
    url.addQueryItem( "type", "xml" );
    url.addQueryItem( "inc", "artist+releases" );

    QNetworkRequest req( url );
    req.setRawHeader( "User-Agent" , "Amarok" );
    req.setRawHeader( "Connection", "Keep-Alive" );

    if( !_timer->isActive() )
        _timer->start();

    return req;
}

#include "MusicBrainzFinder.moc"
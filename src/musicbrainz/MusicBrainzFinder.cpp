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
#include "MusicBrainzMeta.h"
#include <QAuthenticator>
#include <QNetworkAccessManager>
#include <QTimer>
#include <threadweaver/ThreadWeaver.h>

const QStringList tg_schemes( QStringList()
    //01 Artist - Title.ext
    << "^(%"+Meta::Field::TRACKNUMBER+"%)\\W*-?\\W*(%"+Meta::Field::ARTIST
        +"%)\\W*-\\W*(%"+Meta::Field::TITLE+"%)\\.+(?:\\w{2,5})$"
    //01 Title.ext
    << "^(%"+Meta::Field::TRACKNUMBER+"%)\\W*-?\\W*(%"+Meta::Field::TITLE+"%)\\.+(?:\\w{2,5})$"
    //Album - 01 - Artist - Title.ext
    << "^(%"+Meta::Field::ALBUM+"%)\\W*-\\W*(%"
        +Meta::Field::TRACKNUMBER+"%)\\W*-\\W*(%"+Meta::Field::ARTIST+"%)\\W*-\\W*(%"
        +Meta::Field::TITLE+"%)\\.+(?:\\w{2,5})$"
    //Artist - Album - 01 - Title.ext
    << "^(%"+Meta::Field::ARTIST+"%)\\W*-\\W*(%"
        +Meta::Field::ALBUM+"%)\\W*-\\W*(%"+Meta::Field::TRACKNUMBER+"%)\\W*-\\W*(%"
        +Meta::Field::TITLE+"%)\\.+(?:\\w{2,5})$"
    //Artist - 01 - Title.ext
    << "^(%"+Meta::Field::ARTIST+"%)\\W*-\\W*(%"
        +Meta::Field::TRACKNUMBER+"%)\\W*-\\W*(%"+Meta::Field::TITLE+"%)\\.+(?:\\w{2,5})$"
    //Artist - Title.ext
    << "^(%"+Meta::Field::ARTIST+"%)\\W*-\\W*(%"+Meta::Field::TITLE+"%)\\.+(?:\\w{2,5})$"
    //Title.ext
    << "^(%"+Meta::Field::TITLE+"%)\\.+(?:\\w{2,5})$"
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
                debug() << "Track " << track->fullPrettyName() << " already contains MusicBrainz Track ID.";
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
        checkDone();
        return;
    }
    QPair < Meta::TrackPtr, QNetworkRequest > req = m_requests.takeFirst();
    QNetworkReply *reply = net->get( req.second );
    m_replyes.insert( reply, req.first );
    connect( reply, SIGNAL( error( QNetworkReply::NetworkError ) ),
             this, SLOT( replyError( QNetworkReply::NetworkError ) ) );

    debug() << "Request sent: " << req.second.url().toString();
}

void
MusicBrainzFinder::gotReply( QNetworkReply *reply )
{
    DEBUG_BLOCK
    if( reply->error() == QNetworkReply::NoError && m_replyes.contains( reply ) )
    {
        QString document( reply->readAll() );
        MusicBrainzXmlParser *parser = new MusicBrainzXmlParser( document );
        if( !m_replyes.value( reply ).isNull() )
            m_parsers.insert( parser, m_replyes.value( reply ) );

        connect( parser, SIGNAL( done( ThreadWeaver::Job * ) ), SLOT( parsingDone( ThreadWeaver::Job * ) ) );
        ThreadWeaver::Weaver::instance()->enqueue( parser );
    }

    m_replyes.remove( reply );
    reply->deleteLater();
    checkDone();
}

void
MusicBrainzFinder::replyError( QNetworkReply::NetworkError code )
{
    DEBUG_BLOCK
    QNetworkReply *reply = qobject_cast< QNetworkReply * >( sender() );
    if( !reply )
        return;

    if( !m_replyes.contains( reply ) || code == QNetworkReply::NoError )
        return;

    debug() << "Error occurred during network request: " << reply->errorString();
    disconnect( reply, SIGNAL( error( QNetworkReply::NetworkError ) ),
                this, SLOT( replyError( QNetworkReply::NetworkError ) ) );
    m_replyes.remove( reply );
    reply->deleteLater();
    checkDone();
}

void
MusicBrainzFinder::parsingDone( ThreadWeaver::Job *_parser )
{
    DEBUG_BLOCK
    MusicBrainzXmlParser *parser = qobject_cast< MusicBrainzXmlParser * >( _parser );
    disconnect( parser, SIGNAL( done( ThreadWeaver::Job * ) ), this, SLOT( parsingDone( ThreadWeaver::Job * ) ) );
    if( m_parsers.contains( parser ) )
    {
        Meta::TrackPtr trackPtr = m_parsers.take( parser );
        QVariantMap curTrack;

        emit progressStep();
        if( parser->type() == MusicBrainzXmlParser::TrackList && !parser->tracks.isEmpty() )
        {
            if( m_parsedMetaData.contains( trackPtr ) )
            {
                QVariantMap metadata = m_parsedMetaData.value( trackPtr );
                QVariantMap chosenTrack;
                int maxScore = 0;
                foreach( QVariantMap track, parser->tracks.values() )
                {
                    int s = 0;
                    int maxPossibleScore = 0;
                    QString release;
                    if( metadata.contains( Meta::Field::ALBUM ) && track.contains( MusicBrainz::RELEASELIST ) )
                    {
                        int releaseScore = 0;

                        foreach( QString releaseID, track.value( MusicBrainz::RELEASELIST ).toStringList() )
                        {
                            int score = 0;
                            if( ( score =  12*similarity( metadata.value( Meta::Field::ALBUM ).toString().toLower(),
                                  parser->releases.value( releaseID ).value( Meta::Field::TITLE )
                                  .toString().toLower() ) ) > releaseScore )
                            {
                                releaseScore = score;
                                release = releaseID;
                            }
                        }
                        maxPossibleScore += 12;
                    }

                    if( release.isEmpty() && track.contains( MusicBrainz::RELEASELIST) )
                    {
                        release = track.value( MusicBrainz::RELEASELIST ).toStringList().first();

                        track.insert( MusicBrainz::RELEASEID, release );
                        track.insert( Meta::Field::ALBUM,
                                      parser->releases.value( release ).value( Meta::Field::TITLE ).toString() );
                        track.insert( Meta::Field::TRACKNUMBER,
                                      track.value( MusicBrainz::TRACKOFFSET ).toMap().value( release ).toInt() );
                    }

                    if( metadata.contains( Meta::Field::ARTIST ) && track.contains( Meta::Field::ARTIST ) )
                    {
                        s += 16*similarity( metadata.value( Meta::Field::ARTIST ).toString().toLower(),
                                            track.value( Meta::Field::ARTIST ).toString().toLower() );
                        maxPossibleScore += 16;
                    }

                    if( metadata.contains( Meta::Field::TITLE ) && track.contains( Meta::Field::TITLE ) )
                    {
                        s += 20*similarity( metadata.value( Meta::Field::TITLE).toString().toLower(),
                                            track.value( Meta::Field::TITLE ).toString().toLower() );
                        maxPossibleScore += 20;
                    }

                    if( metadata.contains( Meta::Field::TRACKNUMBER ) && track.contains( Meta::Field::TRACKNUMBER ) )
                    {
                        s += ( metadata.value( Meta::Field::TRACKNUMBER ).toInt()
                               == track.value( Meta::Field::TRACKNUMBER ).toInt() )? 22 : 0;
                        maxPossibleScore += 22;
                    }

                    s += 18 - qMin( qAbs( trackPtr->length() - track.value( Meta::Field::LENGTH ).toInt() ),
                                    Q_INT64_C( 30000 ) ) * 18 / 30000;
                    maxPossibleScore += 18;

                    if( ( s > maxScore ) && ( float( s ) / maxPossibleScore > 0.6 ) )   //Minimal similarity level - 60%
                    {
                        maxScore = s;
                        chosenTrack = track;
                    }
                }

                m_parsedMetaData.remove( trackPtr );
                if( chosenTrack.isEmpty() )
                {
                    parser->deleteLater();
                    checkDone();
                    return;
                }
                curTrack = chosenTrack;
            }
            else
                curTrack = parser->grabFirstTrack();

            if( curTrack.contains( MusicBrainz::RELEASEID ) )
            {
                QString releaseID = curTrack.value( MusicBrainz::RELEASEID ).toString();
                if( !mb_releasesCache.contains( releaseID ) )
                {
                    if( !mb_waitingForReleaseQueue.contains( releaseID ) )
                    {
                        mb_waitingForReleaseQueue.insert( releaseID,
                                                          QStringList( curTrack.value( MusicBrainz::TRACKID ).toString() ) );
                        m_requests.append( qMakePair( Meta::TrackPtr(), compileReleaseRequest( releaseID ) ) );
                    }
                    else
                        mb_waitingForReleaseQueue[ releaseID ].append( QStringList( curTrack.value( MusicBrainz::TRACKID ).toString() ) );

                    mb_tracks.insert( curTrack.value( MusicBrainz::TRACKID ).toString(), trackPtr );
                    mb_trackInfo.insert( curTrack.value( MusicBrainz::TRACKID ).toString(), curTrack );
                }
                else
                    sendTrack( trackPtr, curTrack );
            }
            else
                sendTrack( trackPtr, curTrack );
        }
        else if( parser->type() == MusicBrainzXmlParser::Track && !parser->tracks.isEmpty() )
        {
            curTrack = parser->grabFirstTrack();
            if( curTrack.contains( MusicBrainz::RELEASEID ) )
            {
                QString releaseID = curTrack.value( MusicBrainz::RELEASEID ).toString();
                if( !mb_releasesCache.contains( releaseID ) )
                {
                    if( !mb_waitingForReleaseQueue.contains( releaseID ) )
                    {
                        mb_waitingForReleaseQueue.insert( releaseID,
                                                          QStringList( curTrack.value( MusicBrainz::TRACKID ).toString() ) );
                        m_requests.append( qMakePair( Meta::TrackPtr(), compileReleaseRequest( releaseID ) ) );
                    }
                    else
                        mb_waitingForReleaseQueue[ releaseID ].append( QStringList( curTrack.value( MusicBrainz::TRACKID ).toString() ) );

                    mb_tracks.insert( curTrack.value( MusicBrainz::TRACKID ).toString(), trackPtr );
                    mb_trackInfo.insert( curTrack.value( MusicBrainz::TRACKID ).toString(), curTrack );
                }
                else
                    sendTrack( trackPtr, curTrack );
            }
            else
                sendTrack( trackPtr, curTrack );
        }
        else
            debug() << "Unexpected parsing result";
    }
    else if( parser->type() == MusicBrainzXmlParser::Release && !parser->releases.isEmpty() )
    {
        QString release = parser->releases.keys().first();
        mb_releasesCache.insert( release, parser->releases.value( release ) );
        foreach( QString trackID, mb_waitingForReleaseQueue.value( release ) )
            sendTrack( mb_tracks.take( trackID ), mb_trackInfo.take( trackID ) );
        mb_waitingForReleaseQueue.remove( release );
    }

    parser->deleteLater();
    checkDone();
}

void
MusicBrainzFinder::checkDone()
{
    if( m_parsers.isEmpty() && m_requests.isEmpty() && m_replyes.isEmpty() && mb_waitingForReleaseQueue.isEmpty() )
    {
        debug() << "There is no any queued requests. Stopping timer.";
        _timer->stop();
        emit done();
    }
}

QVariantMap
MusicBrainzFinder::guessMetadata( const Meta::TrackPtr &track )
{
    DEBUG_BLOCK
    debug() << "Trying to guess metadata from filename: " << track->playableUrl().fileName().toLower();
    QRegExp rx;
    QRegExp rxm( "%([\\w|\\:]+)%" );
    QRegExp rxt( "(%[\\w|\\:]+%)" );
    QRegExp spaces( "(^\\s+|\\s+$)" );
    QStringList markers;
    QVariantMap metadata;

    if( !track->artist().isNull() && !track->artist()->name().isEmpty() )
        metadata.insert( Meta::Field::ARTIST, track->artist()->name() );
    if( !track->album().isNull() && !track->album()->name().isEmpty() )
        metadata.insert( Meta::Field::ALBUM, track->album()->name() );
    if( track->trackNumber() > 0 )
        metadata.insert( Meta::Field::TRACKNUMBER, track->trackNumber() );

    foreach( QString scheme, tg_schemes )
    {
        markers.clear();
        int pos = 0;
        while( ( pos = rxm.indexIn( scheme, pos ) ) != -1 )
        {
            markers << rxm.cap(1);
            pos += rxm.matchedLength();
        }
        rx.setPattern( scheme.replace( "%"+Meta::Field::TRACKNUMBER+"%", "\\d{1,2}" ).replace( rxt, ".+" ) );
        if( !rx.exactMatch( track->playableUrl().fileName().toLower().replace( "_", " " ) ) )
            continue;
        for( int i = 0; i < markers.count(); i++ )
            metadata.insert( markers[i], rx.cap( i+1 ).remove( spaces ) );
        break;
    }

    debug() << "Guessed track info:";
    foreach( QString tag, metadata.keys() )
    {
        debug() << '\t' << tag << ":\t" << metadata.value( tag ).toString();
    }
    return metadata;
}

void
MusicBrainzFinder::sendTrack( const Meta::TrackPtr track, const QVariantMap &info )
{
    QVariantMap tags = info;
    tags.remove( MusicBrainz::RELEASELIST );
    tags.remove( MusicBrainz::TRACKOFFSET );
    tags.insert( Meta::Field::UNIQUEID, tags.value( MusicBrainz::TRACKID ).toString().prepend( "mb-" ) );
    if( tags.contains( MusicBrainz::RELEASEID )
        && mb_releasesCache.contains( tags.value( MusicBrainz::RELEASEID ).toString() ) )
    {
        QVariantMap release = mb_releasesCache.value( tags.value( MusicBrainz::RELEASEID ).toString() );

        tags.insert( Meta::Field::ALBUM, release.value( Meta::Field::TITLE) );

        if( release.contains( Meta::Field::YEAR ) )
            tags.insert( Meta::Field::YEAR, release.value( Meta::Field::YEAR ) );

        if( release.contains( Meta::Field::ARTIST ) )
            tags.insert( Meta::Field::ALBUMARTIST, release.value( Meta::Field::ARTIST ) );
    }
    emit trackFound( track, tags );
}

QNetworkRequest
MusicBrainzFinder::compileRequest( const Meta::TrackPtr &track )
{
    QString query = QString( "qdur:(%1)" ).arg( int( track->length() / 2000 ) );
    QVariantMap metadata = guessMetadata( track );

    if( metadata.contains( Meta::Field::TITLE ) )
        query += QString( " track:(%1)" ).arg( metadata.value( Meta::Field::TITLE ).toString() );
    if( metadata.contains( Meta::Field::ARTIST ) )
        query += QString( " artist:(%1)" ).arg( metadata.value( Meta::Field::ARTIST ).toString() );
    if( metadata.contains( Meta::Field::ALBUM ) )
        query += QString ( " release:(%1)" ).arg( metadata.value( Meta::Field::ALBUM ).toString() );

    m_parsedMetaData.insert( track, metadata );

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
    QUrl url;
    url.setScheme( "http" );
    url.setHost( mb_host );
    url.setPort( mb_port );
    url.setPath( mb_pathPrefix+"/release/"+ releasId );
    url.addQueryItem( "type", "xml" );
    url.addQueryItem( "inc", "artist+release-events" );

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
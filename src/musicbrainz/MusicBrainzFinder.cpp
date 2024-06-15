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

#define DEBUG_PREFIX "MusicBrainzFinder"

#include "MusicBrainzFinder.h"

#include "core/meta/Meta.h"
#include "core/meta/support/MetaConstants.h"
#include "core/meta/support/MetaUtility.h"
#include "core/support/Debug.h"
#include "MusicBrainzMeta.h"
#include "TagsFromFileNameGuesser.h"

#include <ThreadWeaver/Queue>
#include <ThreadWeaver/Job>

#include <QAuthenticator>
#include <QNetworkAccessManager>
#include <QRegularExpression>
#include <QTimer>
#include <QUrlQuery>

/*
 * Levenshtein distance algorithm implementation carefully pirated from Wikibooks
 * (http://en.wikibooks.org/wiki/Algorithm_implementation/Strings/Levenshtein_distance)
 * and modified (a bit) to return similarity measure instead of distance.
 */
float
similarity( const QString &s1, const QString &s2 )
{
    const size_t len1 = s1.length(), len2 = s2.length();
    QVector<uint> col( len2 + 1 ), prevCol( len2 + 1 );

    for( uint i = 0; i <= len2; i++ )
        prevCol[i] = i;
    for( uint i = 0; i < len1; i++ )
    {
        col[0] = i + 1;
        for( uint j = 0; j < len2; j++ )
            col[j + 1] = qMin( qMin( 1 + col[j], 1 + prevCol[1 + j] ),
                               prevCol[j] + ( s1[i] == s2[j] ? 0 : 1 ) );
        col.swap( prevCol );
    }

    return 1.0 - ( float )prevCol[len2] / ( len1 + len2 );
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
    debug() << "\thost:\t\t" << mb_host;
    debug() << "\tport:\t\t" << mb_port;
    debug() << "\tpath prefix:\t" << mb_pathPrefix;
    debug() << "\tusername:\t" << mb_username;
    debug() << "\tpassword:\t" << mb_password;

    net = The::networkAccessManager();

    m_timer = new QTimer( this );
    m_timer->setInterval( 1000 );

    connect( net, &QNetworkAccessManager::authenticationRequired,
             this, &MusicBrainzFinder::gotAuthenticationRequest );
    connect( net, &QNetworkAccessManager::finished,
             this, &MusicBrainzFinder::gotReply );
    connect( m_timer, &QTimer::timeout, this, &MusicBrainzFinder::sendNewRequest );
}

bool
MusicBrainzFinder::isRunning() const
{
    return !( m_requests.isEmpty() && m_replies.isEmpty() &&
              m_parsers.isEmpty() ) || m_timer->isActive();
}

void
MusicBrainzFinder::run( const Meta::TrackList &tracks )
{
    foreach( const Meta::TrackPtr &track, tracks )
        m_requests.append( qMakePair( track, compileTrackRequest( track ) ) );

    m_timer->start();
}

void
MusicBrainzFinder::lookUpByPUID( const Meta::TrackPtr &track, const QString &puid )
{
    m_requests.append( qMakePair( track, compilePUIDRequest( puid ) ) );

    if( !m_timer->isActive() )
        m_timer->start();
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
    QPair<Meta::TrackPtr, QNetworkRequest> req = m_requests.takeFirst();
    QNetworkReply *reply = net->get( req.second );
    m_replies.insert( reply, req.first );
    connect( reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::errorOccurred),
             this, &MusicBrainzFinder::gotReplyError );

    debug() << "Request sent:" << req.second.url().toString();
}

void
MusicBrainzFinder::gotAuthenticationRequest( const QNetworkReply *reply, QAuthenticator *authenticator )
{
    if( reply->url().host() == mb_host )
    {
        authenticator->setUser( mb_username );
        authenticator->setPassword( mb_password );
    }
}

void
MusicBrainzFinder::gotReplyError( QNetworkReply::NetworkError code )
{
    DEBUG_BLOCK
    QNetworkReply *reply = qobject_cast<QNetworkReply *>( sender() );
    if( !reply )
        return;

    if( !m_replies.contains( reply ) || code == QNetworkReply::NoError )
        return;

    debug() << "Error occurred during network request:" << reply->errorString();
    disconnect( reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::errorOccurred),
                this, &MusicBrainzFinder::gotReplyError );

    // Send an empty result to populate the tagger.
    sendTrack( m_replies.value( reply ), QVariantMap() );

    m_replies.remove( reply );
    reply->deleteLater();
    checkDone();
}

void
MusicBrainzFinder::gotReply( QNetworkReply *reply )
{
    DEBUG_BLOCK
    if( m_replies.contains( reply ) )
    {
        if( reply->error() == QNetworkReply::NoError )
        {
            QString document( reply->readAll() );
            MusicBrainzXmlParser *parser = new MusicBrainzXmlParser( document );
            m_parsers.insert( parser, m_replies.value( reply ) );

            connect( parser, &MusicBrainzXmlParser::done,
                     this, &MusicBrainzFinder::parsingDone );
            ThreadWeaver::Queue::instance()->enqueue( QSharedPointer<ThreadWeaver::Job>(parser) );
        }
        else
            /*
             * Send an empty result to populate the tagger. In theory, this part should
             * never be reachable, but you never know.
             */
            sendTrack( m_replies.value( reply ), QVariantMap() );
    }

    m_replies.remove( reply );
    reply->deleteLater();
    checkDone();
}

void
MusicBrainzFinder::parsingDone( ThreadWeaver::JobPointer _parser )
{
    DEBUG_BLOCK
    MusicBrainzXmlParser *parser = dynamic_cast<MusicBrainzXmlParser*>( _parser.data() );
    disconnect( parser, &MusicBrainzXmlParser::done,
                this, &MusicBrainzFinder::parsingDone );

    if( m_parsers.contains( parser ) && !m_parsers.value( parser ).isNull() )
    {
        // When m_parsers.value( parser ) is not empty, we've been parsing tracks.
        Meta::TrackPtr trackPtr = m_parsers.value( parser );
        bool found = false;

        Q_EMIT progressStep();
        if( parser->type() == MusicBrainzXmlParser::TrackList &&
            !parser->tracks.isEmpty() )
        {
            QVariantMap metadata = m_parsedMetadata.value( trackPtr );

            QString scoreType = MusicBrainz::MUSICBRAINZ;
            // Maximum allowed error in track length (seconds).
            qlonglong lengthTolerance = 30;

            // If there is no parsed metadata, a fingerprint lookup was done.
            if( !m_parsedMetadata.contains( trackPtr ) )
            {
                scoreType = MusicBrainz::MUSICDNS;
                lengthTolerance = 10;
            }

            lengthTolerance *= 1000;
            foreach( QVariantMap track, parser->tracks.values() )
            {
#define SIMILARITY( k ) similarity( metadata.value( k ).toString().toLower(), \
                                    track.value( k ).toString().toLower() )
                if( track.value( Meta::Field::SCORE ).toInt() < 50 )
                    continue;

                QString title = track.value( Meta::Field::TITLE ).toString();
                qlonglong length = track.value( Meta::Field::LENGTH ).toLongLong();
                float score = 0;
                int maxScore = 0;

                /*
                 * We don't check for the entry to exist because a result without an
                 * artist deserves a bad score.
                 */
                if( metadata.contains( Meta::Field::ARTIST ) )
                {
                    score += 6.0 * SIMILARITY( Meta::Field::ARTIST );
                    maxScore += 6;
                }

                if( track.contains( MusicBrainz::RELEASELIST ) )
                {
                    // We try to send as many tracks as are the related releases.
                    foreach( const QString &releaseID,
                             track.value( MusicBrainz::RELEASELIST ).toStringList() )
                    {
                        /*
                         * The album artist could be parsed and inserted here, but since
                         * we have to parse each release group (only once), it's better to
                         * do it later, as we don't need it to calculate the score anyway.
                         * The release date has to be fetched in the second round
                         * (actually, it's the real reason behind the second round), as we
                         * want it to be the first release date of the release group:
                         * http://tickets.musicbrainz.org/browse/SEARCH-218
                         */
                        QVariantMap release = parser->releases.value( releaseID );
                        float releaseScore = score;
                        int maxReleaseScore = maxScore;

                        track.insert( MusicBrainz::RELEASEID, releaseID );
                        track.insert( MusicBrainz::RELEASEGROUPID,
                                      release.value( MusicBrainz::RELEASEGROUPID ) );

                        track.insert( Meta::Field::ALBUM,
                                      release.value( Meta::Field::TITLE ) );
                        if( metadata.contains( Meta::Field::ALBUM ) )
                        {
                            releaseScore += 12.0 * SIMILARITY( Meta::Field::ALBUM );
                            maxReleaseScore += 12;
                        }

                        int trackCount = release.value( MusicBrainz::TRACKCOUNT ).toInt();
                        if( trackCount > 0 )
                            track.insert( MusicBrainz::TRACKCOUNT, trackCount );
                        else
                            track.remove( MusicBrainz::TRACKCOUNT );

                        /*
                         * A track can appear more than once in a release (on different
                         * discs, or in different versions), but we're going to send it
                         * multiple times only if it has different properties per
                         * iteration (yes, if the properties below are defined, at least
                         * some of them must be different by design). Otherwise, it would
                         * result in duplicated entries (which is bad for several
                         * reasons).
                         */
                        foreach( const QVariant &info,
                                 track.value( MusicBrainz::TRACKINFO ).toMap().value( releaseID ).toList() )
                        {
                            QVariantMap trackInfo = info.toMap();
                            float currentReleaseScore = releaseScore;
                            int maxCurrentReleaseScore = maxReleaseScore;

                            /*
                             * Track title and length can be different on different
                             * releases.
                             */
                            QString currentTitle = trackInfo.value( Meta::Field::TITLE ).toString();
                            if( currentTitle.isEmpty() )
                                currentTitle = title;
                            track.insert( Meta::Field::TITLE, currentTitle );
                            // Same logic as for the artist tag above.
                            if( metadata.contains( Meta::Field::TITLE ) )
                            {
                                currentReleaseScore += 22.0 * SIMILARITY( Meta::Field::TITLE );
                                maxCurrentReleaseScore += 22;
                            }

                            qlonglong currentLength = trackInfo.value( Meta::Field::LENGTH ).toLongLong();
                            if( currentLength <= 0 )
                                currentLength = length;
                            if( currentLength > 0 )
                                track.insert( Meta::Field::LENGTH, currentLength );
                            else
                                track.remove( Meta::Field::LENGTH );
                            if( track.contains( Meta::Field::LENGTH ) )
                            {
                                currentReleaseScore += 8.0 * ( 1.0 - float( qMin( qAbs( trackPtr->length() -
                                                       track.value( Meta::Field::LENGTH ).toLongLong() ),
                                                       lengthTolerance ) ) / lengthTolerance );
                                maxCurrentReleaseScore += 8;
                            }

                            int currentDiscNumber = trackInfo.value( Meta::Field::DISCNUMBER ).toInt();
                            if( currentDiscNumber > 0 )
                                track.insert( Meta::Field::DISCNUMBER, currentDiscNumber );
                            else
                                track.remove( Meta::Field::DISCNUMBER );
                            if( metadata.contains( Meta::Field::DISCNUMBER ) &&
                                track.contains( Meta::Field::DISCNUMBER ) )
                            {
                                currentReleaseScore += ( metadata.value( Meta::Field::DISCNUMBER ).toInt() ==
                                                       track.value( Meta::Field::DISCNUMBER ).toInt() )? 6 : 0;
                                maxCurrentReleaseScore += 6;
                            }
                            else if( metadata.value( Meta::Field::DISCNUMBER ).toInt() !=
                                     track.value( Meta::Field::DISCNUMBER ).toInt() )
                                /*
                                 * Always prefer results with matching disc number,
                                 * even when empty.
                                 */
                                currentReleaseScore -= 0.1;

                            int currentTrackNumber = trackInfo.value( Meta::Field::TRACKNUMBER ).toInt();
                            if( currentTrackNumber > 0 )
                                track.insert( Meta::Field::TRACKNUMBER, currentTrackNumber );
                            else
                                track.remove( Meta::Field::TRACKNUMBER );
                            if( metadata.contains( Meta::Field::TRACKNUMBER ) &&
                                track.contains( Meta::Field::TRACKNUMBER ) )
                            {
                                currentReleaseScore += ( metadata.value( Meta::Field::TRACKNUMBER ).toInt() ==
                                                       track.value( Meta::Field::TRACKNUMBER ).toInt() )? 6 : 0;
                                maxCurrentReleaseScore += 6;
                            }
                            else if( metadata.value( Meta::Field::TRACKNUMBER ).toInt() !=
                                     track.value( Meta::Field::TRACKNUMBER ).toInt() )
                                /*
                                 * Always prefer results with matching track number,
                                 * even when empty.
                                 */
                                currentReleaseScore -= 0.1;

                            if( maxCurrentReleaseScore <= 0 )
                                continue;

                            float sim = currentReleaseScore / maxCurrentReleaseScore;
                            if( sim > MusicBrainz::MINSIMILARITY )
                            {
                                found = true;
                                track.insert( scoreType, sim );
                                sendTrack( trackPtr, track );
                            }
                        }
                    }
                }
                else
                {
                    // A track without releases has been found (not too rare).
                    if( metadata.contains( Meta::Field::TITLE ) )
                    {
                        score += 22.0 * SIMILARITY( Meta::Field::TITLE );
                        maxScore += 22;
                    }

                    if( track.contains( Meta::Field::LENGTH ) )
                    {
                        score += 8.0 * ( 1.0 - float( qMin( qAbs( trackPtr->length() -
                                 track.value( Meta::Field::LENGTH ).toLongLong() ),
                                 lengthTolerance ) ) / lengthTolerance );
                        maxScore += 8;
                    }

                    if( maxScore <= 0 )
                        continue;

                    float sim = score / maxScore;
                    if( sim > MusicBrainz::MINSIMILARITY )
                    {
                        found = true;
                        track.insert( scoreType, sim );
                        sendTrack( trackPtr, track );
                    }
                }
#undef SIMILARITY
            }
            m_parsedMetadata.remove( trackPtr );
        }
        else if( parser->type() != MusicBrainzXmlParser::TrackList )
            debug() << "Invalid parsing result.";

        /*
         * Sending an empty result is important: it creates a disabled entry in the tagger
         * to show that the track was not found (otherwise, it would pass unnoticed).
         */
        if( !found )
            sendTrack( trackPtr, QVariantMap() );
    }
    else if( parser->type() == MusicBrainzXmlParser::ReleaseGroup &&
             !parser->releaseGroups.isEmpty() )
    {
        // Cache the release group and flush the queue of tracks.
        QString releaseGroupID = parser->releaseGroups.keys().first();
        mb_releaseGroups.insert( releaseGroupID,
                                 parser->releaseGroups.value( releaseGroupID ) );
        foreach( const TrackInfo &trackInfo, mb_queuedTracks.value( releaseGroupID ) )
            sendTrack( trackInfo.first, trackInfo.second );
        mb_queuedTracks.remove( releaseGroupID );
    }

    m_parsers.remove( parser );
    parser->deleteLater();
    checkDone();
}


void
MusicBrainzFinder::sendTrack( const Meta::TrackPtr &track, QVariantMap tags )
{
    if( !tags.isEmpty() )
    {
        if( tags.contains( MusicBrainz::RELEASEGROUPID ) )
        {
            QString releaseGroupID = tags.value( MusicBrainz::RELEASEGROUPID ).toString();
            if( mb_releaseGroups.contains( releaseGroupID ) )
            {
                QVariantMap releaseGroup = mb_releaseGroups.value( releaseGroupID );
                if( releaseGroup.contains( Meta::Field::ARTIST ) )
                    tags.insert( Meta::Field::ALBUMARTIST,
                                 releaseGroup.value( Meta::Field::ARTIST ) );
                else if( tags.contains( Meta::Field::ARTIST ) )
                    tags.insert( Meta::Field::ALBUMARTIST,
                                 tags.value( Meta::Field::ARTIST ) );
                if( releaseGroup.contains( Meta::Field::YEAR ) )
                    tags.insert( Meta::Field::YEAR,
                                 releaseGroup.value( Meta::Field::YEAR ) );
            }
            else
            {
                /*
                 * The tags reference a release group we don't know yet. Queue the track
                 * and fetch information about the release group.
                 */
                if( !mb_queuedTracks.contains( releaseGroupID ) )
                {
                    QList<TrackInfo> trackList;
                    trackList.append( qMakePair( track, tags ) );
                    mb_queuedTracks.insert( releaseGroupID, trackList );
                    m_requests.prepend( qMakePair( Meta::TrackPtr(),
                                                   compileReleaseGroupRequest( releaseGroupID ) ) );
                }
                else
                    mb_queuedTracks[releaseGroupID].append( qMakePair( track, tags ) );

                return;
            }
        }

        // Clean metadata from unused fields.
        tags.remove( Meta::Field::LENGTH );
        tags.remove( Meta::Field::SCORE );
        tags.remove( MusicBrainz::RELEASELIST );
        tags.remove( MusicBrainz::TRACKINFO );
    }

    Q_EMIT trackFound( track, tags );
}

void
MusicBrainzFinder::checkDone()
{
    if( m_requests.isEmpty() && m_replies.isEmpty() && m_parsers.isEmpty() )
    {
        /*
         * Empty the queue of tracks waiting for release group requests. If the requests
         * fail (hint: network failure), remaining queued tracks will silently disappear.
         * Sending an empty result makes the user aware of the fact that the track will
         * not be tagged.
         */
        foreach( const QList<TrackInfo> &trackInfoList,
                 mb_queuedTracks.values() )
            foreach( const TrackInfo &trackInfo, trackInfoList )
                sendTrack( trackInfo.first, QVariantMap() );

        debug() << "There is no queued request. Stopping timer.";
        m_timer->stop();
        Q_EMIT done();
    }
}

QVariantMap
MusicBrainzFinder::guessMetadata( const Meta::TrackPtr &track ) const
{
    DEBUG_BLOCK
    debug() << "Trying to guess metadata from filename:" << track->playableUrl().fileName();
    QVariantMap metadata;

    if( ( track->artist().isNull() || track->artist()->name().isEmpty() ) &&
        ( track->album().isNull() || track->album()->name().isEmpty() ) )
    {
        Meta::FieldHash tags = Meta::Tag::TagGuesser::guessTags( track->playableUrl().fileName() );
        foreach( const quint64 &key, tags.keys() )
        {
            switch( key )
            {
            case Meta::valAlbum:
                metadata.insert( Meta::Field::ALBUM, tags[key] );
                break;
            case Meta::valAlbumArtist:
                metadata.insert( Meta::Field::ALBUMARTIST, tags[key] );
                break;
            case Meta::valArtist:
                metadata.insert( Meta::Field::ARTIST, tags[key] );
                break;
            case Meta::valDiscNr:
                metadata.insert( Meta::Field::DISCNUMBER, tags[key] );
                break;
            case Meta::valTitle:
                metadata.insert( Meta::Field::TITLE, tags[key] );
                break;
            case Meta::valTrackNr:
                metadata.insert( Meta::Field::TRACKNUMBER, tags[key] );
                break;
            }
        }
    }
    else
        metadata.insert( Meta::Field::TITLE, track->name() );

    if( !track->album().isNull() && !track->album()->name().isEmpty() )
        metadata.insert( Meta::Field::ALBUM, track->album()->name() );
    if( !track->artist().isNull() && !track->artist()->name().isEmpty() )
        metadata.insert( Meta::Field::ARTIST, track->artist()->name() );
    if( track->discNumber() > 0 )
        metadata.insert( Meta::Field::DISCNUMBER, track->discNumber() );
    if( track->trackNumber() > 0 )
        metadata.insert( Meta::Field::TRACKNUMBER, track->trackNumber() );

    debug() << "Guessed track info:";
    foreach( const QString &tag, metadata.keys() )
        debug() << '\t' << tag << ":\t" << metadata.value( tag ).toString();

    return metadata;
}

QNetworkRequest
MusicBrainzFinder::compileTrackRequest( const Meta::TrackPtr &track )
{
    QString queryString;
    QVariantMap metadata = guessMetadata( track );

    // These characters are not considered in the query, and some of them can break it.
    QRegularExpression unsafe( "[.,:;!?()\\[\\]{}\"]" );
    // http://lucene.apache.org/core/old_versioned_docs/versions/3_4_0/queryparsersyntax.html#Escaping Special Characters
    QRegularExpression special( "([+\\-!(){}\\[\\]\\^\"~*?:\\\\]|&&|\\|\\|)" );
    QString escape( "\\\\1" );
    // We use fuzzy search to bypass typos and small mistakes.
    QRegularExpression endOfWord( "([a-zA-Z0-9])(\\s|$)" );
    QString fuzzy( "\\1~\\2" );
    /*
     * The query results in:
     * ("track~ title~"^20 track~ title~) AND artist:("artist~ name~"^2 artist~ name~) AND release:("album~ name~"^7 album~ name~)
     * Phrases inside quotes are searched as is (and they're given precedence with the ^N
     * - where N was found during tests), with the ~ having absolutely no effect (so we
     * don't bother removing it). Words outside quotes have a OR logic: this can throw in
     * some bad results, but helps a lot with very bad tagged tracks.
     * We might be tempted to search also by qdur (quantized duration), but this has
     * proved to exclude lots of good results.
     */
#define VALUE( k ) metadata.value( k ).toString().remove( unsafe ).replace( special, escape ).replace( endOfWord, fuzzy )
    if( metadata.contains( Meta::Field::TITLE ) )
        queryString += QString( "(\"%1\"^20 %1)" ).arg( VALUE( Meta::Field::TITLE ) );
    if( metadata.contains( Meta::Field::ARTIST ) )
        queryString += QString( " AND artist:(\"%1\"^2 %1)" ).arg( VALUE( Meta::Field::ARTIST ) );
    if( metadata.contains( Meta::Field::ALBUM ) )
        queryString += QString( " AND release:(\"%1\"^7 %1)" ).arg( VALUE( Meta::Field::ALBUM ) );
#undef VALUE

    m_parsedMetadata.insert( track, metadata );

    QUrl url;
    QUrlQuery query;
    url.setPath( mb_pathPrefix + "/recording" );
    query.addQueryItem( "limit", "10" );
    query.addQueryItem( "query", queryString );
    url.setQuery( query );

    return compileRequest( url );
}

QNetworkRequest
MusicBrainzFinder::compilePUIDRequest( const QString &puid )
{
    QUrl url;
    QUrlQuery query;
    url.setPath( mb_pathPrefix + "/recording" );
    query.addQueryItem( "query", "puid:" + puid );
    url.setQuery( query );

    return compileRequest( url );
}

QNetworkRequest
MusicBrainzFinder::compileReleaseGroupRequest( const QString &releaseGroupID )
{
    QUrl url;
    QUrlQuery query;
    url.setPath( mb_pathPrefix + "/release-group/" + releaseGroupID );
    query.addQueryItem( "inc", "artists" );
    url.setQuery( query );

    return compileRequest( url );
}

QNetworkRequest
MusicBrainzFinder::compileRequest( QUrl &url )
{
    url.setScheme( "http" );
    url.setHost( mb_host );
    url.setPort( mb_port );

    QNetworkRequest req( url );
    req.setRawHeader( "Accept", "application/xml");
    req.setRawHeader( "Connection", "Keep-Alive" );
    req.setRawHeader( "User-Agent" , "Amarok" );

    if( !m_timer->isActive() )
        m_timer->start();

    return req;
}


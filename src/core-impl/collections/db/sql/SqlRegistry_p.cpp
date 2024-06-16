/****************************************************************************************
 * Copyright (c) 2010 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#define DEBUG_PREFIX "SqlRegistryP"
#include "core/support/Debug.h"

#include "SqlRegistry_p.h"
#include "SqlMeta.h"
#include "SqlCollection.h"

void
AbstractTrackTableCommitter::commit( const QList<Meta::SqlTrackPtr> &tracks )
{
    // Note: The code is greatly inspired by the old ScanResultProcessor
    //  by jeffrai

    // Note2: The code is optimized for batch update.
    //  Reason: a single update is completely harmless and not frequent.
    //  The real difficulty is the collection scanner and it's runtime
    //  Especially with collections larger than 30000 tracks.

    if( tracks.isEmpty() )
        return;

    m_storage = tracks.first()->sqlCollection()->sqlStorage();

    // -- get the maximum size for our commit
    static int maxSize = 0;
    if( maxSize == 0 )
    {
        QStringList res = m_storage->query( "SHOW VARIABLES LIKE 'max_allowed_packet';" );
        if( res.size() < 2 || res[1].toInt() == 0 )
        {
            warning() << "Uh oh! For some reason MySQL thinks there isn't a max allowed size!";
            return;
        }
        debug() << "obtained max_allowed_packet is " << res[1];
        maxSize = res[1].toInt() / 3; //for safety, due to multibyte encoding
    }


    QStringList fields = getFields();

    const QString updateQueryStart = "UPDATE LOW_PRIORITY "+tableName()+" SET ";
    const QString insertQueryStart = "INSERT INTO "+tableName()+
        " ("+fields.join(",")+") VALUES ";

    QList< Meta::SqlTrackPtr > insertedTracks;
    QString insertQuery;
    insertQuery.reserve( 1024 ); // a sensible initial size

    for( Meta::SqlTrackPtr track : tracks )
    {
        QStringList values = getValues( track.data() );

        // -- update
        if( getId( track.data() ) > 0 )
        {
            // we just commit all values to save code complexity.
            // we would need to track the real changed fields otherwise
            QString updateQuery;
            updateQuery.reserve( 256 ); // a sensible initial size
            for( int i = 0; i < fields.count() && i < values.count(); i++ )
            {
                if( !updateQuery.isEmpty() )
                    updateQuery += ", ";
                updateQuery += fields.at( i );
                updateQuery += '=';
                updateQuery += values.at( i );
            }
            updateQuery = updateQueryStart + updateQuery +
                " WHERE id=" + QString::number( getId( track.data() ) ) + ';';
            m_storage->query( updateQuery );

        }
        else
        // -- insert
        {
            QString newValues = '(' + values.join(",") + ')';

            // - if the insertQuery is long enough, commit it.
            if( insertQueryStart.length() + insertQuery.length() + newValues.length() + 1 >= maxSize - 3 ) // ";"
            {
                // commit
                insertQuery = insertQueryStart + insertQuery + ';';
                int firstId = m_storage->insert( insertQuery, tableName() );

                // set the resulting ids
                if( firstId <= 0 )
                    warning() << "Insert failed.";
                for( int i = 0; i < insertedTracks.count(); i++ )
                    setId( const_cast<Meta::SqlTrack*>(insertedTracks.at( i ).data()),
                           firstId + i );

                insertQuery.clear();
                insertedTracks.clear();
            }

            if( !insertQuery.isEmpty() )
                insertQuery += ',';
            insertQuery += newValues;
            insertedTracks.append( track );
        }
    }

    // - insert the rest
    if( !insertQuery.isEmpty() )
    {
        // commit
        insertQuery = insertQueryStart + insertQuery + ';';
        int firstId = m_storage->insert( insertQuery, tableName() );

        // set the resulting ids
        if( firstId <= 0 )
            warning() << "Insert failed.";
        for( int i = 0; i < insertedTracks.count(); i++ )
            setId( const_cast<Meta::SqlTrack*>(insertedTracks.at( i ).data()),
                   firstId + i );

        insertQuery.clear();
        insertedTracks.clear();
    }
}


// --- some help functions for the query
QString
AbstractTrackTableCommitter::nullString( const QString &str ) const
{
    if( str.isEmpty() )
        return "NULL";
    else
        return str;
}

QString
AbstractTrackTableCommitter::nullNumber( const qint64 number ) const
{
    if( number <= 0 )
        return "NULL";
    else
        return QString::number( number );
}

QString
AbstractTrackTableCommitter::nullNumber( const int number ) const
{
    if( number <= 0 )
        return "NULL";
    else
        return QString::number( number );
}

QString
AbstractTrackTableCommitter::nullNumber( const double number ) const
{
    if( number <= 0 )
        return "NULL";
    else
        return QString::number( number );
}

QString
AbstractTrackTableCommitter::nullDate( const QDateTime &date ) const
{
    if( date.isValid() )
        return QString::number( date.toSecsSinceEpoch() );
    else
        return "NULL";
}


QString
AbstractTrackTableCommitter::escape( const QString &str ) const
{
    return '\'' + m_storage->escape( str ) + '\'';
}


// ------------ urls ---------------

QString
TrackUrlsTableCommitter::tableName()
{
    return "urls";
}

int
TrackUrlsTableCommitter::getId( Meta::SqlTrack *track )
{
    return track->m_urlId;
}

void
TrackUrlsTableCommitter::setId( Meta::SqlTrack *track, int id )
{
    track->m_urlId = id;
}

QStringList
TrackUrlsTableCommitter::getFields()
{
    QStringList result;
    result << "deviceid" << "rpath" << "directory" << "uniqueid";
    return result;
}

QStringList
TrackUrlsTableCommitter::getValues( Meta::SqlTrack *track )
{
    QStringList result;
    Q_ASSERT( track->m_deviceId != 0 && "refusing to write zero deviceId to urls table, please file a bug" );
    result << QString::number( track->m_deviceId );
    result << escape( track->m_rpath );
    Q_ASSERT( track->m_directoryId > 0 && "refusing to write non-positive directoryId to urls table, please file a bug" );
    result << nullNumber( track->m_directoryId );
    result << escape( track->m_uid );
    return result;
}


// ------------ tracks ---------------

QString
TrackTracksTableCommitter::tableName()
{
    return "tracks";
}

int
TrackTracksTableCommitter::getId( Meta::SqlTrack *track )
{
    return track->m_trackId;
}

void
TrackTracksTableCommitter::setId( Meta::SqlTrack *track, int id )
{
    track->m_trackId = id;
}

QStringList
TrackTracksTableCommitter::getFields()
{
    QStringList result;
    result << "url" << "artist" << "album" << "genre" << "composer" << "year" <<
        "title" << "comment" << "tracknumber" << "discnumber" << "bitrate" <<
        "length" << "samplerate" << "filesize" << "filetype" << "bpm" << "createdate" <<
        "modifydate" << "albumgain" << "albumpeakgain" << "trackgain" << "trackpeakgain";
    return result;
}

QStringList
TrackTracksTableCommitter::getValues( Meta::SqlTrack *track )
{
    QStringList result;
    Q_ASSERT( track->m_urlId > 0 && "refusing to write non-positive urlId to tracks table, please file a bug" );
    result << QString::number( track->m_urlId );
    result << QString::number( track->m_artist ?
            AmarokSharedPointer<Meta::SqlArtist>::staticCast( track->m_artist )->id() :
            -1 );
    result << QString::number( track->m_album ?
            AmarokSharedPointer<Meta::SqlAlbum>::staticCast( track->m_album )->id() :
            -1 );
    result << QString::number( track->m_genre ?
            AmarokSharedPointer<Meta::SqlGenre>::staticCast( track->m_genre )->id() :
            -1 );
    result << QString::number( track->m_composer ?
            AmarokSharedPointer<Meta::SqlComposer>::staticCast( track->m_composer )->id() :
            -1 );
    result << QString::number( track->m_year ?
            AmarokSharedPointer<Meta::SqlYear>::staticCast( track->m_year )->id() :
            -1 );
    result << escape( track->m_title );
    result << escape( track->m_comment );
    result << nullNumber( track->m_trackNumber );
    result << nullNumber( track->m_discNumber );
    result << nullNumber( track->m_bitrate );
    result << nullNumber( track->m_length );
    result << nullNumber( track->m_sampleRate );
    result << nullNumber( track->m_filesize );
    result << nullNumber( int(track->m_filetype) );
    result << nullNumber( track->m_bpm );
    result << nullDate( track->m_createDate );
    result << nullDate( track->m_modifyDate );
    result << QString::number( track->m_albumGain );
    result << QString::number( track->m_albumPeakGain );
    result << QString::number( track->m_trackGain );
    result << QString::number( track->m_trackPeakGain );
    return result;
}

// ------------ statistics ---------------

QString
TrackStatisticsTableCommitter::tableName()
{
    return "statistics";
}

int
TrackStatisticsTableCommitter::getId( Meta::SqlTrack *track )
{
    return track->m_statisticsId;
}

void
TrackStatisticsTableCommitter::setId( Meta::SqlTrack *track, int id )
{
    track->m_statisticsId = id;
}

QStringList
TrackStatisticsTableCommitter::getFields()
{
    QStringList result;
    result << "url" << "createdate" << "accessdate" << "score" << "rating" << "playcount" << "deleted";
    return result;
}

QStringList
TrackStatisticsTableCommitter::getValues( Meta::SqlTrack *track )
{
    QStringList result;
    Q_ASSERT( track->m_urlId > 0 && "refusing to write non-positive urlId to statistics table, please file a bug" );
    result << QString::number( track->m_urlId );
    result << nullDate( track->m_firstPlayed );
    result << nullDate( track->m_lastPlayed );
    result << nullNumber( track->m_score );
    result << QString::number( track->m_rating ); // NOT NULL
    result << QString::number( track->m_playCount ); // NOT NULL
    result << "0"; // not deleted
    return result;
}

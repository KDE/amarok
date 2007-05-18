/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/

#include "debug.h"
#include "playlistmodel.h"

#include "collection/blockingquery.h"
#include "collection/collection.h"
#include "collection/collectionmanager.h"
#include "collection/querymaker.h"


using namespace PlaylistNS;

Model::Model( QObject* parent )
    : QAbstractTableModel( parent )
{ }

int
Model::rowCount( const QModelIndex& ) const
{
    return m_tracks.size();
}

int
Model::columnCount( const QModelIndex& ) const
{
    return m_columns.size();
}

QVariant
Model::data( const QModelIndex& index, int role ) const
{
    DEBUG_BLOCK
    TrackPtr track = m_tracks.at( index.row() );
    if( role == Qt::DisplayRole )
    {
    //if you need this code like this elsewhere, don't copy & paste! transfer it into meta.cpp.
        switch( m_columns.at( index.column() ) )
        {
            case AlbumArtist: return track->album()->albumArtist()->name();
            case Album: return track->album()->name();
            case Artist: return track->artist()->name();
            case Bitrate: return track->bitrate();
            case Composer: return track->composer()->name();
            case Comment: return track->comment();
            case DiscNumber: return track->discNumber();
            case Filesize: return track->filesize();
            case Genre: return track->genre()->name();
            case Length: return track->length();
            case Rating: return track->rating();
            case Score: return track->score();
            case Title: return track->name();
            case TrackNumber: return track->trackNumber();
            case Year: return track->year()->name().toInt();
            default: return "not implemented";
        }
    }
    else 
        return QVariant();


}

// void
// insertColumns( int column, Column type )
// {
//     beginInsertColumns( QModelIndex(), column, column + 1 );
//     m_columns.insert( column, type );
//     endInsertColumns();
//     return true;
// }

void
Model::insertTracks( int row, TrackList list )
{
    beginInsertRows( QModelIndex(), row, row + list.size() );
    //m_columns.insert( row, list )
    int i = 0;
    TrackPtr track;
    foreach( track , list )
    {
        m_tracks.insert( row + i, track );
        i++;
    }
    endInsertRows();
}

bool
Model::removeRows( int position, int rows )
{
    beginRemoveRows( QModelIndex(), position, position + rows );
    TrackList::iterator start = m_tracks.begin() + position;
    TrackList::iterator end = start + rows;
    m_tracks.erase( start, end );
    endRemoveRows();
    return true;
}

QVariant
Model::headerData( int section, Qt::Orientation, int role ) const
{
    DEBUG_BLOCK
    if( role == Qt::DisplayRole && section < m_columns.size() )
    {
        debug() << "section: " << section << " enum: " << m_columns.at( section ) << " " << prettyColumnName( m_columns.at( section ) ) << endl;
        return prettyColumnName( m_columns.at( section ) );
    }
    else
        return QVariant();
}

void
Model::testData()
{
    DEBUG_BLOCK
    /*TrackList sample;
    DaapTrackPtr one( new DaapTrack( "host", 6666, "1", "1", "mp3" ) );
    one->setTitle( "hello one");
    one->setTrackNumber( 1 );
    DaapTrackPtr two( new DaapTrack( "host", 6666, "1", "2", "mp3" ) );
    two->setTitle( "hello two" );
    two->setTrackNumber( 2 );
    sample.append( TrackPtr::staticCast( one ) );
    sample.append( TrackPtr::staticCast( two ) );
    insertTracks( 0, sample );
    m_columns << TrackNumber << Title;
    reset();*/
    Collection *local = 0;
    foreach( Collection *coll, CollectionManager::instance()->collections() )
    {
        if( coll->collectionId() == "localCollection" )
            local = coll;
    }
    if( !local )
        return;
    QueryMaker *qm = local->queryBuilder();
    qm->startTrackQuery();
    qm->limitMaxResultSize( 10 );
    BlockingQuery bq( qm );
    bq.startQuery();
    insertTracks( 0, bq.tracks( "localCollection" ) );
    m_columns << TrackNumber << Title;
    reset();
}

QString
Model::prettyColumnName( Column index ) //static
{
    switch( index )
    {
        case Filename:   return i18n( "Filename"    );
        case Title:      return i18n( "Title"       );
        case Artist:     return i18n( "Artist"      );
        case AlbumArtist:return i18n( "Album Artist");
        case Composer:   return i18n( "Composer"    );
        case Year:       return i18n( "Year"        );
        case Album:      return i18n( "Album"       );
        case DiscNumber: return i18n( "Disc Number" );
        case TrackNumber:return i18n( "Track"       );
        case Bpm:        return i18n( "BPM"         );
        case Genre:      return i18n( "Genre"       );
        case Comment:    return i18n( "Comment"     );
        case Directory:  return i18n( "Directory"   );
        case Type:       return i18n( "Type"        );
        case Length:     return i18n( "Length"      );
        case Bitrate:    return i18n( "Bitrate"     );
        case SampleRate: return i18n( "Sample Rate" );
        case Score:      return i18n( "Score"       );
        case Rating:     return i18n( "Rating"      );
        case PlayCount:  return i18n( "Play Count"  );
        case LastPlayed: return i18nc( "Column name", "Last Played" );
        case Mood:       return i18n( "Mood"        );
        case Filesize:   return i18n( "File Size"   );
        default:         return "This is a bug.";
    }
    
}

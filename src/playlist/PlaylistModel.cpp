/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/

#include "PlaylistModel.h"

#include "debug.h"
#include "enginecontroller.h"
#include "StandardTrackAdvancer.h"
#include "statusbar.h"
#include "TheInstances.h"

#include "collection/blockingquery.h"
#include "collection/collection.h"
#include "collection/collectionmanager.h"
#include "collection/querymaker.h"
#include "meta/lastfm/LastFmMeta.h"

#include <kurl.h>

using namespace PlaylistNS;

Model *Model::s_instance = 0;

Model::Model( QObject* parent )
    : QAbstractTableModel( parent )
    , m_activeRow( -1 )
    , m_advancer( new StandardTrackAdvancer( this ) )
{ 
    connect( EngineController::instance(), SIGNAL( trackFinished() ), this, SLOT( trackFinished() ) );
    m_columns << TrackNumber << Title << Artist << Album;
    s_instance = this;
}

Model::~Model()
{
    delete m_advancer;
}

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
    int row = index.row();
    TrackPtr track = m_tracks.at( row );
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
    else if( ( role ==  Qt::FontRole) && ( row == m_activeRow ) )
    {
        QFont original;
        original.setBold( true );
        return original;
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
    if( !list.size() )
        return;

    beginInsertRows( QModelIndex(), row, row + list.size() - 1 );
    //m_columns.insert( row, list )
    int i = 0;
    foreach( TrackPtr track , list )
    {
        if( track )
        {
            track->subscribe( this );
            m_tracks.insert( row + i, track );
            i++;
        }
    }
    endInsertRows();
}

void
Model::insertTrack( int row, TrackPtr track )
{
    TrackList list;
    list.append( track );
    insertTracks( row, list );
}

void
Model::insertTracks( int row, QueryMaker *qm )
{
    qm->startTrackQuery();
    connect( qm, SIGNAL( queryDone() ), SLOT( queryDone() ) );
    connect( qm, SIGNAL( newResultReady( QString, Meta::TrackList ) ), SLOT( newResultReady( QString, Meta::TrackList ) ) );
    m_queryMap.insert( qm, row );
    qm->run();
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
    Collection *local = 0;
    foreach( Collection *coll, CollectionManager::instance()->collections() )
    {
        if( coll->collectionId() == "localCollection" )
            local = coll;
    }
    if( !local )
        return;
    QueryMaker *qm = local->queryMaker();
    qm->startTrackQuery();
    qm->limitMaxResultSize( 10 );
    BlockingQuery bq( qm );
    bq.startQuery();
    insertTracks( 0, bq.tracks( "localCollection" ) );
    //m_columns << TrackNumber << Title << Artist << Album;
    reset();
}

Qt::DropActions
Model::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}

void
Model::trackFinished()
{
    Meta::TrackPtr track = m_tracks.at( m_activeRow );
    track->finishedPlaying( 1.0 ); //TODO: get correct value for parameter
    m_advancer->advanceTrack();
}

void
Model::play( const QModelIndex& index )
{
    play( index.row() );
}

void
Model::play( int row )
{
    setActiveRow( row );
    EngineController::instance()->play( m_tracks[ m_activeRow ] );
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


void
Model::setActiveRow( int row )
{
DEBUG_BLOCK
    int max = qMax( row, m_activeRow );
    int min = qMin( row, m_activeRow );
    if( ( max - min ) == 1 )
        emit dataChanged( createIndex( min, 0 ), createIndex( max, columnCount() -1 ) );
    else
    {
        emit dataChanged( createIndex( min, 0 ), createIndex( min, columnCount() - 1 ) );
        emit dataChanged( createIndex( max, 0 ), createIndex( max, columnCount() - 1 ) );
    }
    debug() << "between " << min << " and " << max << endl;
    m_activeRow = row;
}

void
Model::metadataChanged( Meta::Track *track )
{
    int index = m_tracks.indexOf( Meta::TrackPtr( track ), 0 );
    if( index != -1 )
        emit dataChanged( createIndex( index, 0 ), createIndex( index, columnCount() -1 ) );
}

void
Model::clear()
{
    m_tracks.clear();
    reset();
}

void
Model::insertOptioned( Meta::TrackList list, int options )
{

    if( list.isEmpty() ) {
        Amarok::StatusBar::instance()->shortMessage( i18n("Attempted to insert nothing into playlist.") );
        return; // don't add empty items
    }

    if( Unique )
    {
        int alreadyOnPlaylist = 0;
        for( int i = 0; i < list.size(); ++i )
        {
            if( m_tracks.contains( list.at( i ) ) )
            {
                list.removeAt( i );
                alreadyOnPlaylist++;
            }
        }
        if ( alreadyOnPlaylist )
            Amarok::StatusBar::instance()->shortMessage( i18np("One track was already in the playlist, so it was not added.", "%1 tracks were already in the playlist, so they were not added.", alreadyOnPlaylist ) );
    }
    int firstItemAdded = -1;
    if( options & Replace )
    {
        clear();
        firstItemAdded = 0;
        insertTracks( 0, list );
    }
    else if( options & Append )
    {
        firstItemAdded = rowCount();
        insertTracks( firstItemAdded, list );
    }
    else if( options & Queue )
    {
        //TODO implement queue
    }
    if( options & DirectPlay )
    {
        play( firstItemAdded );
    }
    else if( ( options & StartPlay ) && ( EngineController::engine()->state() != Engine::Playing ) )
    {
        play( firstItemAdded );
    }
}

void
Model::insertOptioned( Meta::TrackPtr track, int options )
{
    Meta::TrackList list;
    list.append( track );
    insertOptioned( list, options );
}

void
Model::insertOptioned( QueryMaker *qm, int options )
{
    qm->startTrackQuery();
    connect( qm, SIGNAL( queryDone() ), SLOT( queryDone() ) );
    connect( qm, SIGNAL( newResultReady( QString, Meta::TrackList ) ), SLOT( newResultReady( QString, Meta::TrackList ) ) );
    m_optionedQueryMap.insert( qm, options );
    qm->run();
}

void
Model::insertMedia( KUrl::List list, int options )
{
    KUrl url;
    Meta::TrackList trackList;
    foreach( url, list )
    {
        Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( url );
        if( track )
            trackList.push_back( track );
    }
    insertOptioned( trackList, options );
}
////////////
//Private Slots
///////////
void
Model::queryDone()
{
    QueryMaker *qm = dynamic_cast<QueryMaker*>( sender() );
    if( qm )
    {
        m_queryMap.remove( qm );
        qm->deleteLater();
    }
}

void
Model::newResultReady( const QString &collectionId, const Meta::TrackList &tracks )
{
    Q_UNUSED( collectionId )
    QueryMaker *qm = dynamic_cast<QueryMaker*>( sender() );
    if( qm )
    {
        //requires better handling of queries which return multiple results
        if( m_queryMap.contains( qm ) )
            insertTracks( m_queryMap.value( qm ), tracks );
        else if( m_optionedQueryMap.contains( qm ) )
            insertOptioned( tracks, m_optionedQueryMap.value( qm ) );
    }
}

namespace The {
    PlaylistNS::Model* playlistModel() { return PlaylistNS::Model::s_instance; } 
}

#include "PlaylistModel.moc"

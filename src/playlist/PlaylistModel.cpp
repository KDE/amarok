/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/

#include "PlaylistModel.h"

#include "amarok.h"
#include "debug.h"
#include "enginecontroller.h"
#include "StandardTrackAdvancer.h"
#include "statusbar.h"
#include "TheInstances.h"
#include "UndoCommands.h"

#include "collection/blockingquery.h"
#include "collection/collection.h"
#include "collection/collectionmanager.h"
#include "collection/querymaker.h"
#include "meta/lastfm/LastFmMeta.h"
#include "AmarokMimeData.h"

#include <QAction>
#include <QItemSelectionModel>
#include <QStringList>
#include <QUndoStack>


#include <KIcon>
#include <KUrl>

using namespace PlaylistNS;

Model *Model::s_instance = 0;

Model::Model( QObject* parent )
    : QAbstractListModel( parent )
    , m_activeRow( -1 )
    , m_advancer( new StandardTrackAdvancer( this ) )
    , m_undoStack( new QUndoStack( this ) )
    , m_selectionModel( new QItemSelectionModel( this ) )
{ 
    connect( EngineController::instance(), SIGNAL( trackFinished() ), this, SLOT( trackFinished() ) );
    s_instance = this;
}

void
Model::init()
{
    KActionCollection* ac = Amarok::actionCollection();
    QAction* undoButton  = m_undoStack->createUndoAction( this, i18n("Undo") );
    undoButton->setIcon( KIcon( Amarok::icon( "undo" ) ) );
    ac->addAction("playlist_undo", undoButton);
    QAction* redoButton  = m_undoStack->createRedoAction( this, i18n("Redo") );
    ac->addAction("playlist_redo", redoButton);
    redoButton->setIcon( KIcon( Amarok::icon( "redo" ) ) );
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

QVariant
Model::data( const QModelIndex& index, int role ) const
{
    int row = index.row();
    TrackPtr track = m_tracks.at( row );
    /*if( ( role ==  Qt::FontRole) && ( row == m_activeRow ) )
    {
        QFont original;
        original.setBold( true );
        return original;
    }
    else*/ 
    if( role == TrackRole && ( row != -1 ) && track )
    {
        return QVariant::fromValue( track );
    }
    else if( role == Qt::DisplayRole && row != -1 )
    {
        return track->name();
    }
    else
    {
        return QVariant();
    }
/*        switch( role )
        {
            case AlbumArtist: return track->album()->albumArtist()->name();
            case Album: return track->album()->name();
            case Artist: return track->artist()->name();
            case Bitrate: return track->bitrate();
            case Composer: return track->composer()->name();
            case CoverImage: return track->album()->image( 50 );
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
            default: return QVariant();
        } */
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
Model::insertTrack( int row, TrackPtr track )
{
    TrackList list;
    list.append( track );
    insertTracks( row, list );
}

void
Model::insertTracks( int row, TrackList tracks )
{
    m_undoStack->push( new AddTracksCmd( 0, row, tracks ) );
}

bool
Model::removeRows( int position, int rows, const QModelIndex& /*parent*/  )
{
    m_undoStack->push( new RemoveTracksCmd( 0, position, rows ) );
    return true;
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
    reset();
}

Qt::DropActions
Model::supportedDropActions() const
{
    DEBUG_BLOCK
    return Qt::ActionMask;
    //return Qt::CopyAction | Qt::MoveAction | Qt::LinkAction;
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
        emit dataChanged( createIndex( min, 0 ), createIndex( max, 0 ) );
    else
    {
        emit dataChanged( createIndex( min, 0 ), createIndex( min, 0 ) );
        emit dataChanged( createIndex( max, 0 ), createIndex( max, 0 ) );
    }
    debug() << "between " << min << " and " << max << endl;
    m_activeRow = row;
}

void
Model::metadataChanged( Meta::Track *track )
{
    int index = m_tracks.indexOf( Meta::TrackPtr( track ), 0 );
    if( index != -1 )
        emit dataChanged( createIndex( index, 0 ), createIndex( index, 0 ) );
}

void
Model::clear()
{
    removeRows( 0, m_tracks.size() );
    m_activeRow = -1;
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
    Amarok::actionCollection()->action( "playlist_clear" )->setEnabled( !m_tracks.isEmpty() );
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

bool
Model::saveM3U( const QString &path, bool relative ) const
{
//     Q3ValueList<KUrl> urls;
//     Q3ValueList<QString> titles;
//     Q3ValueList<int> lengths;
//     for( MyIt it( firstChild(), MyIt::Visible ); *it; ++it )
//     {
//         urls << (*it)->url();
//         titles << (*it)->title();
//         lengths << (*it)->length();
//     }
//     return PlaylistBrowser::savePlaylist( path, urls, titles, lengths, relative );
    return false;
}

Qt::ItemFlags
Model::flags(const QModelIndex &index) const
{
    Qt::ItemFlags defaultFlags = QAbstractListModel::flags(index);
    return Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsEnabled | defaultFlags;
}

QStringList
Model::mimeTypes() const
{
    DEBUG_BLOCK
    //QStringList ret = QAbstractTableModel::mimeTypes();
    QStringList ret;
    ret << AmarokMimeData::TRACK_MIME;
    debug() << ret << endl;
    return ret;
}

QMimeData*
Model::mimeData( const QModelIndexList &indexes ) const
{
    DEBUG_BLOCK
    AmarokMimeData* mime = new AmarokMimeData();
    
    Meta::TrackList selectedTracks;

    foreach( QModelIndex it, m_selectionModel->selectedIndexes() )
        selectedTracks << m_tracks.at( it.row() );

    mime->setTracks( selectedTracks );
    return mime;
}

bool
Model::dropMimeData ( const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent )
{
    DEBUG_BLOCK

    if( action == Qt::IgnoreAction )
        return true;

    if( data->hasFormat( AmarokMimeData::TRACK_MIME ) )
    {
        debug() << "Found track mime type" << endl;

        const AmarokMimeData* trackListDrag = dynamic_cast<const AmarokMimeData*>( data );
        if( trackListDrag )
        {
            debug() << "It's a list drag!" << endl;
            if( row < 0 )
            {
                debug() << "Inserting at row: " << row << " so we're appending to the list." << endl;
                insertOptioned( trackListDrag->tracks(), PlaylistNS::Append );
            }
            else
            {
                debug() << "Inserting at row: " << row <<" so its inserted correctly." << endl;
                insertTracks( row, trackListDrag->tracks() );
            }
            return true;
        }
    }
    else if( data->hasUrls() )
    {
        //probably a drop from an external source
        debug() << "Drop from external source" << endl;
        QList<QUrl> urls = data->urls();
        Meta::TrackList tracks;
        foreach( QUrl url, urls )
        {
            Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( KUrl( url ) );
            if( track )
                tracks.append( track );
        }
        if( !tracks.isEmpty() )
            insertOptioned( tracks, PlaylistNS::Append );
        return true;
    }
    return false;
}

////////////
//Private Methods
///////////


void
Model::insertTracksCommand( int row, TrackList list )
{
    DEBUG_BLOCK 
    debug() << "inserting... " << row << ' ' << list.count() << endl;
    if( !list.size() )
        return;

    beginInsertRows( QModelIndex(), row, row + list.size() - 1 );
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
    //push up the active row if needed
    if( m_activeRow > row )
    {
        int oldActiveRow = m_activeRow;
        m_activeRow += list.size();
        //dataChanged( createIndex( oldActiveRow, 0 ), createIndex( oldActiveRow, columnCount() -1 ) );
        //dataChanged( createIndex( m_activeRow, 0 ), createIndex( m_activeRow, columnCount() -1 ) );
    }
    dataChanged( createIndex( row, 0 ), createIndex( rowCount() - 1, 0 ) );
    Amarok::actionCollection()->action( "playlist_clear" )->setEnabled( !m_tracks.isEmpty() );
}


TrackList
Model::removeRowsCommand( int position, int rows )
{
    DEBUG_BLOCK

    beginRemoveRows( QModelIndex(), position, position + rows - 1 );
//     TrackList::iterator start = m_tracks.begin() + position;
//     TrackList::iterator end = start + rows;
//     m_tracks.erase( start, end );
    TrackList ret;
    for( int i = position; i < position + rows; i++ )
    {
        TrackPtr track = m_tracks.takeAt( position ); //take at position, row times
        track->unsubscribe( this );
        ret.push_back( track );
    }
    endRemoveRows();

    Amarok::actionCollection()->action( "playlist_clear" )->setEnabled( !m_tracks.isEmpty() );

    //update m_activeRow
    bool activeRowChanged = true;
    bool oldActiveRow = m_activeRow;
    if( m_activeRow >= position && m_activeRow < ( position + rows ) )
        m_activeRow = -1;
    else if( m_activeRow >= position )
        m_activeRow = m_activeRow - position;
    else
        activeRowChanged = false;
    if( activeRowChanged )
    {
        dataChanged( createIndex( oldActiveRow, 0 ), createIndex( oldActiveRow, columnCount() -1 ) );
        dataChanged( createIndex( m_activeRow, 0 ), createIndex( m_activeRow, columnCount() -1 ) );
    }
    dataChanged( createIndex( position, 0 ), createIndex( rowCount(), 0 ) );
    return ret;
}

void
Model::queryDone() //Slot
{
    QueryMaker *qm = dynamic_cast<QueryMaker*>( sender() );
    if( qm )
    {
        m_queryMap.remove( qm );
        qm->deleteLater();
    }
}

void
Model::newResultReady( const QString &collectionId, const Meta::TrackList &tracks ) //Slot
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

/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/

#include "PlaylistModel.h"

#include "amarok.h"
#include "AmarokMimeData.h"
#include "debug.h"
#include "enginecontroller.h"
#include "PlaylistItem.h"
#include "RepeatTrackNavigator.h"
#include "StandardTrackNavigator.h"
#include "statusbar.h"
#include "TheInstances.h"
#include "UndoCommands.h"

#include "collection/BlockingQuery.h"
#include "collection/Collection.h"
#include "collection/CollectionManager.h"
#include "collection/QueryMaker.h"
#include "meta/lastfm/LastFmMeta.h"

#include <QAction>
#include <QStringList>
#include <QUndoStack>

#include <KIcon>
#include <KUrl>

using namespace Playlist;
using namespace Meta;

Model *Model::s_instance = 0;

Model::Model( QObject* parent )
    : QAbstractListModel( parent )
    , m_activeRow( -1 )
    , m_advancer( new StandardTrackNavigator( this ) )
    , m_undoStack( new QUndoStack( this ) )
    , m_playlistLoader ( new PlaylistLoader )
{
    connect( EngineController::instance(), SIGNAL( trackFinished() ), this, SLOT( trackFinished() ) );
    connect( EngineController::instance(), SIGNAL( orderCurrent() ), this, SLOT( playCurrentTrack() ) );
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
    delete m_playlistLoader;
}

int
Model::rowCount( const QModelIndex& ) const
{
    return m_items.size();
}

QVariant
Model::data( const QModelIndex& index, int role ) const
{
    int row = index.row();
    /*if( ( role ==  Qt::FontRole) && ( row == m_activeRow ) )
    {
        QFont original;
        original.setBold( true );
        return original;
    }
    else*/
    if( role == ItemRole && ( row != -1 ) )
    {
        return QVariant::fromValue( m_items.at( row ) );
    }
    else if( role == ActiveTrackRole )
        return ( row == m_activeRow );
    else if( role == TrackRole && ( row != -1 ) && m_items.at( row )->track() )
    {
        return QVariant::fromValue( m_items.at( row )->track() );
    }
    else if( role == Qt::DisplayRole && row != -1 )
    {
        return m_items.at( row )->track()->name();
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
    DEBUG_BLOCK
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
    return Qt::CopyAction | Qt::MoveAction;
}

void
Model::trackFinished()
{
    if( m_activeRow < 0 || m_activeRow >= m_items.size() )
        return;
    Meta::TrackPtr track = m_items.at( m_activeRow )->track();
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
    EngineController::instance()->play( m_items[ m_activeRow ]->track() );
}

void
Model::playlistRepeatMode( int item )
{
    if( item == 0 )
        playModeChanged( Playlist::Standard );
    else //for now just turn on repeat if anything but "off" is clicked
        playModeChanged( Playlist::Repeat );
}

void
Model::next()
{
    if( m_activeRow < 0 || m_activeRow >= m_items.size() )
        return;
    Meta::TrackPtr track = m_items.at( m_activeRow )->track();
    track->finishedPlaying( 0.5 ); //TODO: get correct value for parameter
    m_advancer->userAdvanceTrack();
}

void
Model::back()
{
    if( m_activeRow < 0 || m_activeRow >= m_items.size() )
        return;
    Meta::TrackPtr track = m_items.at( m_activeRow )->track();
    track->finishedPlaying( 0.5 ); //TODO: get correct value for parameter
    m_advancer->recedeTrack();
}

void
Model::playCurrentTrack()
{
    int selected = m_activeRow;
    if( selected < 0 || selected >= m_items.size() )
    {
        //play first track if there are tracks in the playlist
        if( m_items.size() )
            selected = 0;
        else
            return;
    }
    play( selected );
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
Model::playModeChanged( int row )
{
    delete m_advancer;
    switch (row)
    {
        case Playlist::Standard:
            m_advancer = new StandardTrackNavigator(this);
            break;
        case Playlist::Repeat:
            m_advancer = new RepeatTrackNavigator(this);
            break;
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
    debug() << "between " << min << " and " << max;
    m_activeRow = row;
}

void
Model::metadataChanged( Meta::Track *track )
{
    DEBUG_BLOCK
    const int size = m_items.size();
    const Meta::TrackPtr needle =  Meta::TrackPtr( track );
    for( int i = 0; i < size; i++ )
    {
        if( m_items.at( i )->track() == needle )
        {
            debug() << "Track in playlist";
            emit dataChanged( createIndex( i, 0 ), createIndex( i, 0 ) );
            break;
        }
    }

#if 0
    int index = m_tracks.indexOf( Meta::TrackPtr( track ), 0 );
    if( index != -1 )
        emit dataChanged( createIndex( index, 0 ), createIndex( index, 0 ) );
#endif
}

void 
Model::metadataChanged(Meta::Album * album)
{
    DEBUG_BLOCK
    //process each track
    TrackList tracks = album->tracks();
    foreach( TrackPtr track, tracks ) {
        metadataChanged( track.data() );
    }

}

void
Model::clear()
{
    removeRows( 0, m_items.size() );
//    m_activeRow = -1;
}

void
Model::insertOptioned( Meta::TrackList list, int options )
{

    if( list.isEmpty() ) {
        Amarok::StatusBar::instance()->shortMessage( i18n("Attempted to insert nothing into playlist.") );
        return; // don't add empty items
    }


    if( options & Unique )
    {
        int alreadyOnPlaylist = 0;
        for( int i = 0; i < list.size(); ++i )
        {

            Item* item;
            foreach( item, m_items )
            {
                if( item->track() == list.at( i ) )
                {
                    list.removeAt( i );
                    alreadyOnPlaylist++;
                    break;
                }
            }

        }
        if ( alreadyOnPlaylist )
            Amarok::StatusBar::instance()->shortMessage( i18np("One track was already in the playlist, so it was not added.", "%1 tracks were already in the playlist, so they were not added.", alreadyOnPlaylist ) );
    }

   int orgCount = rowCount(); //needed because recursion messes up counting
   bool playlistAdded = false;

   //HACK! Check if any of the incomming tracks is really a playlist. Warning, this can get highly recursive
   for( int i = 0; i < list.size(); ++i )
   {
       if ( m_playlistLoader->isPlaylist( list.at( i )->url() ) ) {
           playlistAdded = true;
           m_playlistLoader->load( list.takeAt( i )->url() );
       }
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
        if ( playlistAdded )
           firstItemAdded = orgCount;
        else 
           firstItemAdded = rowCount();

        if ( rowCount() > firstItemAdded )
            insertTracks( firstItemAdded, list );
    }
    else if( options & Queue )
    {
        //TODO implement queue
    }
    if( options & DirectPlay )
    {
        if ( rowCount() > firstItemAdded )
        play( firstItemAdded );
    }
    else if( ( options & StartPlay ) && ( EngineController::engine()->state() != Engine::Playing ) && ( rowCount() != 0 ) )
    {
        play( firstItemAdded );
    }
    Amarok::actionCollection()->action( "playlist_clear" )->setEnabled( !m_items.isEmpty() );
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
    if( trackList.isEmpty() )
        debug() << "Attempted to insert nothing into the playlist!";

    insertOptioned( trackList, options );
}

bool
Model::saveM3U( const QString &path, bool relative ) const
{
    Q_UNUSED( path ); Q_UNUSED( relative );
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
    if( index.isValid() )
    {
        return ( Qt::ItemIsEnabled     | Qt::ItemIsSelectable | Qt::ItemIsDropEnabled |
                 Qt::ItemIsDragEnabled | Qt::ItemIsSelectable );
    }
    return Qt::ItemIsDropEnabled;
}

QStringList
Model::mimeTypes() const //reimplemented
{
    QStringList ret = QAbstractListModel::mimeTypes();
    ret << AmarokMimeData::TRACK_MIME;
    return ret;
}

QMimeData*
Model::mimeData( const QModelIndexList &indexes ) const //reimplemented
{
    AmarokMimeData* mime = new AmarokMimeData();
    Meta::TrackList selectedTracks;

    foreach( QModelIndex it, indexes )
        selectedTracks << m_items.at( it.row() )->track();

    mime->setTracks( selectedTracks );
    return mime;
}

bool
Model::dropMimeData ( const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent ) //reimplemented
{
    Q_UNUSED( column ); Q_UNUSED( parent );
    DEBUG_BLOCK

    if( action == Qt::IgnoreAction )
        return true;

    if( data->hasFormat( AmarokMimeData::TRACK_MIME ) )
    {
        debug() << "Found track mime type";

        const AmarokMimeData* trackListDrag = dynamic_cast<const AmarokMimeData*>( data );
        if( trackListDrag )
        {
            if( row < 0 )
            {
                debug() << "Inserting at row: " << row << " so we're appending to the list.";
                insertOptioned( trackListDrag->tracks(), Playlist::Append );
            }
            else
            {
                debug() << "Inserting at row: " << row <<" so its inserted correctly.";
                insertTracks( row, trackListDrag->tracks() );
            }
            return true;
        }
    }
    else if( data->hasUrls() )
    {
        //probably a drop from an external source
        debug() << "Drop from external source";
        QList<QUrl> urls = data->urls();
        Meta::TrackList tracks;
        foreach( QUrl url, urls )
        {
            Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( KUrl( url ) );
            if( track )
                tracks.append( track );
        }
        if( !tracks.isEmpty() )
            insertOptioned( tracks, Playlist::Append );
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
    debug() << "inserting... " << row << ' ' << list.count();
    if( !list.size() )
        return;

    beginInsertRows( QModelIndex(), row, row + list.size() - 1 );
    int i = 0;
    foreach( TrackPtr track , list )
    {
        if( track )
        {
            track->subscribe( this );
            if( track->album() )
                track->album()->subscribe( this );
            m_items.insert( row + i, new Item( track ) );
            i++;
        }
    }
    endInsertRows();
    //push up the active row if needed
    if( m_activeRow > row )
    {
        int oldActiveRow = m_activeRow;
        m_activeRow += list.size();
        Q_UNUSED( oldActiveRow );
        //dataChanged( createIndex( oldActiveRow, 0 ), createIndex( oldActiveRow, columnCount() -1 ) );
        //dataChanged( createIndex( m_activeRow, 0 ), createIndex( m_activeRow, columnCount() -1 ) );
    }
    dataChanged( createIndex( row, 0 ), createIndex( rowCount() - 1, 0 ) );
    Amarok::actionCollection()->action( "playlist_clear" )->setEnabled( !m_items.isEmpty() );

    emit playlistCountChanged( rowCount() );
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
        Item* item = m_items.takeAt( position ); //take at position, row times
        item->track()->unsubscribe( this );
        if( item->track()->album() )
            item->track()->album()->unsubscribe( this );
        ret.push_back( item->track() );
        delete item;
    }
    endRemoveRows();

    Amarok::actionCollection()->action( "playlist_clear" )->setEnabled( !m_items.isEmpty() );

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
    emit playlistCountChanged( rowCount() );
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
    Playlist::Model* playlistModel() { return Playlist::Model::s_instance; }
}




#include "PlaylistModel.moc"

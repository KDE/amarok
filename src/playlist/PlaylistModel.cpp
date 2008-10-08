/***************************************************************************
 * copyright        : (C) 2007-2008 Ian Monroe <ian@monroe.nu>
 *                    (C) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
 *                    (C) 2008 Seb Ruiz <ruiz@kde.org>
 *                    (C) 2008 Soren Harward <stharward@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **************************************************************************/

#define DEBUG_PREFIX "Playlist::Model"

#include "PlaylistModel.h"

#include "Amarok.h"
#include "amarokconfig.h"
#include "AmarokMimeData.h"
#include "Debug.h"
#include "DirectoryLoader.h"
#include "EngineController.h"
#include "PlaylistController.h"
#include "PlaylistItem.h"
#include "PlaylistFileSupport.h"
#include "UndoCommands.h"
#include "playlistmanager/PlaylistManager.h"

#include <QAction>
#include <QStringList>
#include <QUndoStack>

#include <KFileItem>
#include <KIcon>
#include <KUrl>

#include <typeinfo>

Playlist::Model* Playlist::Model::s_instance = 0;

Playlist::Model* Playlist::Model::instance()
{
    return ( s_instance ) ? s_instance : new Model();
}

void
Playlist::Model::destroy()
{
    if ( s_instance )
    {
        delete s_instance;
        s_instance = 0;
    }
}

Playlist::Model::Model()
        : QAbstractListModel( 0 )
        , m_activeRow( -1 )
        , m_totalLength( 0 )
{
    DEBUG_BLOCK
    s_instance = this;

    if ( QFile::exists( defaultPlaylistPath() ) )
    {
        Meta::TrackList tracks = Meta::loadPlaylist( KUrl( defaultPlaylistPath() ) )->tracks();
        foreach( Meta::TrackPtr track, tracks )
        {
            m_totalLength += track->length();
            subscribeTo( track );
            if ( track->album() )
                subscribeTo( track->album() );

            Item* i = new Item( track );
            m_items.append( i );
            m_itemIds.insert( i->id(), i );
        }
    }
}

Playlist::Model::~Model()
{
    DEBUG_BLOCK

    // Save current playlist
    Meta::TrackList list;
    foreach( Item* item, m_items )
    {
        list << item->track();
    }
    The::playlistManager()->exportPlaylist( list, defaultPlaylistPath() );
}

QVariant
Playlist::Model::headerData( int section, Qt::Orientation orientation, int role ) const
{
    Q_UNUSED( orientation );

    if ( role != Qt::DisplayRole )
        return QVariant();

    switch ( section )
    {
    case 0:
        return "title";
    case 1:
        return "album";
    case 2:
        return "artist";
    case 3:
        return "custom";
    default:
        return QVariant();
    }
}

QVariant
Playlist::Model::data( const QModelIndex& index, int role ) const
{
    if ( !index.isValid() )
        return QVariant();

    int row = index.row();

    if ( role == ItemRole && ( row != -1 ) )
        return QVariant::fromValue( m_items.at( row ) );

    else if ( role == ActiveTrackRole )
        return ( row == m_activeRow );

    else if ( role == TrackRole && ( row != -1 ) && m_items.at( row )->track() )
        return QVariant::fromValue( m_items.at( row )->track() );

    else if ( role == StateRole && ( row != -1 ) )
        return m_items.at( row )->state();

    else if ( role == Qt::DisplayRole && row != -1 )
    {
        switch ( index.column() )
        {
        case 0:
        {
            return m_items.at( row )->track()->name();
        }
        case 1:
        {
            if ( m_items.at( row )->track()->album() )
                return m_items.at( row )->track()->album()->name();
            else
                return QString();
        }
        case 2:
        {
            if ( m_items.at( row )->track()->artist() )
                return m_items.at( row )->track()->artist()->name();
            else
                return QString();
        }
        case 3:
        {
            QString artist;
            QString album;
            QString track;

            if ( m_items.at( row )->track() )
            {

                track = m_items.at( row )->track()->name();
                if ( m_items.at( row )->track()->artist() )
                    artist = m_items.at( row )->track()->artist()->name();
                if ( m_items.at( row )->track()->artist() )
                    album = m_items.at( row )->track()->album()->name();
            }

            return QString( "%1 - %2 - %3" )
                   .arg( artist )
                   .arg( album )
                   .arg( track );
        }
        }
    }
    // else
    return QVariant();
}

Qt::DropActions
Playlist::Model::supportedDropActions() const
{
    return Qt::MoveAction | QAbstractListModel::supportedDropActions();
}

Qt::ItemFlags
Playlist::Model::flags( const QModelIndex &index ) const
{
    if ( index.isValid() )
        return ( Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDropEnabled | Qt::ItemIsDragEnabled );
    else
        return Qt::ItemIsDropEnabled;
}

QStringList
Playlist::Model::mimeTypes() const
{
    QStringList ret = QAbstractListModel::mimeTypes();
    ret << AmarokMimeData::TRACK_MIME;
    ret << "text/uri-list"; //we do accept urls
    return ret;
}

QMimeData*
Playlist::Model::mimeData( const QModelIndexList &indexes ) const
{
    AmarokMimeData* mime = new AmarokMimeData();
    Meta::TrackList selectedTracks;

    foreach( const QModelIndex &it, indexes )
    selectedTracks << m_items.at( it.row() )->track();

    mime->setTracks( selectedTracks );
    return mime;
}

bool
Playlist::Model::dropMimeData( const QMimeData* data, Qt::DropAction action, int row, int, const QModelIndex& parent )
{
    DEBUG_BLOCK

    if ( action == Qt::IgnoreAction )
    {
        debug() << "ignoring drop";
        return true;
    }

    int beginRow;
    if ( row != -1 )
        beginRow = row;
    else if ( parent.isValid() )
        beginRow = parent.row();
    else
        beginRow = m_items.size();

    if ( data->hasFormat( AmarokMimeData::TRACK_MIME ) )
    {
        const AmarokMimeData* trackListDrag = dynamic_cast<const AmarokMimeData*>( data );
        if ( trackListDrag )
        {
            Controller::instance()->insertTracks( beginRow, trackListDrag->tracks() );
        }
        return true;
    }
    else if ( data->hasFormat( AmarokMimeData::PLAYLIST_MIME ) )
    {
        const AmarokMimeData* dragList = dynamic_cast<const AmarokMimeData*>( data );
        if ( dragList )
        {
            Controller::instance()->insertPlaylists( beginRow, dragList->playlists() );
        }
        return true;
    }
    else if ( data->hasUrls() )
    {
        DirectoryLoader* dl = new DirectoryLoader(); //this deletes itself
        dl->insertAtRow( beginRow );
        dl->init( data->urls() );
        return true;
    }
    debug() << "ignoring unknown drop data";
    return false;
}

void
Playlist::Model::setActiveRow( int row )
{
    if ( rowExists( row ) )
    {
        m_items.at( row )->setState( Item::Played );
        int oldactiverow = m_activeRow;
        m_activeRow = row;

        if ( rowExists( oldactiverow ) )
            emit dataChanged( createIndex( oldactiverow, 0 ), createIndex( oldactiverow, columnCount() - 1 ) );
        emit dataChanged( createIndex( m_activeRow, 0 ), createIndex( m_activeRow, columnCount() - 1 ) );
        emit activeTrackChanged( m_items.at( row )->id() );
    }
    else
    {
        m_activeRow = -1;
        emit activeTrackChanged( 0 );
    }
    emit activeRowChanged( m_activeRow );
}

Playlist::Item::State
Playlist::Model::stateOfRow( int row ) const
{
    if ( rowExists( row ) )
    {
        return m_items.at( row )->state();
    }
    else
    {
        return Item::Invalid;
    }
}

bool
Playlist::Model::containsTrack( const Meta::TrackPtr track ) const
{
    foreach( Item* i, m_items )
    {
        if ( i->track() == track )
        {
            return true;
        }
    }
    return false;
}

int
Playlist::Model::rowForTrack( const Meta::TrackPtr track ) const
{
    int row = 0;
    foreach( Item* i, m_items )
    {
        if ( i->track() == track )
        {
            return row;
        }
        row++;
    }
    return -1;
}

Meta::TrackPtr
Playlist::Model::trackAt( int row ) const
{
    if ( rowExists( row ) )
        return m_items.at( row )->track();
    else
        return Meta::TrackPtr();
}

Meta::TrackPtr
Playlist::Model::activeTrack() const
{
    if ( rowExists( m_activeRow ) )
        return m_items.at( m_activeRow )->track();
    else
        return Meta::TrackPtr();
}

int
Playlist::Model::rowForId( const quint64 id ) const
{
    if ( containsId( id ) )
        return m_items.indexOf( m_itemIds.value( id ) );
    else
        return -1;
}

Meta::TrackPtr
Playlist::Model::trackForId( const quint64 id ) const
{
    if ( containsId( id ) )
        return m_itemIds.value( id )->track();
    else
        return Meta::TrackPtr();
}

quint64
Playlist::Model::idAt( const int row ) const
{
    if ( rowExists( row ) )
        return m_items.at( row )->id();
    else
        return 0;
}

quint64
Playlist::Model::activeId() const
{
    if ( rowExists( m_activeRow ) )
        return m_items.at( m_activeRow )->id();
    else
        return 0;
}

Playlist::Item::State
Playlist::Model::stateOfId( quint64 id ) const
{
    if ( containsId( id ) )
        return m_itemIds.value( id )->state();
    else
        return Item::Invalid;
}

void
Playlist::Model::metadataChanged( Meta::Track *track )
{
    const int size = m_items.size();
    const Meta::TrackPtr needle =  Meta::TrackPtr( track );
    for ( int i = 0; i < size; i++ )
    {
        if ( m_items.at( i )->track() == needle )
        {
            emit dataChanged( createIndex( i, 0 ), createIndex( i, columnCount() - 1 ) );
            break;
        }
    }
}

void
Playlist::Model::metadataChanged( Meta::Album * album )
{
    Meta::TrackList tracks = album->tracks();
    foreach( Meta::TrackPtr track, tracks )
    {
        metadataChanged( track.data() );
    }
}

bool
Playlist::Model::exportPlaylist( const QString &path ) const
{
    Meta::TrackList tl;
    foreach( Item* item, m_items )
    tl << item->track();

    return The::playlistManager()->exportPlaylist( tl, path );
}

bool
Playlist::Model::savePlaylist( const QString & name ) const
{
    DEBUG_BLOCK

    Meta::TrackList tl;
    foreach( Item* item, m_items )
    tl << item->track();

    return The::playlistManager()->save( tl, name, true );
}

QString
Playlist::Model::prettyColumnName( Column index ) //static
{
    switch ( index )
    {
    case Filename:   return i18nc( "The name of the file this track is stored in", "Filename" );
    case Title:      return i18n( "Title" );
    case Artist:     return i18n( "Artist" );
    case AlbumArtist: return i18n( "Album Artist" );
    case Composer:   return i18n( "Composer" );
    case Year:       return i18n( "Year" );
    case Album:      return i18n( "Album" );
    case DiscNumber: return i18n( "Disc Number" );
    case TrackNumber: return i18nc( "The Track number for this item", "Track" );
    case Bpm:        return i18n( "BPM" );
    case Genre:      return i18n( "Genre" );
    case Comment:    return i18n( "Comment" );
    case Directory:  return i18nc( "The location on disc of this track", "Directory" );
    case Type:       return i18n( "Type" );
    case Length:     return i18n( "Length" );
    case Bitrate:    return i18n( "Bitrate" );
    case SampleRate: return i18n( "Sample Rate" );
    case Score:      return i18n( "Score" );
    case Rating:     return i18n( "Rating" );
    case PlayCount:  return i18n( "Play Count" );
    case LastPlayed: return i18nc( "Column name", "Last Played" );
    case Mood:       return i18n( "Mood" );
    case Filesize:   return i18n( "File Size" );
    default:         return "This is a bug.";
    }

}

////////////
//Private Methods
///////////

void
Playlist::Model::insertTracksCommand( const InsertCmdList& cmds )
{
    DEBUG_BLOCK

    if ( cmds.size() < 1 )
        return;

    for ( int i = 0; i < m_items.size(); i++ )
    {
        if ( m_items.at( i )->state() == Item::NewlyAdded )
        {
            m_items.at( i )->setState( Item::Unplayed );
            emit dataChanged( createIndex( i, 0 ), createIndex( i, columnCount() - 1 ) );
        }
    }

    int min = m_items.size() + cmds.size();
    int max = 0;
    int activeShift = 0;
    QList<quint64> newIds;
    foreach( InsertCmd ic, cmds )
    {
        min = qMin( min, ic.second );
        max = qMax( max, ic.second );
        activeShift += ( ic.second < m_activeRow ) ? 1 : 0;
        debug() << "inserting" << ic.first->prettyName() << "at" << ic.second;
    }

    // actually do the insertion
    beginInsertRows( QModelIndex(), min, min + cmds.size() - 1 );
    foreach( InsertCmd ic, cmds )
    {
        Meta::TrackPtr track = ic.first;
        m_totalLength += track->length();
        subscribeTo( track );
        if ( track->album() )
            subscribeTo( track->album() );

        Item* newitem = new Item( track );
        m_items.insert( ic.second, newitem );
        m_itemIds.insert( newitem->id(), newitem );
        newIds.append( newitem->id() );
    }
    endInsertRows();
    emit dataChanged( createIndex( min, 0 ), createIndex( max, columnCount() - 1 ) );
    emit insertedIds( newIds );

    // update the active row
    m_activeRow = ( m_activeRow >= 0 ) ? m_activeRow + activeShift : -1;

    Amarok::actionCollection()->action( "playlist_clear" )->setEnabled( !m_items.isEmpty() );
    //Amarok::actionCollection()->action( "play_pause" )->setEnabled( !activeTrack().isNull() );
}

void
Playlist::Model::removeTracksCommand( const RemoveCmdList& cmds )
{
    DEBUG_BLOCK

    if ( cmds.size() < 1 )
        return;

    int min = m_items.size();
    int max = 0;
    int activeShift = 0;
    bool activeDeleted = false;
    QList<quint64> delIds;
    foreach( RemoveCmd rc, cmds )
    {
        min = qMin( min, rc.second );
        max = qMax( max, rc.second );
        activeShift += ( rc.second < m_activeRow ) ? 1 : 0;
        debug() << "removing" << rc.first->prettyName() << "from" << rc.second;
        if ( rc.second == m_activeRow )
        {
            debug() << "deleting the active track";
            activeDeleted = true;
        }
    }

    beginRemoveRows( QModelIndex(), min, min + cmds.size() - 1 );
    foreach( RemoveCmd rc, cmds )
    {
        Meta::TrackPtr track = rc.first;
        m_totalLength -= track->length();
        unsubscribeFrom( track );
        if ( track->album() )
            unsubscribeFrom( track->album() );

        Item* delitem = m_items.at( rc.second );
        delIds.append( delitem->id() );
        m_itemIds.remove( delitem->id() );
        delete delitem;
        m_items[rc.second] = 0;
    }

    QMutableListIterator<Item*> i( m_items );
    while ( i.hasNext() )
    {
        i.next();
        if ( i.value() == 0 )
            i.remove();
    }
    endRemoveRows();
    if ( m_items.size() > 0 )
    {
        max = ( max < m_items.size() ) ? max : m_items.size() - 1;
        emit dataChanged( createIndex( min, 0 ), createIndex( max, columnCount() ) );
    }
    else
    {

        /* As of version 4.4.2, Qt's ItemViews are really unhappy when all rows
         * are removed from the model, even if you call beginRemoveRows and
         * endRemoveRows properly (like above).  The Views and SelectionModel
         * try to access invalid indexes and the whole program crashes pretty
         * soon thereafter.  Resetting the model after all rows are removed
         * works around this problem.  I'm filing a bug about this with
         * TrollTech, so hopefully the workaround won't be needed in the
         * future. -- stharward */

        debug() << "empty model; calling reset";
        reset();
    }
    emit removedIds( delIds );

    //update the active row
    if ( !activeDeleted && ( m_activeRow >= 0 ) )
    {
        m_activeRow = ( m_activeRow > 0 ) ? m_activeRow - activeShift : 0;
    }
    else
    {
        m_activeRow = -1;
    }

    Amarok::actionCollection()->action( "playlist_clear" )->setEnabled( !m_items.isEmpty() );
    //Amarok::actionCollection()->action( "play_pause" )->setEnabled( !activeTrack().isNull() );
}

void
Playlist::Model::moveTracksCommand( const MoveCmdList& cmds, bool reverse )
{
    DEBUG_BLOCK

    if ( cmds.size() < 1 )
        return;

    int min = m_items.size() + cmds.size();
    int max = 0;
    foreach( MoveCmd rc, cmds )
    {
        min = qMin( min, rc.first );
        min = qMin( min, rc.second );
        max = qMax( max, rc.first );
        max = qMax( max, rc.second );
        debug() << "moving" << rc.first << "to" << rc.second;
        //FIXME
        if ( rc.second < 0 )
        {
            debug() << "FIXME: Target out of range. Aborting.";
            return;
        }
    }

    int newActiveRow = m_activeRow;
    QList<Item*> oldItems( m_items );
    if ( reverse )
    {
        foreach( MoveCmd mc, cmds )
        {
            m_items[mc.first] = oldItems.at( mc.second );
            if ( m_activeRow == mc.second )
                newActiveRow = mc.first;
        }
    }
    else
    {
        foreach( MoveCmd mc, cmds )
        {
            m_items[mc.second] = oldItems.at( mc.first );
            if ( m_activeRow == mc.first )
                newActiveRow = mc.second;
        }
    }
    m_activeRow = newActiveRow;
    emit dataChanged( createIndex( min, 0 ), createIndex( max, columnCount() ) );

    //update the active row
}

namespace The
{
AMAROK_EXPORT Playlist::Model* playlistModel()
{
    return Playlist::Model::instance();
}
}

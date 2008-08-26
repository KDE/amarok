/***************************************************************************
 * copyright        : (C) 2007 Ian Monroe <ian@monroe.nu>
 *                    (C) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
 *                    (C) 2008 Seb Ruiz <ruiz@kde.org>
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


#include "PlaylistModel.h"

#include "ActionClasses.h" //playlistModeChanged()
#include "Amarok.h"
#include "amarokconfig.h"
#include "AmarokMimeData.h"
#include "Debug.h"
#include "DynamicModel.h"
#include "DynamicTrackNavigator.h"
#include "EngineController.h"
#include "PlaylistItem.h"
#include "PlaylistFileSupport.h"
#include "RandomAlbumNavigator.h"
#include "RandomTrackNavigator.h"
#include "RepeatAlbumNavigator.h"
#include "RepeatPlaylistNavigator.h"
#include "RepeatTrackNavigator.h"
#include "StandardTrackNavigator.h"
#include "StatusBar.h"
#include "UndoCommands.h"

#include "collection/BlockingQuery.h"
#include "collection/Collection.h"
#include "collection/CollectionManager.h"
#include "collection/QueryMaker.h"

#include <QAction>
#include <QStringList>
#include <QUndoStack>

#include <KFileItem>
#include <KIcon>
#include <KUrl>

#include <typeinfo>

namespace Amarok
{
    // Sorting of a tracklist.
    bool trackNumberLessThan( Meta::TrackPtr left, Meta::TrackPtr right )
    {
        if( left->album() == right->album() ) // If the albums are the same
        {
            //First compare by disc number
            if ( left->discNumber() < right->discNumber() )
                return true;
            else if( left->discNumber() == right->discNumber() ) //Disc #'s are equal, compare by track number
                return left->trackNumber() < right->trackNumber();
            else
                return false; // Right disc has a lower number
        }
        else if( left->artist() == right->artist() )
            return QString::localeAwareCompare( left->album()->prettyName(), right->album()->prettyName() ) < 0;
        else // compare artists alphabetically
            return QString::localeAwareCompare( left->artist()->prettyName(), right->artist()->prettyName() ) < 0;
    }
}


Playlist::Model *Playlist::Model::s_instance = 0;

Playlist::Model::Model( QObject* parent )
    : QAbstractListModel( parent )
    , EngineObserver( The::engineController() )
    , m_activeRow( -1 )
    , m_nextRowCandidate( -1 )
    , m_advancer( 0 )
    , m_undoStack( new QUndoStack( this ) )
    , m_stopAfterMode( StopNever )
    , m_stopPlaying( false )
    , m_waitingForNextTrack( false )
{
    s_instance = this;
    playlistModeChanged(); // sets m_advancer.
}

void
Playlist::Model::init()
{
    KActionCollection* ac = Amarok::actionCollection();
    QAction* undoButton  = m_undoStack->createUndoAction( this, i18n("Undo") );
    undoButton->setIcon( KIcon( "edit-undo-amarok" ) );
    ac->addAction("playlist_undo", undoButton);
    QAction* redoButton  = m_undoStack->createRedoAction( this, i18n("Redo") );
    ac->addAction("playlist_redo", redoButton);
    redoButton->setIcon( KIcon( "edit-redo-amarok" ) );

    // essentially announces to the TrackAdvancer that there was a change in the playlist
    connect( this, SIGNAL( playlistCountChanged(int) ), SLOT( notifyAdvancersOnItemChange() ) );

}

void
Playlist::Model::restoreSession()
{
    // Restore current playlist
    if( QFile::exists( defaultPlaylistPath() ) )
        insertOptioned( Meta::loadPlaylist( KUrl( defaultPlaylistPath() ) ), Append );

    if( typeid(*m_advancer) == typeid(DynamicTrackNavigator) )
        ((DynamicTrackNavigator*)m_advancer)->appendUpcoming();
}

Playlist::Model::~Model()
{
    DEBUG_BLOCK

    // Save current playlist
    Meta::TrackList list;
    foreach( Item* item, itemList() )
    {
        list << item->track();
    }
    The::playlistManager()->exportPlaylist( list, defaultPlaylistPath() );

    m_advancer->deleteLater();
}


void
Playlist::Model::requestNextTrack()
{
    DEBUG_BLOCK
    if( !m_waitingForNextTrack )
    {
        m_waitingForNextTrack = true;
        m_advancer->requestNextRow();
    }
}

void
Playlist::Model::requestUserNextTrack()
{
    DEBUG_BLOCK
    if( !m_waitingForNextTrack )
    {
        m_waitingForNextTrack = true;
        m_advancer->requestUserNextRow();
    }
}

void
Playlist::Model::requestPrevTrack()
{
    DEBUG_BLOCK
    if( !m_waitingForNextTrack )
    {
        m_waitingForNextTrack = true;
        m_advancer->requestLastRow();
    }
}


void
Playlist::Model::setNextRow( int row )
{
    DEBUG_BLOCK

    if( !m_stopPlaying && rowExists( row ) )
    {
        m_nextRowCandidate = row;
        The::engineController()->setNextTrack( m_items.at(row)->track() );
    }

    m_waitingForNextTrack = false;
}

void
Playlist::Model::setUserNextRow( int row )
{
    DEBUG_BLOCK

    if( !m_stopPlaying && rowExists( row ) )
    {
        m_nextRowCandidate = row;
        play( row );
    }

    m_waitingForNextTrack = false;
}

void
Playlist::Model::setPrevRow( int row )
{
    DEBUG_BLOCK

    if( !m_stopPlaying && rowExists( row ) )
    {
        m_nextRowCandidate = row;
        play( row );
    }

    m_waitingForNextTrack = false;
}


int
Playlist::Model::rowCount( const QModelIndex& ) const
{
    return m_items.size();
}

QVariant
Playlist::Model::data( const QModelIndex& index, int role ) const
{
    int row = index.row();
    if( role == ItemRole && ( row != -1 ) )
        return QVariant::fromValue( m_items.at( row ) );

    else if( role == ActiveTrackRole )
        return ( row == m_activeRow );

    else if( role == TrackRole && ( row != -1 ) && m_items.at( row )->track() )
        return QVariant::fromValue( m_items.at( row )->track() );

    else if( role == StateRole && ( row != -1 ) )
        return m_items.at( row )->state();

    else if ( role == GroupRole )
    {
        Meta::TrackPtr track = m_items.at( row )->track();

        if ( !track->album() )
            return None;  // no album set
        else if ( !m_albumGroups.contains( track->album()->prettyName() ) )
            return None;  // no group for this album, should never happen...

        AlbumGroup * albumGroup = m_albumGroups.value( track->album()->prettyName() );
        return albumGroup->groupMode( row );

    }

    else if ( role == GroupedTracksRole )
    {
        Meta::TrackPtr track = m_items.at( row )->track();
        AlbumGroup * albumGroup = m_albumGroups.value( track->album()->prettyName() );
        return albumGroup->elementsInGroup( row );
    }

    else if ( role == GroupedAlternateRole )
    {
        /*Meta::TrackPtr track = m_items.at( row )->track();
        AlbumGroup * albumGroup = m_albumGroups.value( track->album()->prettyName() );
        if( albumGroup )
            return albumGroup->alternate( row );
        return true;*/

        return ( row % 2 == 1 );
    }

    else if ( role == GroupedCollapsibleRole )
    {
        Meta::TrackPtr track = m_items.at( row )->track();
        AlbumGroup * albumGroup = m_albumGroups.value( track->album()->prettyName() );
        //we cannot collapse the group that contains the currently selected track.
        return ( albumGroup->firstInGroup( m_activeRow ) == -1 );

    }

    else if( role == Qt::DisplayRole && row != -1 )
    {
        switch( index.column() )
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

                if ( m_items.at( row )->track() ) {

                    track = m_items.at( row )->track()->name();
                    if ( m_items.at( row )->track()->artist() )
                        artist = m_items.at( row )->track()->artist()->name();
                    if ( m_items.at( row )->track()->artist() )
                        album = m_items.at( row )->track()->album()->name();
                }
                
                return QString("%1 - %2 - %3")
                        .arg( artist )
                        .arg( album )
                        .arg( track );
            }
        }
    }
    // else
    return QVariant();
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


bool
Playlist::Model::setData( const QModelIndex& index, const QVariant& value, int role )
{
    if( role == StateRole )
    {
        m_items[ index.row() ]->setState( (Item::State)value.toInt() );

        emit dataChanged( index, index );

        return true;
    }
    else return false;
}

void
Playlist::Model::insertTrack( int row, Meta::TrackPtr track )
{
//     DEBUG_BLOCK
    Meta::TrackList list;
    list.append( track );
    insertTracks( row, list );
}

void
Playlist::Model::insertTracks( int row, Meta::TrackList tracks )
{
    DEBUG_BLOCK

    clearNewlyAdded();

    //check if any tracks in this list ha a url that is actuall a playlist
    bool containsPlaylists = false;
    QList<int> playlistIndices;
    int index = 0;
    foreach( Meta::TrackPtr track, tracks )
    {
        if( The::playlistManager()->canExpand( track ) )
        {
            containsPlaylists = true;
            playlistIndices.append( index );
            index++;
        }
    }

    if ( !containsPlaylists )
    {
         //if its all plain tracks, do the easy thing
        m_undoStack->push( new AddTracksCmd( 0, row, tracks ) );
    }
    else
    {
        //split it up, but add as much as possible as groups to keep the undo stuff usable

        int lastIndex = 0;
        int offset = 0;

        foreach( int playlistIndex, playlistIndices )
        {
            if( ( playlistIndex  - lastIndex ) > 0 )
            {
                m_undoStack->push( new AddTracksCmd( 0, row + lastIndex + offset, tracks.mid( lastIndex, playlistIndex  - lastIndex ) ) );
                row = row + ( playlistIndex  - lastIndex );
            }

            Meta::PlaylistPtr playlist =  The::playlistManager()->expand( tracks.at( playlistIndex ) );
            if( playlist ) {
                m_undoStack->push( new AddTracksCmd( 0, row + playlistIndex + offset, playlist->tracks() ) );
                offset += playlist->tracks().size();

                lastIndex = playlistIndex + 1;
            }
        }
    }
}

bool
Playlist::Model::removeRows( int position, int rows, const QModelIndex& /*parent*/  )
{
    m_undoStack->push( new RemoveTracksCmd( 0, position, rows ) );
    return true;
}

bool
Playlist::Model::moveRow( int from, int to )
{
    m_undoStack->push( new MoveTrackCmd( 0, from, to ) );
    return true;
}

bool
Playlist::Model::moveMultipleRows(QList< int > rows, int to)
{
    m_undoStack->push( new MoveMultipleTracksCmd( 0, rows, to ) );
    return true;
}

void
Playlist::Model::insertTracks( int row, QueryMaker *qm )
{
    qm->setQueryType( QueryMaker::Track );
    connect( qm, SIGNAL( queryDone() ), SLOT( queryDone() ) );
    connect( qm, SIGNAL( newResultReady( QString, Meta::TrackList ) ), SLOT( newResultReady( QString, Meta::TrackList ) ) );
    m_queryMap.insert( qm, row );
    qm->run();
}

Qt::DropActions
Playlist::Model::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}

// void
// Model::trackFinished()
// {
//     if( m_activeRow < 0 || m_activeRow >= m_items.size() )
//         return;
//     Meta::TrackPtr track = m_items.at( m_activeRow )->track();
//     track->finishedPlaying( 1.0 ); //TODO: get correct value for parameter
//     m_advancer->advanceTrack();
// }

void
Playlist::Model::play( const QModelIndex& index )
{
    play( index.row() );
}

void
Playlist::Model::play( int row )
{
    m_stopPlaying = false;

    if( m_items.size() > row )
    {
        m_nextRowCandidate = row;
        The::engineController()->play( m_items[ row ]->track() );
    }
}

void
Playlist::Model::next()
{
    requestUserNextTrack();
}

void
Playlist::Model::back()
{
    requestPrevTrack();
}

// void
// Model::playCurrentTrack()
// {
//     DEBUG_BLOCK
//     int selected = m_activeRow;
//     if( selected < 0 || selected >= m_items.size() )
//     {
//         //play first track if there are tracks in the playlist
//         if( m_items.size() )
//             selected = 0;
//         else
//             return;
//     }
//     debug() << "SELECTED: " << selected;
//     play( selected );
// }


QString
Playlist::Model::prettyColumnName( Column index ) //static
{
    switch( index )
    {
        case Filename:   return i18nc( "The name of the file this track is stored in", "Filename"    );
        case Title:      return i18n( "Title"       );
        case Artist:     return i18n( "Artist"      );
        case AlbumArtist:return i18n( "Album Artist");
        case Composer:   return i18n( "Composer"    );
        case Year:       return i18n( "Year"        );
        case Album:      return i18n( "Album"       );
        case DiscNumber: return i18n( "Disc Number" );
        case TrackNumber:return i18nc( "The Track number for this item", "Track"       );
        case Bpm:        return i18n( "BPM"         );
        case Genre:      return i18n( "Genre"       );
        case Comment:    return i18n( "Comment"     );
        case Directory:  return i18nc( "The location on disc of this track", "Directory"   );
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
Playlist::Model::playlistModeChanged()
{
    DEBUG_BLOCK

    m_advancer->deleteLater();

    int options = Playlist::StandardPlayback;

    debug() << "Repeat enabled: " << Amarok::repeatEnabled();
    debug() << "Random enabled: " << Amarok::randomEnabled();
    debug() << "Track mode:     " << ( Amarok::repeatTrack() || Amarok::randomTracks() );
    debug() << "Album mode:     " << ( Amarok::repeatAlbum() || Amarok::randomAlbums() );
    debug() << "Playlist mode:  " << Amarok::repeatPlaylist();
    debug() << "Dynamic mode:   " << AmarokConfig::dynamicMode();

    if( AmarokConfig::dynamicMode() )
    {
        PlaylistBrowserNS::DynamicModel* dm =
            PlaylistBrowserNS::DynamicModel::instance();

        Dynamic::DynamicPlaylistPtr playlist = dm->activePlaylist();

        if( !playlist )
            playlist = dm->defaultPlaylist();

        const bool wasNull = m_advancer == 0; 

        m_advancer = new DynamicTrackNavigator( this, playlist );


        // wasNull == true indicates that Amarok is just starting up.
        // Because of some quirk in the construction order it
        // will crash if we if try adding tracks now.
        if( !wasNull )
        {
            ((DynamicTrackNavigator*) m_advancer)->appendUpcoming();
            if( activeRow() < 0 && rowCount() > 0 )
                play( 0 );
        }

        return;
    }

    m_advancer = 0;

    if( Amarok::repeatEnabled() )
        options |= Playlist::RepeatPlayback;
    if( Amarok::randomEnabled() )
        options |= Playlist::RandomPlayback;
    if( Amarok::repeatTrack() || Amarok::randomTracks() )
        options |= Playlist::TrackPlayback;
    if( Amarok::repeatAlbum() || Amarok::randomAlbums() )
        options |= Playlist::AlbumPlayback;
    if( Amarok::repeatPlaylist() )
        options |= Playlist::PlaylistPlayback;

    if( options == Playlist::StandardPlayback )
    {
        m_advancer = new StandardTrackNavigator( this );
    }
    else if( options & Playlist::RepeatPlayback )
    {
        if( options & Playlist::TrackPlayback )
            m_advancer = new RepeatTrackNavigator( this );
        else if( options & Playlist::PlaylistPlayback )
            m_advancer = new RepeatPlaylistNavigator( this );
        else if( options & Playlist::AlbumPlayback )
            m_advancer = new RepeatAlbumNavigator( this );
    }
    else if( options & Playlist::RandomPlayback )
    {
        if( options & Playlist::TrackPlayback )
            m_advancer = new RandomTrackNavigator( this );
        else if( options & Playlist::AlbumPlayback )
            m_advancer = new RandomAlbumNavigator( this );
    }

    if( m_advancer == 0 )
    {
        debug() << "Play mode not implemented, defaulting to Standard Playback";
        m_advancer = new StandardTrackNavigator( this );
    }
}


void
Playlist::Model::setActiveRow( int row )
{
    DEBUG_BLOCK

    emit dataChanged( createIndex( m_activeRow, 0 ), createIndex( m_activeRow, 0 ) );
    emit dataChanged( createIndex( row, 0 ), createIndex( row, 0 ) );


    m_activeRow = row;

    //make sure that the group containing this track is expanded

    //not all tracks have a valid album ( radio stations for instance... )
    QString albumName;
    if ( !m_items[ row ]->track()->album().isNull() ) {
        albumName = m_items[ row ]->track()->album()->prettyName();
    }

    if( !albumName.isEmpty() && m_albumGroups.contains( albumName ) && m_albumGroups[ albumName ] )
    {
        m_albumGroups[ albumName ]->setCollapsed( row,  false );
        emit( playlistGroupingChanged() );
    }
}

Meta::TrackPtr
Playlist::Model::activeTrack() const
{
    if( m_activeRow > -1 )
        return m_items.at( m_activeRow )->track();
    else if( rowCount() > 0 )
        return m_items.at( 0 )->track();

    return Meta::TrackPtr();
}

void
Playlist::Model::metadataChanged( Meta::Track *track )
{
    const int size = m_items.size();
    const Meta::TrackPtr needle =  Meta::TrackPtr( track );
    for( int i = 0; i < size; i++ )
    {
        if( m_items.at( i )->track() == needle )
        {
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
Playlist::Model::metadataChanged(Meta::Album * album)
{
    //process each track
    Meta::TrackList tracks = album->tracks();
    foreach( Meta::TrackPtr track, tracks ) {
        metadataChanged( track.data() );
    }

}

void
Playlist::Model::trackListChanged( Meta::Playlist * playlist )
{
    //So what if it changes, we don't care. We shouldn't even receive these events!
    if( m_observedPlaylist != playlist)
        return;
}

void
Playlist::Model::clear()
{
    if( m_items.size() < 1 )
        return;
    removeRows( 0, m_items.size() );
    m_albumGroups.clear();
    m_lastAddedTrackAlbum = Meta::AlbumPtr();
//    m_activeRow = -1;
}

void
Playlist::Model::insertTrackListSlot( Meta::TrackList list ) //slot
{
    disconnect( this, SLOT( insertTrackListSlot( Meta::TrackList ) ) );
    int row = m_dragHash[sender()];

    qStableSort( list.begin(), list.end(), Amarok::trackNumberLessThan );

    if( row < 0 )
        insertOptioned( list, Playlist::AppendAndPlay );
    else
        insertTracks( row, list );
}

void
Playlist::Model::insertOptioned( Meta::TrackList list, int options )
{
    //DEBUG_BLOCK
    //TODO: we call insertOptioned on resume before the statusbar is fully created... We need a better way to handle this
    if( list.isEmpty() )
    {
        // The::statusBar()->shortMessage( i18n("Attempted to insert nothing into playlist.") );
        return; // don't add empty items
    }

    if( options & Unique )
    {
        int alreadyOnPlaylist = 0;
        QMutableListIterator<Meta::TrackPtr> i( list );
        while( i.hasNext() )
        {
            i.next();
            Item* item;
            foreach( item, m_items )
            {
                if( item->track() == i.value() )
                {
                    i.remove();
                    alreadyOnPlaylist++;
                    break;
                }
            }
        }

        if ( alreadyOnPlaylist )
            The::statusBar()->shortMessage( i18np("One track was already in the playlist, so it was not added.", "%1 tracks were already in the playlist, so they were not added.", alreadyOnPlaylist ) );
    }

    int firstItemAdded = -1;
    if( options & Replace )
    {
        clear();
        firstItemAdded = 0;
        insertTracks( 0, list );

        for( int i = 0; i < list.size() && m_items.size() > i; ++i )
        {
            m_newlyAdded.append( i );
            m_items[i]->setState( Item::NewlyAdded );
        }
    }
    else if( options & Append )
    {
        firstItemAdded = rowCount();
        insertTracks( firstItemAdded, list );

        for( int i = 0; i < list.size() && ( m_items.size() > firstItemAdded + i ); ++i )
        {
            m_newlyAdded.append( firstItemAdded + i );
            m_items[firstItemAdded + i]->setState( Item::NewlyAdded );
        }

        // This makes unpolite things happen on startup.. the enginecontroller and us are both trying to play.
        // Is this really necessary or should places that want this behavior use a different set of options.
/*        if( The::engineController()->state() != Engine::Playing )
            play( firstItemAdded );*/
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
    else if( ( options & StartPlay )
               && ( The::engineController()->state() != Phonon::PlayingState )
               && ( rowCount() != 0 ) )
    {
        play( firstItemAdded );
    }
    Amarok::actionCollection()->action( "playlist_clear" )->setEnabled( !m_items.isEmpty() );
}

void
Playlist::Model::insertOptioned( Meta::TrackPtr track, int options )
{
    if( !track )
    {
        return;
    }
    Meta::TrackList list;
    list.append( track );
    insertOptioned( list, options );
}

void
Playlist::Model::insertOptioned( Meta::PlaylistList list, int options )
{
    foreach( Meta::PlaylistPtr playlist, list )
    {
        insertOptioned( playlist, options );
    }
}

void
Playlist::Model::insertOptioned( Meta::PlaylistPtr playlist, int options )
{
    if( !playlist )
        return;

    //TODO: Add this Meta::Playlist to the observed list
    insertOptioned( playlist->tracks(), options );
}

void
Playlist::Model::insertPlaylist( int row, Meta::PlaylistPtr playlist )
{
    m_undoStack->push( new AddTracksCmd( 0, row, playlist->tracks() ) );
}

void
Playlist::Model::insertPlaylists( int row, Meta::PlaylistList playlists )
{
    int lastRow = row;
    foreach( Meta::PlaylistPtr playlist, playlists )
    {
        insertPlaylist( lastRow, playlist );
        lastRow += playlist->tracks().size();
    }
}

void
Playlist::Model::insertOptioned( QueryMaker *qm, int options )
{
    if( !qm )
    {
        return;
    }
    qm->setQueryType( QueryMaker::Track );
    connect( qm, SIGNAL( queryDone() ), SLOT( queryDone() ) );
    connect( qm, SIGNAL( newResultReady( QString, Meta::TrackList ) ), SLOT( newResultReady( QString, Meta::TrackList ) ) );
    m_optionedQueryMap.insert( qm, options );
    qm->run();
}

bool
Playlist::Model::exportPlaylist( const QString &path ) const
{
    Meta::TrackList tl;
    foreach( Item* item, itemList() )
        tl << item->track();

    return The::playlistManager()->exportPlaylist( tl, path );
}

bool Playlist::Model::savePlaylist( const QString & name ) const
{
    DEBUG_BLOCK
            
    Meta::TrackList tl;
    foreach( Item* item, itemList() )
        tl << item->track();
    
    return The::playlistManager()->save( tl, name );
}


void
Playlist::Model::engineStateChanged( Phonon::State currentState, Phonon::State oldState )
{
    DEBUG_BLOCK
    Q_UNUSED( oldState )

    static int failures = 0;
    const int maxFailures = 4;

    if( currentState == Phonon::ErrorState )
    {
        failures++;
        debug() << "Error, can not play this track.";
        debug() << "Failure count: " << failures;
        if( failures >= maxFailures )
        {
            The::statusBar()->longMessageThreadSafe( 
                i18n( "Too many errors encountered in playlist. Playback stopped." ),
                KDE::StatusBar::Warning );
            debug() << "Stopping playlist.";
            failures = 0;
            m_stopPlaying = true;
        }
    }
    else if( currentState == Phonon::PlayingState )
    {
        failures = 0;
        m_stopPlaying = false;
        debug() << "Successfully played track. Resetting failure count.";
    }
}


void
Playlist::Model::engineNewTrackPlaying()
{
    DEBUG_BLOCK

    int oldActiveRow = activeRow();

    if( rowExists(oldActiveRow) &&
            m_items[oldActiveRow]->state() == Item::NewlyAdded )
    {
        m_newlyAdded.removeAll( oldActiveRow );
        m_items[oldActiveRow]->setState( Item::Normal );
    }

    Meta::TrackPtr track = The::engineController()->currentTrack();

    if( track )
    {
        if( rowExists( m_nextRowCandidate  ) &&
                track == m_items.at( m_nextRowCandidate )->track() )
        {
            setActiveRow( m_nextRowCandidate );

            emit activeRowChanged( oldActiveRow, activeRow() );
        }
        else
        {
            warning() << "engineNewTrackPlaying:: MISS";

            foreach( Item* item, itemList() )
            {
                if( item->track() == track )
                {
                    warning() << "candidate = " << m_nextRowCandidate << ", actual = " << m_items.lastIndexOf( item );
                    setActiveItem( item );
                    break;
                }
            }

            emit activeRowChanged( oldActiveRow, activeRow() );
        }
    }

    m_nextRowCandidate = -1;
}



Qt::ItemFlags
Playlist::Model::flags(const QModelIndex &index) const
{
    if( index.isValid() )
    {
        return ( Qt::ItemIsEnabled     | Qt::ItemIsSelectable | Qt::ItemIsDropEnabled |
                 Qt::ItemIsDragEnabled | Qt::ItemIsSelectable );
    }
    return Qt::ItemIsDropEnabled;
}

QStringList
Playlist::Model::mimeTypes() const //reimplemented
{
    QStringList ret = QAbstractListModel::mimeTypes();
    ret << AmarokMimeData::TRACK_MIME;
    ret << "text/uri-list"; //we do accept urls
    return ret;
}

QMimeData*
Playlist::Model::mimeData( const QModelIndexList &indexes ) const //reimplemented
{
    AmarokMimeData* mime = new AmarokMimeData();
    Meta::TrackList selectedTracks;

    foreach( const QModelIndex &it, indexes )
        selectedTracks << m_items.at( it.row() )->track();

    mime->setTracks( selectedTracks );
    return mime;
}

bool
Playlist::Model::dropMimeData ( const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent ) //reimplemented
{
    Q_UNUSED( column ); Q_UNUSED( parent );
//     DEBUG_BLOCK

    if( action == Qt::IgnoreAction )
        return true;

    if( data->hasFormat( AmarokMimeData::TRACK_MIME ) )
    {
        debug() << "Found track mime type";

        const AmarokMimeData* trackListDrag = dynamic_cast<const AmarokMimeData*>( data );
        if( trackListDrag )
        {
            m_dragHash[const_cast<AmarokMimeData*>(trackListDrag)] = row;
            connect( trackListDrag, SIGNAL( trackListSignal( Meta::TrackList ) ),
                   this, SLOT( insertTrackListSlot( Meta::TrackList ) ) ); 
            trackListDrag->getTrackListSignal();
            return true;
        }
    }
    else if( data->hasFormat( AmarokMimeData::PLAYLIST_MIME ) )
    {
        debug() << "Found playlist mime type";

        const AmarokMimeData* dragList = dynamic_cast<const AmarokMimeData*>( data );
        if( dragList )
        {
            if( row < 0 )
            {
                debug() << "Inserting at row: " << row << " so we are appending to the list.";
                insertOptioned( dragList->playlists(), Playlist::AppendAndPlay );
            }
            else
            {
                debug() << "Inserting at row: " << row <<" so its inserted correctly.";
                insertPlaylists( row, dragList->playlists() );
            }
            return true;
        }
    }
    else if( data->hasUrls() )
    {
        //drop from an external source or the file browser
        debug() << "Drop from external source or file browser";
        QList<QUrl> urls = data->urls();
        UrlListDropInfo* info = new UrlListDropInfo();
        info->row = row;
        QList<KIO::ListJob*> jobs;
        //we use listOperations to figure out when we are done with the directory listing
        //since I (think) we can't be sure what order the directory listing results
        //happen in.
        //listOperations are incremented for every new listRecursive.
        //When listOperations is 0, run finishUrlListDrop, which cleans up and inserts the tracks
        //this all assumes that listRecursive doesn't do weird threaded things and acts before
        //this function is finished.
        foreach( const QUrl &url, urls )
        {
            KUrl kurl = KUrl( url );
            KFileItem kitem( KFileItem::Unknown, KFileItem::Unknown, kurl, true );
            if( kitem.isDir() )
            {
                info->listOperations++;
                KIO::ListJob* lister = KIO::listRecursive( kurl ); //kjob's delete themselves
                connect( lister, SIGNAL( entries( KIO::Job*, const KIO::UDSEntryList&) )
                        , SLOT( directoryListResults( KIO::Job*, const KIO::UDSEntryList& ) ) );
                lister->setProperty("info", QVariant::fromValue( info ) );
            }
            else if( EngineController::canDecode( kurl ) )
            {
                Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( kurl );
                if( track )
                    info->tracks << track;
            }
            //else TODO: notify user if can't decode, see also MyDirLister::matchesFilter
        }
        if( info->listOperations == 0 )
        {
            finishUrlListDrop( info );
            info = 0;
        }
        return true;
    }
    return false;
}

Meta::TrackPtr
Playlist::Model::trackForRow( int row ) const
{
    if( row >= 0 && row < itemList().size() )
    {
        return itemList().at( row )->track();
    }
    else
        return Meta::TrackPtr( 0 );
}


////////////
//Private Methods
///////////

void
Playlist::Model::directoryListResults( KIO::Job *job, const KIO::UDSEntryList &list )
{
//     DEBUG_BLOCK
    //dfaure says that job->redirectionUrl().isValid() ? job->redirectionUrl() : job->url(); might be needed
    //but to wait until an issue is actually found, since it might take more work
    const KUrl dir = static_cast<KIO::SimpleJob*>( job )->url();
    UrlListDropInfo* info = job->property("info" ).value<UrlListDropInfo*>();
    if( !info )
    {
        warning() << "invalid UrlListDropInfo, playlist drop will not function.";
        return;
    }
    foreach( const KIO::UDSEntry &entry, list )
    {
        KUrl currentUrl = dir;
        currentUrl.addPath( entry.stringValue( KIO::UDSEntry::UDS_NAME ) );
        debug() << "listing url: " << currentUrl;
        if( !entry.isDir() &&  EngineController::canDecode( currentUrl ) )
        {
            Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( currentUrl );
            if( track )
                info->tracks.append( track );
        }
    }
    info->listOperations--;
    if( info->listOperations < 1 )
        finishUrlListDrop( info );
}

void
Playlist::Model::finishUrlListDrop( UrlListDropInfo* info )
{
    if( !( info->tracks.isEmpty() ) )
    {
        qStableSort( info->tracks.begin(), info->tracks.end(), Amarok::trackNumberLessThan);
        insertTracks( info->row, info->tracks );
    }
    delete info;
}


void
Playlist::Model::insertTracksCommand( int row, Meta::TrackList list )
{
    DEBUG_BLOCK

    debug() << "row: " << row << ", list count: " << list.size();

    if ( row == -1 )
        row = m_items.count();
    
    if( !list.size() )
        return;

    clearNewlyAdded();

    int adjCount = list.size() - 1;

    if ( adjCount < 0 )
        adjCount = 0;

    beginInsertRows( QModelIndex(), row, row + adjCount );

    
    int i = 0;
    foreach( Meta::TrackPtr track , list )
    {
        if( track )
        {
            subscribeTo( track );
            if( track->album() )
                subscribeTo( track->album() );

            m_items.insert( row + i, new Item( track ) );
            i++;
        }
    }

    //we need to regroup everything below this point as all the index changes
    regroupAlbums( row, m_items.count() );

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
    dataChanged( createIndex( row, 0 ), createIndex( adjCount, 0 ) );

    Amarok::actionCollection()->action( "playlist_clear" )->setEnabled( !m_items.isEmpty() );
    Amarok::actionCollection()->action( "play_pause" )->setEnabled( !activeTrack().isNull() );

    emit playlistCountChanged( rowCount() );
}

Meta::TrackList
Playlist::Model::removeTracksCommand( int position, int rows )
{
    DEBUG_BLOCK

    clearNewlyAdded();

    beginRemoveRows( QModelIndex(), position, position + rows - 1 );
//     TrackList::iterator start = m_tracks.begin() + position;
//     TrackList::iterator end = start + rows;
//     m_tracks.erase( start, end );
    Meta::TrackList ret;
    for( int i = position; i < position + rows; i++ )
    {
        Item* item = m_items.takeAt( position ); //take at position, row times
        unsubscribeFrom( item->track() );
        if( item->track()->album() )
            unsubscribeFrom( item->track()->album() );
        ret.push_back( item->track() );
        delete item;
    }

    //update m_activeRow
    bool activeRowChanged = true;
    const bool oldActiveRow = m_activeRow;

    debug() << "activeRow: " << m_activeRow;
    debug() << "position: " << position;
    debug() << "rows: " << rows;
    
    
    if( m_activeRow >= position && m_activeRow < ( position + rows ) )
        m_activeRow = -1;
    else if( m_activeRow >= position )
        m_activeRow = m_activeRow - rows;
    else
        activeRowChanged = false;

    if( activeRowChanged )
    {
        debug() << "Active row has changed from " << oldActiveRow << " to " << m_activeRow;
        
        dataChanged( createIndex( oldActiveRow, 0 ), createIndex( oldActiveRow, columnCount() -1 ) );
        dataChanged( createIndex( m_activeRow, 0 ), createIndex( m_activeRow, columnCount() -1 ) );
    }
    //dataChanged( createIndex( position, 0 ), createIndex( rowCount(), 0 ) );


    //we need to regroup everything below this point as all the index changes
    //also, use the count before the rows was removed to make sure all groups are deleted
    regroupAlbums( position, rows, OffsetAfter, 0 - rows );

    endRemoveRows();

    Amarok::actionCollection()->action( "playlist_clear" )->setEnabled( !m_items.isEmpty() );
    Amarok::actionCollection()->action( "play_pause" )->setEnabled( !activeTrack().isNull() );

    emit playlistCountChanged( rowCount() );

    emit rowsChanged( position );

    return ret;
}

void
Playlist::Model::queryDone() //Slot
{
    QueryMaker *qm = dynamic_cast<QueryMaker*>( sender() );
    if( qm )
    {
        m_queryMap.remove( qm );
        qm->deleteLater();
    }
}

void
Playlist::Model::newResultReady( const QString &collectionId, const Meta::TrackList &tracks ) //Slot
{
    Meta::TrackList ourTracks = tracks;
    qStableSort( ourTracks.begin(), ourTracks.end(), Amarok::trackNumberLessThan );
    Q_UNUSED( collectionId )
    QueryMaker *qm = dynamic_cast<QueryMaker*>( sender() );
    if( qm )
    {
        //requires better handling of queries which return multiple results
        if( m_queryMap.contains( qm ) )
            insertTracks( m_queryMap.value( qm ), ourTracks );
        else if( m_optionedQueryMap.contains( qm ) )
            insertOptioned( ourTracks, m_optionedQueryMap.value( qm ) );
    }
}

QVariant
Playlist::Model::headerData(int section, Qt::Orientation orientation, int role) const
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

void
Playlist::Model::moveRowCommand( int row, int to )
{
    DEBUG_BLOCK

    debug() << "row " << row << " to " << to;
    
    
    clearNewlyAdded();

    m_items.move( row, to );

    //if we are moving stuff from one side of the current track to the other, we need to update its row:

    if ( row < m_activeRow && to > m_activeRow )
        m_activeRow -= 1;
    else if ( row > m_activeRow && to < m_activeRow )
        m_activeRow += 1;
    else if ( row == m_activeRow )
        m_activeRow = to;
    else if ( to == m_activeRow && row > m_activeRow)
        m_activeRow += 1;
    else if ( to == m_activeRow && row < m_activeRow)
        m_activeRow -= 1;

    
    int offset = -1;
    if ( to < row )
        offset = 1;

    int min = row;
    int max = to;

    if ( row > to ) {
        min = to;
        max = row;
    }

    debug() << "min " << min << " max " << max;
        
    regroupAlbums( min, max, OffsetBetween, offset );

    emit rowMoved( row, to );

}


void
Playlist::Model::regroupAlbums( int firstRow, int lastRow, OffsetMode offsetMode, int offset )
{
    int area1Start = -1;
    int area1End = -1;
    int area2Start = -1;
    int area2End = -1;

    int aboveFirst;
    int belowLast;

    aboveFirst = firstRow - 1;
    if ( aboveFirst < 0 ) aboveFirst = 0;

    belowLast = lastRow + 1;
    if ( belowLast > ( m_items.count() - 1 ) ) belowLast = m_items.count() - 1;
     

    debug() << "firstRow: " << firstRow << ", lastRow: " << lastRow;
    debug() << "aboveFirst: " << aboveFirst << ", belowLast: " << belowLast;

    //delete affected groups

    QMapIterator< QString, AlbumGroup *> itt(m_albumGroups);
    while (itt.hasNext()) {

        itt.next();
        AlbumGroup * group = itt.value();

        bool removeGroupAboveFirstRow = false;
        bool removeGroupBelowFirstRow = false;
        bool removeGroupAboveLastRow = false;
        bool removeGroupBelowLastRow = false;

        int temp = group->firstInGroup( aboveFirst );
        if ( temp != -1 ) {
//             debug() << "--3";
            area1Start = temp;
            removeGroupAboveFirstRow = true;
        }

        temp = group->lastInGroup( firstRow + 1 );
        if ( temp != -1 ) {
//             debug() << "--4";
            area1End = temp;
            removeGroupBelowFirstRow = true;
        }

        temp = group->firstInGroup( lastRow - 1 );
        if ( temp != -1 ) {
//             debug() << "--5";
            area2Start = temp;
            removeGroupAboveLastRow = true;
        }

        temp = group->lastInGroup( belowLast );
        if ( temp != -1 ) {
//             debug() << "--6";
            area2End = temp;
            removeGroupBelowLastRow = true;
        }


        debug() << "area1Start: " << area1Start << ", area1End: " << area1End;
        debug() << "area2Start: " << area2Start << ", area2End: " << area2End;

        

        if ( removeGroupAboveFirstRow )
        { group->removeGroup( aboveFirst ); /*debug() << "removing group at row: " <<  aboveFirst;*/ }

        if ( removeGroupBelowFirstRow )
            { group->removeGroup( firstRow + 1 ); /*debug() << "removing group at row: " <<  firstRow + 1;*/ }

        if ( removeGroupAboveLastRow )
            { group->removeGroup( lastRow -1 ); /*debug() << "removing group at row: " <<  lastRow - 1;*/ }
        if ( removeGroupBelowLastRow )
        { group->removeGroup( belowLast );  /*debug() << "removing group at row: " <<  belowLast;*/ }

        group->removeGroup( firstRow );
        group->removeGroup( lastRow );

        //if there is nothing left in album group, discard it.

        if ( group->subgroupCount() == 0 ) {
            //debug() << "empty...";
            delete m_albumGroups.take( itt.key() );

        }



    }


    if ( m_albumGroups.count() == 0 ) { // start from scratch
        debug() << "--1";
        area1Start = 0;
        area1End = m_items.count();
        area2Start = area1Start; // just to skip second pass

    }


    if ( area1Start == -1 ) {
        debug() << "--2";
        area1Start = aboveFirst;
        area1End = belowLast;
        area2Start = area1Start;
    }

    debug() << "area1Start: " << area1Start << ", area1End: " << area1End;
    debug() << "area2Start: " << area2Start << ", area2End: " << area2End;

    if ( area1End == -1 )
        area1End = belowLast;

    if ( area1Start < 0 )
        area1Start = 0;
    if ( area1End > ( m_items.count() - 1 ) )
        area1End = m_items.count() - 1;

    if ( area2Start < 0 )
        area2Start = 0;
    if ( area2End > ( m_items.count() - 1 ) )
        area2End = m_items.count() - 1;


    // regroup the two affected areas

    if ( area1Start == area2Start ) //merge areas
        area1End = qMax( area1End, area2End );
    else if ( area1End == area2End ) {//merge areas
        area1Start = qMin( area1Start, area2Start );
        area2Start = area1Start;
    }


     debug() << "area1Start: " << area1Start << ", area1End: " << area1End;
     debug() << "area2Start: " << area2Start << ", area2End: " << area2End;

    //debug stuff
     debug() << "Groups before:";
     foreach( AlbumGroup * ag, m_albumGroups)
        ag->printGroupRows();



    if ( offsetMode != OffsetNone ) {

        int offsetFrom;
        int offsetTo;

        if ( offsetMode == OffsetBetween ) {
            offsetFrom = area1End + 1;
            offsetTo = area2Start - 1;
            // last but not least, change area1end and area2start to match new offsets
             if ( area1End != area2End ) {
                area1End += offset;
                area2Start += offset;
//                 debug() << "area1Start: " << area1Start << ", area1End: " << area1End;
//                 debug() << "area2Start: " << area2Start << ", area2End: " << area2End;
             }
        } else {
            offsetFrom = lastRow;
            offsetTo = ( m_items.count() - offset ) + 1;
        }

        QMapIterator< QString, AlbumGroup *> itt(m_albumGroups);
        while (itt.hasNext()) {

           itt.next();
           AlbumGroup * group = itt.value();
           group->offsetBetween( offsetFrom, offsetTo, offset);

        }
    }

    //debug stuff
//     debug() << "Groups after offsetting:";
//    foreach( AlbumGroup * ag, m_albumGroups)
//       ag->printGroupRows();


    int i;
    for (  i = area1Start; ( i <= area1End ) && ( i < m_items.count() ); i++ )
    {

       //debug() << "i: " << i;

       Meta::TrackPtr track = m_items.at( i )->track();

       if ( !track->album() )
           continue;

       QString albumName;

       //not all tracks have a valid album ( radio stations for instance... )
       if ( !track->album().isNull() ) {
           albumName = track->album()->prettyName();
       }

       if ( m_albumGroups.contains( albumName ) && !albumName.isEmpty() ) {
           m_albumGroups[ albumName ]->addRow( i );
       } else {
            //debug() << "Create new group for album " << track->album()->name() ;
            AlbumGroup * newGroup = new AlbumGroup();
            newGroup->addRow( i );
            m_albumGroups.insert( albumName, newGroup );
       }
    }

    if ( ( area1Start == area2Start ) || area2Start == -1 ) {

         debug() << "Groups after v1:";
        foreach( AlbumGroup * ag, m_albumGroups)
            ag->printGroupRows();
        emit( playlistGroupingChanged() );
        return;
    }

    for (  i = area2Start; i <= area2End; i++ )
    {

       //debug() << "i: " << i;

       Meta::TrackPtr track = m_items.at( i )->track();

       if ( !track->album() )
           continue;

       if ( m_albumGroups.contains( track->album()->prettyName() ) ) {
            m_albumGroups[ track->album()->prettyName() ]->addRow( i );
       } else {
            AlbumGroup * newGroup = new AlbumGroup();
            newGroup->addRow( i );
            m_albumGroups.insert( track->album()->prettyName(), newGroup );
       }
    }

     debug() << "Groups after v2:";
     foreach( AlbumGroup *ag, m_albumGroups)
      ag->printGroupRows();

   emit( playlistGroupingChanged() );

   //reset();

}

void
Playlist::Model::setCollapsed(int row, bool collapsed)
{
    //DEBUG_BLOCK
    m_albumGroups[ m_items[ row ]->track()->album()->prettyName() ]->setCollapsed( row,  collapsed );
    emit( playlistGroupingChanged() );
}


void
Playlist::Model::clearNewlyAdded()
{
    int row;
    for( int i = 0; i < m_newlyAdded.size(); ++i )
    {
        row = m_newlyAdded[i];
        if( rowExists(row) && m_items[row]->state() == Item::NewlyAdded )
        {
            m_items[row]->setState( Item::Normal );
        }
    }
    m_newlyAdded.clear();
}


bool Playlist::Model::moveMultipleRowsCommand( QList< int > rows, int to )
{

    DEBUG_BLOCK

    if ( to >= m_items.count() ) to =  m_items.count() - 1;

    debug() << "rows " << rows << " to " << to;
    
    clearNewlyAdded();

    int first = qMin( rows[0], to );

    int i = 0;

    if ( first < to ) {

        // moving down
        
        foreach ( int row, rows ) {
            debug() << "moving " << row - i << " to " << to;
            //updates values to reflect new values after each move
            m_items.move( row - i, to );
            i++;
        }

    } else {

        //moving up

        foreach ( int row, rows ) {
            debug() << "moving " << row << " to " << to + i;
            //updates values to reflect new values after each move
            m_items.move( row, to + i );
            i++;
        }
        
    }

    int orgFirst = rows[0];
    int orgLast = orgFirst + i;
    int count = i;

    //if we are moving stuff from one side of the current track to the other, we need to update its row, as well as if the current track is within the album.

    if ( orgFirst < m_activeRow && orgLast > m_activeRow) {
        //current track is in the selection begin moved...
        debug() << "current track is in the selection begin moved...";
        //if we are moving down, remember to subtract the number of rows being moved as they counted against the origitnal value of "to"
        if ( orgFirst > to ) 
            m_activeRow = to + ( m_activeRow - orgFirst );
        else
            m_activeRow = to + ( ( m_activeRow - orgFirst ) - count ) + 1;
    } else if ( orgFirst > m_activeRow && to < m_activeRow ) {
        //we are moving a selection from below the active track to above it
        debug() << "we are moving a selection from below the active track to above it";
        m_activeRow += count;
    } else if ( orgFirst < m_activeRow && to > m_activeRow ) {
        //we are moving a selection from above the active track to below it
        debug() << "we are moving a selection from above the active track to below it";
        m_activeRow -= count;
    } else if ( to == m_activeRow && orgFirst > m_activeRow) {
        debug() << "we are moving something on top of the current track that was previously below it.";
        m_activeRow += count;
    }

    int offset = -1;
    if ( to < first )
        offset = 1;

    int min = first;
    int max = to + count -1;

    if ( first > to ) {
        min = to + count -1;
        max = first;
    }

    debug() << "min " << min << " max " << max;


    //regroupAlbums( min, max +1, OffsetBetween, offset );
    regroupAll();


    i = 0;

    if ( first < to ) {

        //moving stuff downwards
        foreach ( int row, rows ) {
            emit rowMoved( row - i, to );
            i++;
        }

    } else {

        //moving stuff upwards
        foreach ( int row, rows ) {
            emit rowMoved( row, to + i );
            i++;
        }

    }

    return true;
}

int Playlist::Model::firstInGroup( int row )
{

    QMapIterator< QString, AlbumGroup *> itt(m_albumGroups);
    while (itt.hasNext()) {

        itt.next();
        AlbumGroup * group = itt.value();

        int temp = group->firstInGroup( row );

        if ( temp != -1 ) return temp;

    }

    return -1;
}

int Playlist::Model::lastInGroup( int row )
{
    QMapIterator< QString, AlbumGroup *> itt(m_albumGroups);
    while (itt.hasNext()) {

        itt.next();
        AlbumGroup * group = itt.value();

        int temp = group->lastInGroup( row );

        if ( temp != -1 ) return temp;

    }

    return -1;
}


void
Playlist::Model::regroupAll()
{
    
    //delete all groups 
    m_albumGroups.clear();


    int i;
    for ( i = 0; i < m_items.count(); i++ )
    {

       //debug() << "i: " << i;

        Meta::TrackPtr track = m_items.at( i )->track();

        if ( !track->album() )
            continue;

        QString albumName;

       //not all tracks have a valid album ( radio stations for instance... )
        if ( !track->album().isNull() ) {
            albumName = track->album()->prettyName();
        }

        if ( m_albumGroups.contains( albumName ) && !albumName.isEmpty() ) {
            m_albumGroups[ albumName ]->addRow( i );
        } else {
            //debug() << "Create new group for album " << track->album()->name() ;
            AlbumGroup * newGroup = new AlbumGroup();
            newGroup->addRow( i );
            m_albumGroups.insert( albumName, newGroup );
        }
    }

    foreach( AlbumGroup *ag, m_albumGroups)
        ag->printGroupRows();

    emit( playlistGroupingChanged() );

   //reset();

}



namespace The {
    Playlist::Model* playlistModel() { return Playlist::Model::s_instance; }
}







#include "PlaylistModel.moc"

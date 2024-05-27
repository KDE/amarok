/****************************************************************************************
 * Copyright (c) 2007-2008 Ian Monroe <ian@monroe.nu>                                   *
 * Copyright (c) 2007-2008 Nikolaj Hald Nielsen <nhn@kde.org>                           *
 * Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>                                           *
 * Copyright (c) 2008 Soren Harward <stharward@gmail.com>                               *
 * Copyright (c) 2009 John Atkinson <john@fauxnetic.co.uk>                              *
 * Copyright (c) 2009,2010 Téo Mrnjavac <teo@kde.org>                                   *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#define DEBUG_PREFIX "Playlist::Controller"

// WORKAROUND for QTBUG-25960. Required for Qt versions < 4.8.5 in combination with libc++.
#define QT_NO_STL 1
    #include <qiterator.h>
#undef QT_NO_STL

#include "PlaylistController.h"

#include "EngineController.h"
#include "amarokconfig.h"
#include "core/collections/QueryMaker.h"
#include "core/support/Debug.h"
#include "core-impl/meta/cue/CueFileSupport.h"
#include "core-impl/meta/file/File.h"
#include "core-impl/meta/multi/MultiTrack.h"
#include "core-impl/playlists/types/file/PlaylistFileSupport.h"
#include "core-impl/support/TrackLoader.h"
#include "playlist/PlaylistActions.h"
#include "playlist/PlaylistModelStack.h"
#include "playlistmanager/PlaylistManager.h"

#include <QAction>

#include <algorithm>
#include <typeinfo>

using namespace Playlist;

namespace The
{
    AMAROK_EXPORT Controller* playlistController()
    {
        return Controller::instance();
    }
}


Controller* Controller::s_instance = nullptr;

Controller*
Controller::instance()
{
    if( s_instance == nullptr )
        s_instance = new Controller();
    return s_instance;
}

void
Controller::destroy()
{
    if( s_instance )
    {
        delete s_instance;
        s_instance = nullptr;
    }
}

Controller::Controller()
        : QObject()
        , m_undoStack( new QUndoStack( this ) )
{
    DEBUG_BLOCK

    //As a rule, when talking to the playlist one should always use the topmost model, as
    //Playlist::ModelStack::instance->top() or simply The::playlist().
    //This is an exception, because we handle the presence of tracks in the bottom model,
    //so we get a pointer to the bottom model and use it with great care.
    // TODO: get these values only when we really need them to loosen up the
    // coupling between Controller and Model
    m_bottomModel = ModelStack::instance()->bottom();
    m_topModel = The::playlist();

    m_undoStack->setUndoLimit( 20 );
    connect( m_undoStack, &QUndoStack::canRedoChanged, this, &Controller::canRedoChanged );
    connect( m_undoStack, &QUndoStack::canUndoChanged, this, &Controller::canUndoChanged );
}

Controller::~Controller() {}

void
Controller::insertOptioned( Meta::TrackPtr track, AddOptions options )
{
    if( !track )
        return;

    Meta::TrackList list;
    list.append( track );
    insertOptioned( list, options );
}

void
Controller::insertOptioned( Meta::TrackList list, AddOptions options )
{
    DEBUG_BLOCK
    /* Note: don't use (options & flag) here to test whether flag is present in options.
     * We have compound flags and for example (Queue & DirectPlay) == Queue, which
     * evaluates to true, which isn't usually what you want.
     *
     * Use (options & flag == flag) instead, or rather QFlag's convenience method:
     * options.testFlag( flag )
     */

    if( list.isEmpty() )
        return;

    QString actionName = i18nc( "name of the action in undo stack", "Add tracks to playlist" );
    if( options.testFlag( Queue ) )
        actionName = i18nc( "name of the action in undo stack", "Queue tracks" );
    if( options.testFlag( Replace ) )
        actionName = i18nc( "name of the action in undo stack", "Replace playlist" );
    m_undoStack->beginMacro( actionName );

    if( options.testFlag( Replace ) )
    {
        Q_EMIT replacingPlaylist();   //make sure that we clear filters
        clear();
        //make sure that we turn off dynamic mode.
        Amarok::actionCollection()->action( QStringLiteral("disable_dynamic") )->trigger();
    }

    int bottomModelRowCount = m_bottomModel->qaim()->rowCount();
    int bottomModelInsertRow;
    if( options.testFlag( Queue ) )
    {
        // queue is a list of playlist item ids
        QQueue<quint64> queue = Actions::instance()->queue();
        int activeRow = m_bottomModel->activeRow();

        if( options.testFlag( PrependToQueue ) )
        {
            if( activeRow >= 0 )
                bottomModelInsertRow = activeRow + 1; // right after active track
            else if( !queue.isEmpty() )
                bottomModelInsertRow = m_bottomModel->rowForId( queue.first() ); // prepend to queue
            else
                bottomModelInsertRow = bottomModelRowCount; // fallback: append to end
        }
        else // append to queue
        {
            if( !queue.isEmpty() )
                bottomModelInsertRow = m_bottomModel->rowForId( queue.last() ) + 1; // after queue
            else if( activeRow >= 0 )
                bottomModelInsertRow = activeRow + 1; // after active track
            else
                bottomModelInsertRow = bottomModelRowCount; // fallback: append to end
        }
    }
    else
        bottomModelInsertRow = bottomModelRowCount;

    // this guy does the thing:
    insertionHelper( bottomModelInsertRow, list );

    if( options.testFlag( Queue ) )
    {
        // Construct list of rows to be queued
        QList<quint64> ids;
        for( int bottomModelRow = bottomModelInsertRow;
             bottomModelRow < bottomModelInsertRow + list.size(); bottomModelRow++ )
        {
            ids << m_bottomModel->idAt( bottomModelRow );
        }

        if( options.testFlag( PrependToQueue ) ) // PrependToQueue implies Queue
        {
            // append current queue to new queue and remove it
            foreach( const quint64 id, Actions::instance()->queue() )
            {
                Actions::instance()->dequeue( id );
                ids << id;
            }
        }

        Actions::instance()->queue( ids );
    }

    m_undoStack->endMacro();

    bool startPlaying = false;
    EngineController *engine = The::engineController();
    if( options.testFlag( DirectPlay ) ) // implies PrependToQueue
        startPlaying = true;
    else if( options.testFlag( Playlist::StartPlayIfConfigured )
             && AmarokConfig::startPlayingOnAdd() && engine && !engine->isPlaying() )
    {
        // if nothing is in the queue, queue the first item we have added so that the call
        // to ->requestUserNextTrack() pops it. The queueing is therefore invisible to
        // user. Else we start playing the queue.
        if( Actions::instance()->queue().isEmpty() )
            Actions::instance()->queue( QList<quint64>() << m_bottomModel->idAt( bottomModelInsertRow ) );

        startPlaying = true;
    }

    if( startPlaying )
        Actions::instance()->requestUserNextTrack(); // desired track will be first in queue

    Q_EMIT changed();
}

void
Controller::insertOptioned( Playlists::PlaylistPtr playlist, AddOptions options )
{
    insertOptioned( Playlists::PlaylistList() << playlist, options );
}

void
Controller::insertOptioned( Playlists::PlaylistList list, AddOptions options )
{
    TrackLoader::Flags flags;
    // if we are going to play, we need full metadata (playable tracks)
    if( options.testFlag( DirectPlay ) || ( options.testFlag( Playlist::StartPlayIfConfigured )
        && AmarokConfig::startPlayingOnAdd() ) )
    {
        flags |= TrackLoader::FullMetadataRequired;
    }
    if( options.testFlag( Playlist::RemotePlaylistsAreStreams ) )
        flags |= TrackLoader::RemotePlaylistsAreStreams;
    TrackLoader *loader = new TrackLoader( flags ); // auto-deletes itself
    loader->setProperty( "options", QVariant::fromValue<AddOptions>( options ) );
    connect( loader, &TrackLoader::finished,
             this, &Controller::slotLoaderWithOptionsFinished );
    loader->init( list );
}

void
Controller::insertOptioned( const QUrl &url, AddOptions options )
{
    insertOptioned( QList<QUrl>() << url, options );
}

void
Controller::insertOptioned( QList<QUrl> &urls, AddOptions options )
{
    TrackLoader::Flags flags;
    // if we are going to play, we need full metadata (playable tracks)
    if( options.testFlag( DirectPlay ) || ( options.testFlag( Playlist::StartPlayIfConfigured )
        && AmarokConfig::startPlayingOnAdd() ) )
    {
        flags |= TrackLoader::FullMetadataRequired;
    }
    if( options.testFlag( Playlist::RemotePlaylistsAreStreams ) )
        flags |= TrackLoader::RemotePlaylistsAreStreams;
    TrackLoader *loader = new TrackLoader( flags ); // auto-deletes itself
    loader->setProperty( "options", QVariant::fromValue<AddOptions>( options ) );
    connect( loader, &TrackLoader::finished,
             this, &Controller::slotLoaderWithOptionsFinished );
    loader->init( urls );
}

void
Controller::insertTrack( int topModelRow, Meta::TrackPtr track )
{
    if( !track )
        return;
    insertTracks( topModelRow, Meta::TrackList() << track );
}

void
Controller::insertTracks( int topModelRow, Meta::TrackList tl )
{
    insertionHelper( insertionTopRowToBottom( topModelRow ), tl );
}

void
Controller::insertPlaylist( int topModelRow, Playlists::PlaylistPtr playlist )
{
    insertPlaylists( topModelRow, Playlists::PlaylistList() << playlist );
}

void
Controller::insertPlaylists( int topModelRow, Playlists::PlaylistList playlists )
{
    TrackLoader *loader = new TrackLoader(); // auto-deletes itself
    loader->setProperty( "topModelRow", QVariant( topModelRow ) );
    connect( loader, &TrackLoader::finished,
             this, &Controller::slotLoaderWithRowFinished );
    loader->init( playlists );
}

void
Controller::insertUrls( int topModelRow, QList<QUrl> &urls )
{
    TrackLoader *loader = new TrackLoader(); // auto-deletes itself
    loader->setProperty( "topModelRow", QVariant( topModelRow ) );
    connect( loader, &TrackLoader::finished,
             this, &Controller::slotLoaderWithRowFinished );
    loader->init( urls );
}

void
Controller::removeRow( int topModelRow )
{
    DEBUG_BLOCK
    removeRows( topModelRow, 1 );
}

void
Controller::removeRows( int topModelRow, int count )
{
    DEBUG_BLOCK
    QList<int> rl;
    for( int i = 0; i < count; ++i )
        rl.append( topModelRow++ );
    removeRows( rl );
}

void
Controller::removeRows( QList<int>& topModelRows )
{
    DEBUG_BLOCK
    RemoveCmdList bottomModelCmds;
    foreach( int topModelRow, topModelRows )
    {
        if( m_topModel->rowExists( topModelRow ) )
        {
            Meta::TrackPtr track = m_topModel->trackAt( topModelRow );    // For "undo".
            int bottomModelRow = m_topModel->rowToBottomModel( topModelRow );
            bottomModelCmds.append( RemoveCmd( track, bottomModelRow ) );
        }
        else
            warning() << "Received command to remove non-existent row. This should NEVER happen. row=" << topModelRow;
    }

    if( bottomModelCmds.size() > 0 )
        m_undoStack->push( new RemoveTracksCmd( nullptr, bottomModelCmds ) );

    Q_EMIT changed();
}

void
Controller::removeDeadAndDuplicates()
{
    DEBUG_BLOCK

    QList<int> topModelRowsToRemove;

    foreach( Meta::TrackPtr unique, m_topModel->tracks() )
    {
        QList<int> trackRows = m_topModel->allRowsForTrack( unique ).values();

        if( unique->playableUrl().isLocalFile() && !QFile::exists( unique->playableUrl().path() ) )
        {
            // Track is Dead
            // TODO: Check remote files as well
            topModelRowsToRemove << trackRows;
        }
        else if( trackRows.size() > 1 )
        {
            // Track is Duplicated
            // Remove all rows except the first
            std::sort( trackRows.begin(), trackRows.end() );
            for( QList<int>::const_iterator it = ++trackRows.constBegin(); it != trackRows.constEnd(); ++it )
                topModelRowsToRemove.push_back( *it );
        }
    }

    if( !topModelRowsToRemove.empty() )
    {
        QList<int> uniqueModelRowsToRemove = QSet<int>(topModelRowsToRemove.begin(), topModelRowsToRemove.end()).values();
        m_undoStack->beginMacro( QStringLiteral("Remove dead and duplicate entries") );     // TODO: Internationalize?
        removeRows( uniqueModelRowsToRemove );
        m_undoStack->endMacro();
    }
}

void
Controller::moveRow( int from, int to )
{
    DEBUG_BLOCK
    if( ModelStack::instance()->sortProxy()->isSorted() )
        return;
    if( from == to )
        return;

    QList<int> source;
    QList<int> target;
    source.append( from );
    source.append( to );

    // shift all the rows in between
    if( from < to )
    {
        for( int i = from + 1; i <= to; i++ )
        {
            source.append( i );
            target.append( i - 1 );
        }
    }
    else
    {
        for( int i = from - 1; i >= to; i-- )
        {
            source.append( i );
            target.append( i + 1 );
        }
    }

    reorderRows( source, target );
}

int
Controller::moveRows( QList<int>& from, int to )
{
    DEBUG_BLOCK
    if( from.size() <= 0 )
        return to;

    std::sort( from.begin(), from.end() );

    if( ModelStack::instance()->sortProxy()->isSorted() )
        return from.first();

    to = ( to == qBound( 0, to, m_topModel->qaim()->rowCount() ) ) ? to : m_topModel->qaim()->rowCount();

    from.erase( std::unique( from.begin(), from.end() ), from.end() );

    int min = qMin( to, from.first() );
    int max = qMax( to, from.last() );

    QList<int> source;
    QList<int> target;
    for( int i = min; i <= max; i++ )
    {
        if( i >=  m_topModel->qaim()->rowCount() )
            break; // we are likely moving below the last element, to an index that really does not exist, and thus should not be moved up.
        source.append( i );
        target.append( i );
    }

    int originalTo = to;

    foreach ( int f, from )
    {
        if( f < originalTo )
            to--; // since we are moving an item down in the list, this item will no longer count towards the target row
        source.removeOne( f );
    }


    // We iterate through the items in reverse order, as this allows us to keep the target row constant
    // (remember that the item that was originally on the target row is pushed down)
    QList<int>::const_iterator f_iter = from.constEnd();
    while( f_iter != from.constBegin() )
    {
        --f_iter;
        source.insert( ( to - min ), *f_iter );
    }

    reorderRows( source, target );

    return to;
}

void
Controller::reorderRows( const QList<int> &from, const QList<int> &to )
{
    DEBUG_BLOCK
    if( from.size() != to.size() )
        return;

    // validity check: each item should appear exactly once in both lists
    {
        QSet<int> fromItems( from.begin(), from.end() );
        QSet<int> toItems( to.begin(), to.end() );

        if( fromItems.size() != from.size() || toItems.size() != to.size() || fromItems != toItems )
        {
            error() << "row move lists malformed:";
            error() << from;
            error() << to;
            return;
        }
    }

    MoveCmdList bottomModelCmds;
    for( int i = 0; i < from.size(); i++ )
    {
        debug() << "moving rows:" << from.at( i ) << "->" << to.at( i );
        if( ( from.at( i ) >= 0 ) && ( from.at( i ) < m_topModel->qaim()->rowCount() ) )
            if( from.at( i ) != to.at( i ) )
                bottomModelCmds.append( MoveCmd( m_topModel->rowToBottomModel( from.at( i ) ), m_topModel->rowToBottomModel( to.at( i ) ) ) );
    }

    if( bottomModelCmds.size() > 0 )
        m_undoStack->push( new MoveTracksCmd( nullptr, bottomModelCmds ) );

    Q_EMIT changed();
}

void
Controller::undo()
{
    DEBUG_BLOCK
    m_undoStack->undo();
    Q_EMIT changed();
}

void
Controller::redo()
{
    DEBUG_BLOCK
    m_undoStack->redo();
    Q_EMIT changed();
}

void
Controller::clear()
{
    DEBUG_BLOCK
    removeRows( 0, ModelStack::instance()->bottom()->qaim()->rowCount() );
    Q_EMIT changed();
}

/**************************************************
 * Private Functions
 **************************************************/

void
Controller::slotLoaderWithOptionsFinished( const Meta::TrackList &tracks )
{
    QObject *loader = sender();
    if( !loader )
    {
        error() << __PRETTY_FUNCTION__ << "must be connected to TrackLoader";
        return;
    }
    QVariant options = loader->property( "options" );
    if( !options.canConvert<AddOptions>() )
    {
        error() << __PRETTY_FUNCTION__ << "loader property 'options' is not valid";
        return;
    }
    if( !tracks.isEmpty() )
        insertOptioned( tracks, options.value<AddOptions>() );
}

void
Controller::slotLoaderWithRowFinished( const Meta::TrackList &tracks )
{
    QObject *loader = sender();
    if( !loader )
    {
        error() << __PRETTY_FUNCTION__ << "must be connected to TrackLoader";
        return;
    }
    QVariant topModelRow = loader->property( "topModelRow" );
    if( !topModelRow.isValid() || topModelRow.type() != QVariant::Int )
    {
        error() << __PRETTY_FUNCTION__ << "loader property 'topModelRow' is not a valid integer";
        return;
    }
    if( !tracks.isEmpty() )
        insertTracks( topModelRow.toInt(), tracks );
}

int
Controller::insertionTopRowToBottom( int topModelRow )
{
    if( ( topModelRow < 0 ) || ( topModelRow > m_topModel->qaim()->rowCount() ) )
    {
        error() << "Row number invalid, using bottom:" << topModelRow;
        topModelRow = m_topModel->qaim()->rowCount();    // Failsafe: append.
    }

    if( ModelStack::instance()->sortProxy()->isSorted() )
        // if the playlist is sorted there's no point in placing the added tracks at any
        // specific point in relation to another track, so we just append them.
        return m_bottomModel->qaim()->rowCount();
    else
        return m_topModel->rowToBottomModel( topModelRow );
}

void
Controller::insertionHelper( int bottomModelRow, Meta::TrackList& tl )
{
    //expand any tracks that are actually playlists into multisource tracks
    //and any tracks with an associated cue file

    Meta::TrackList modifiedList;

    QMutableListIterator<Meta::TrackPtr> i( tl );
    while( i.hasNext() )
    {
        i.next();
        Meta::TrackPtr track = i.value();

        if( !track )
        {
            /*ignore*/
        }
        else if( MetaFile::TrackPtr::dynamicCast( track ) )
        {
            QUrl cuesheet = MetaCue::CueFileSupport::locateCueSheet( track->playableUrl() );
            if( !cuesheet.isEmpty() )
            {
                MetaCue::CueFileItemMap cueMap = MetaCue::CueFileSupport::loadCueFile( cuesheet, track );
                if( !cueMap.isEmpty() )
                {
                    Meta::TrackList cueTracks = MetaCue::CueFileSupport::generateTimeCodeTracks( track, cueMap );
                    if( !cueTracks.isEmpty() )
                        modifiedList <<  cueTracks;
                    else
                        modifiedList << track;
                }
                else
                    modifiedList << track;
            }
            else
                modifiedList << track;
        }
        else
        {
           modifiedList << track;
        }
    }

    InsertCmdList bottomModelCmds;

    foreach( Meta::TrackPtr t, modifiedList )
        bottomModelCmds.append( InsertCmd( t, bottomModelRow++ ) );

    if( bottomModelCmds.size() > 0 )
        m_undoStack->push( new InsertTracksCmd( nullptr, bottomModelCmds ) );

    Q_EMIT changed();
}

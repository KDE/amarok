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

#include "PlaylistController.h"

#include "amarokconfig.h"
#include "core/support/Debug.h"
#include "EngineController.h"
#include "core/collections/QueryMaker.h"
#include "core-impl/meta/cue/CueFileSupport.h"
#include "playlist/PlaylistActions.h"
#include "playlist/PlaylistModelStack.h"
#include "playlistmanager/PlaylistManager.h"
#include "core-impl/playlists/types/file/PlaylistFileSupport.h"
#include "core-impl/meta/multi/MultiTrack.h"
#include "core-impl/meta/file/File.h"
#include "core-impl/support/TrackLoader.h"

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


Controller* Controller::s_instance = 0;

Controller*
Controller::instance()
{
    if( s_instance == 0 )
        s_instance = new Controller();
    return s_instance;
}

void
Controller::destroy()
{
    if( s_instance )
    {
        delete s_instance;
        s_instance = 0;
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
    connect( m_undoStack, SIGNAL(canRedoChanged(bool)), this, SIGNAL(canRedoChanged(bool)) );
    connect( m_undoStack, SIGNAL(canUndoChanged(bool)), this, SIGNAL(canUndoChanged(bool)) );
}

Controller::~Controller() {}

void
Controller::insertOptioned( Meta::TrackPtr track, AddOptions options )
{
    DEBUG_BLOCK
    if( ! track )
        return;

    Meta::TrackList list;
    list.append( track );
    insertOptioned( list, options );
}

void
Controller::insertOptioned( Meta::TrackList list, AddOptions options )
{
    DEBUG_BLOCK

    if( list.isEmpty() )
        return;

    QString actionName = i18nc( "name of the action in undo stack", "Add tracks to playlist" );
    if( options & Queue )
        actionName = i18nc( "name of the action in undo stack", "Queue tracks" );
    if( options & Replace )
        actionName = i18nc( "name of the action in undo stack", "Replace playlist" );
    m_undoStack->beginMacro( actionName );

    if( options & Replace )
    {
        emit replacingPlaylist();   //make sure that we clear filters
        clear();
        //make sure that we turn off dynamic mode.
        Amarok::actionCollection()->action( "disable_dynamic" )->trigger();
    }

    int bottomModelRowCount = m_bottomModel->qaim()->rowCount();
    int bottomModelInsertRow;
    if( options & Queue )
    {
        // queue is a list of playlist item ids
        QQueue<quint64> queue = Actions::instance()->queue();
        if( !queue.isEmpty() )
        {
            int lastQueueRow = m_bottomModel->rowForId( queue.last() );
            debug() << "queue is:" << queue << "lastQueueRow:" << lastQueueRow;
            bottomModelInsertRow = ( lastQueueRow >= 0 ) ? lastQueueRow + 1 : bottomModelRowCount;
        }
        else
        {
            int activeRow = m_bottomModel->activeRow();
            bottomModelInsertRow = ( activeRow >= 0 ) ? activeRow + 1 : bottomModelRowCount;
        }
    }
    else
        bottomModelInsertRow = m_bottomModel->qaim()->rowCount();

    int oldVisibleRowCount = m_topModel->qaim()->rowCount();
    insertionHelper( bottomModelInsertRow, list );
    int visibleInsertedRowCount = m_topModel->qaim()->rowCount() - oldVisibleRowCount;

    if( options & Queue )
    {
        // Construct list of rows to be queued
        QList<quint64> ids;
        for( int bottomModelRow = bottomModelInsertRow;
             bottomModelRow < bottomModelInsertRow + list.size(); bottomModelRow++ )
        {
            ids << m_bottomModel->idAt( bottomModelRow );
        }
        Actions::instance()->queue( ids );
    }
    m_undoStack->endMacro();

    bool playNow = false;
    if( options & DirectPlay )
        playNow = true;

    if( options & StartPlay )
    {
        bool isDynamic = AmarokConfig::dynamicMode();
        bool isPaused  = The::engineController()->isPaused();
        bool isPlaying = The::engineController()->isPlaying();

        // when not in dyanmic mode: start playing when paused
        // when in dyanmic mode: do not start playing when paused
        if( !isPlaying && (!isDynamic || (isDynamic && !isPaused)) )
            playNow = true;
    }

    if( playNow && visibleInsertedRowCount > 0 )
    {
        int fuzz = 0;
        if ( AmarokConfig::trackProgression() == AmarokConfig::EnumTrackProgression::RandomTrack ||
             AmarokConfig::trackProgression() == AmarokConfig::EnumTrackProgression::RandomAlbum )
            fuzz = qrand() % visibleInsertedRowCount;
        Actions::instance()->play( m_topModel->rowFromBottomModel( bottomModelInsertRow ) + fuzz );
    }

    emit changed();
}

void
Controller::insertOptioned( Playlists::PlaylistPtr playlist, AddOptions options )
{
    insertOptioned( Playlists::PlaylistList() << playlist, options );
}

void
Controller::insertOptioned( Playlists::PlaylistList list, AddOptions options )
{
    // if we are going to play, we need full metadata (playable tracks)
    TrackLoader::Flags flags = ( options & ( StartPlay | DirectPlay ) )
                             ? TrackLoader::FullMetadataRequired : TrackLoader::NoFlags;
    TrackLoader *loader = new TrackLoader( flags ); // auto-deletes itself
    loader->setProperty( "options", QVariant::fromValue<AddOptions>( options ) );
    connect( loader, SIGNAL(finished(Meta::TrackList)),
                 SLOT(slotLoaderWithOptionsFinished(Meta::TrackList)) );
    loader->init( list );
}

void
Controller::insertOptioned( QList<KUrl> &urls, AddOptions options )
{
    // if we are going to play, we need full metadata (playable tracks)
    TrackLoader::Flags flags = ( options & ( StartPlay | DirectPlay ) )
                             ? TrackLoader::FullMetadataRequired : TrackLoader::NoFlags;
    TrackLoader *loader = new TrackLoader( flags ); // auto-deletes itself
    loader->setProperty( "options", QVariant::fromValue<AddOptions>( options ) );
    connect( loader, SIGNAL(finished(Meta::TrackList)),
                 SLOT(slotLoaderWithOptionsFinished(Meta::TrackList)) );
    loader->init( urls );
}

void
Controller::insertTrack( int topModelRow, Meta::TrackPtr track )
{
    DEBUG_BLOCK
    if( !track )
        return;

    Meta::TrackList tl;
    tl.append( track );
    insertTracks( topModelRow, tl );
}

// Overloads use this method.
void
Controller::insertTracks( int topModelRow, Meta::TrackList tl )
{
    DEBUG_BLOCK
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
    connect( loader, SIGNAL(finished(Meta::TrackList)),
                 SLOT(slotLoaderWithRowFinished(Meta::TrackList)) );
    loader->init( playlists );
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
        m_undoStack->push( new RemoveTracksCmd( 0, bottomModelCmds ) );

    emit changed();
}

void
Controller::removeDeadAndDuplicates()
{
    DEBUG_BLOCK

    QSet<Meta::TrackPtr> uniqueTracks = m_topModel->tracks().toSet();
    QList<int> topModelRowsToRemove;

    foreach( Meta::TrackPtr unique, uniqueTracks )
    {
        QList<int> trackRows = m_topModel->allRowsForTrack( unique ).toList();

        if( unique->playableUrl().isLocalFile() && !QFile::exists( unique->playableUrl().path() ) )
        {
            // Track is Dead
            // TODO: Check remote files as well
            topModelRowsToRemove <<  trackRows;
        }
        else if( trackRows.size() > 1 )
        {
            // Track is Duplicated
            // Remove all rows except the first
            for( QList<int>::const_iterator it = ++trackRows.constBegin(); it != trackRows.constEnd(); ++it )
                topModelRowsToRemove.push_back( *it );
        }
    }

    if( !topModelRowsToRemove.empty() )
    {
        m_undoStack->beginMacro( "Remove dead and duplicate entries" );     // TODO: Internationalize?
        removeRows( topModelRowsToRemove );
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

    moveRows( source, target );
}

int
Controller::moveRows( QList<int>& from, int to )
{
    DEBUG_BLOCK
    if( from.size() <= 0 )
        return to;

    qSort( from.begin(), from.end() );

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

    moveRows( source, target );

    return to;
}

void
Controller::moveRows( QList<int>& from, QList<int>& to )
{
    DEBUG_BLOCK
    if( from.size() != to.size() )
        return;

    // validity check
    foreach( int i, from )
    {
        if(( from.count( i ) != 1 ) || ( to.count( i ) != 1 ) )
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
        debug() << "moving rows:" << from.at( i ) << to.at( i );
        if( ( from.at( i ) >= 0 ) && ( from.at( i ) < m_topModel->qaim()->rowCount() ) )
            if( from.at( i ) != to.at( i ) )
                bottomModelCmds.append( MoveCmd( m_topModel->rowToBottomModel( from.at( i ) ), m_topModel->rowToBottomModel( to.at( i ) ) ) );
    }

    if( bottomModelCmds.size() > 0 )
        m_undoStack->push( new MoveTracksCmd( 0, bottomModelCmds ) );

    emit changed();
}

void
Controller::undo()
{
    DEBUG_BLOCK
    m_undoStack->undo();
    emit changed();
}

void
Controller::redo()
{
    DEBUG_BLOCK
    m_undoStack->redo();
    emit changed();
}

void
Controller::clear()
{
    DEBUG_BLOCK
    removeRows( 0, ModelStack::instance()->bottom()->qaim()->rowCount() );
    emit changed();
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

        if ( track == Meta::TrackPtr() )
        { /*ignore*/ }

        else if( Playlists::canExpand( track ) )
        {
            Playlists::PlaylistPtr playlist = Playlists::expand( track ); //expand() can return 0 if the KIO job times out
            if( playlist )
            {
                playlist->makeLoadingSync();
                playlist->triggerTrackLoad(); //playlist track loading is on demand.
                //since this is a playlist masqueurading as a single track, make a MultiTrack out of it:
                if ( playlist->tracks().count() > 0 )
                    modifiedList << Meta::TrackPtr( new Meta::MultiTrack( playlist ) );
            }
        }
        else if( typeid( *track.data() ) == typeid( MetaFile::Track  ) )
        {
            KUrl cuesheet = MetaCue::CueFileSupport::locateCueSheet( track->playableUrl() );
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
        m_undoStack->push( new InsertTracksCmd( 0, bottomModelCmds ) );

    emit changed();
}

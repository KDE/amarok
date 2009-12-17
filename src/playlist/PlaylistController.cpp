/****************************************************************************************
 * Copyright (c) 2007-2008 Ian Monroe <ian@monroe.nu>                                   *
 * Copyright (c) 2007-2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>               *
 * Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>                                           *
 * Copyright (c) 2008 Soren Harward <stharward@gmail.com>                               *
 * Copyright (c) 2009 John Atkinson <john@fauxnetic.co.uk>                              *
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

#include "PlaylistController.h"

#define DEBUG_PREFIX "Playlist::Controller"

#include "Debug.h"
#include "DirectoryLoader.h"
#include "EngineController.h"
#include "collection/QueryMaker.h"
#include "playlist/PlaylistActions.h"
#include "playlist/PlaylistModelStack.h"
#include "playlistmanager/PlaylistManager.h"
#include "PlaylistFileSupport.h"
#include "meta/multi/MultiTrack.h"


#include <algorithm> // STL
#include <QAction>

Playlist::Controller* Playlist::Controller::s_instance = 0;

Playlist::Controller* Playlist::Controller::instance()
{
    return ( s_instance ) ? s_instance : new Controller();
}

void
Playlist::Controller::destroy()
{
    if ( s_instance )
    {
        delete s_instance;
        s_instance = 0;
    }
}

Playlist::Controller::Controller( QObject* parent )
        : QObject( parent )
        , m_undoStack( new QUndoStack( this ) )
{
    s_instance = this;
    m_topmostModel = Playlist::ModelStack::instance()->top();

    m_undoStack->setUndoLimit( 20 );
    connect( m_undoStack, SIGNAL( canRedoChanged( bool ) ), this, SIGNAL( canRedoChanged( bool ) ) );
    connect( m_undoStack, SIGNAL( canUndoChanged( bool ) ), this, SIGNAL( canUndoChanged( bool ) ) );
}

Playlist::Controller::~Controller() {}

void
Playlist::Controller::insertOptioned( Meta::TrackPtr track, int options )
{
    DEBUG_BLOCK
    if ( track == Meta::TrackPtr() )
        return;

    Meta::TrackList list;
    list.append( track );
    insertOptioned( list, options );
}

void
Playlist::Controller::insertOptioned( Meta::TrackList list, int options )
{
    DEBUG_BLOCK
    if ( list.isEmpty() )
        return;

    if ( options & Unique )
    {
        QMutableListIterator<Meta::TrackPtr> i( list );
        while ( i.hasNext() )
        {
            i.next();
            if ( m_topmostModel->containsTrack( i.value() ) )
                i.remove();
        }
    }

    int firstItemAdded = -1;
    if ( options & Replace )
    {
        emit replacingPlaylist();

        m_undoStack->beginMacro( "Replace playlist" ); // TODO: does this need to be internationalized?
        clear();
        
        //make sure that we turn off dynamic mode.
        Amarok::actionCollection()->action( "disable_dynamic" )->trigger();
        
        insertionHelper( -1, list );
        m_undoStack->endMacro();
        firstItemAdded = 0;

    }
    else if ( options & Queue )
    {
        firstItemAdded = m_topmostModel->activeRow() + 1;
        // We want to add the newly queued items after any items which are already queued
        while( m_topmostModel->stateOfRow( firstItemAdded ) & Item::Queued )
            firstItemAdded++;

        insertionHelper( firstItemAdded, list );
        // Construct list of rows to be queued
        // NOTE: possible race condition here, if the rows get moved around?
        QList<int> rows;
        for( int i = firstItemAdded; i < firstItemAdded + list.size(); ++i )
            rows << i;
        Actions::instance()->queue( rows );
    }
    else
    {
        firstItemAdded = m_topmostModel->rowCount();
        insertionHelper( firstItemAdded, list );
    }

    const Phonon::State engineState = The::engineController()->state();
    debug() << "engine state: " << engineState;

    if ( options & DirectPlay )
    {
        Actions::instance()->play( firstItemAdded );
    }
    else if ( ( options & StartPlay ) && ( ( engineState == Phonon::StoppedState ) || ( engineState == Phonon::LoadingState ) ) )
        Actions::instance()->play( firstItemAdded );
}

void
Playlist::Controller::insertOptioned( Meta::PlaylistPtr playlist, int options )
{
    DEBUG_BLOCK
    if ( !playlist )
        return;

    insertOptioned( playlist->tracks(), options );
}

void
Playlist::Controller::insertOptioned( Meta::PlaylistList list, int options )
{
    DEBUG_BLOCK
    if ( list.isEmpty() )
        return;

    foreach( Meta::PlaylistPtr playlist, list )
    {
        insertOptioned( playlist, options );
    }
}

void
Playlist::Controller::insertOptioned( QueryMaker *qm, int options )
{
    DEBUG_BLOCK
    qm->setQueryType( QueryMaker::Track );
    connect( qm, SIGNAL( queryDone() ), SLOT( queryDone() ) );
    connect( qm, SIGNAL( newResultReady( QString, Meta::TrackList ) ), SLOT( newResultReady( QString, Meta::TrackList ) ) );
    m_optionedQueryMap.insert( qm, options );
    qm->run();
}

void
Playlist::Controller::insertOptioned( QList<KUrl>& urls, int options )
{
    DirectoryLoader* dl = new DirectoryLoader(); //dl handles memory management
    dl->setProperty( "options", QVariant( options ) );
    connect( dl, SIGNAL( finished( const Meta::TrackList& ) ), this, SLOT( slotFinishDirectoryLoader( const Meta::TrackList& ) ) );

    dl->init( urls );
}

void
Playlist::Controller::insertTrack( int row, Meta::TrackPtr track )
{
    DEBUG_BLOCK
    if ( track == Meta::TrackPtr() )
        return;

    Meta::TrackList tl;
    tl.append( track );
    insertionHelper( row, tl );
}

void
Playlist::Controller::insertTracks( int row, Meta::TrackList tl )
{
    DEBUG_BLOCK

    insertionHelper( row, tl );
}

void
Playlist::Controller::insertPlaylist( int row, Meta::PlaylistPtr playlist )
{
    DEBUG_BLOCK
    Meta::TrackList tl( playlist->tracks() );
    insertionHelper( row, tl );
}

void
Playlist::Controller::insertPlaylists( int row, Meta::PlaylistList playlists )
{
    DEBUG_BLOCK
    Meta::TrackList tl;
    foreach( Meta::PlaylistPtr playlist, playlists )
    {
        tl += playlist->tracks();
    }
    insertionHelper( row, tl );
}

void
Playlist::Controller::insertTracks( int row, QueryMaker *qm )
{
    DEBUG_BLOCK
    qm->setQueryType( QueryMaker::Track );
    connect( qm, SIGNAL( queryDone() ), SLOT( queryDone() ) );
    connect( qm, SIGNAL( newResultReady( QString, Meta::TrackList ) ), SLOT( newResultReady( QString, Meta::TrackList ) ) );
    m_queryMap.insert( qm, row );
    qm->run();
}

void
Playlist::Controller::insertUrls( int row, const QList<KUrl>& urls )
{
    Q_UNUSED( row );
    // FIXME: figure out some way to have this insert at the appropriate row, rather than always at end
    const int options = Append | DirectPlay;
    DirectoryLoader* dl = new DirectoryLoader(); //dl handles memory management
    dl->setProperty( "options", QVariant( options ) );
    connect( dl, SIGNAL( finished( const Meta::TrackList& ) ), this, SLOT( slotFinishDirectoryLoader( const Meta::TrackList& ) ) );

    dl->init( urls );
}

void
Playlist::Controller::removeRow( int row )
{
    DEBUG_BLOCK
    QList<int> rl;
    rl.append( row );
    removeRows( rl );
}

void
Playlist::Controller::removeRows( int row, int count )
{
    DEBUG_BLOCK
    QList<int> rl;
    for ( int i = 0; i < count; ++i )
        rl.append( row++ );
    removeRows( rl );
}

void
Playlist::Controller::removeRows( QList<int>& rows )
{
    DEBUG_BLOCK
    RemoveCmdList cmds;
    foreach( int r, rows )
    {
        if (( r >= 0 ) && ( r < m_topmostModel->rowCount() ) )
            cmds.append( RemoveCmd( m_topmostModel->trackAt( r ), m_topmostModel->rowToBottomModel( r ) ) );
        else
            warning() << "received command to remove non-existent row" << r;
    }

    if ( cmds.size() > 0 )
        m_undoStack->push( new RemoveTracksCmd( 0, cmds ) );
}

void
Playlist::Controller::removeDeadAndDuplicates()
{
    DEBUG_BLOCK

    QSet<Meta::TrackPtr> uniqueTracks = m_topmostModel->tracks().toSet();
    QList<int> rowsToRemove;

    foreach( Meta::TrackPtr unique, uniqueTracks )
    {
        QList<int> trackRows = m_topmostModel->allRowsForTrack( unique ).toList();

        if( unique->playableUrl().isLocalFile() && !QFile::exists( unique->playableUrl().path() ) )
        {
            // Track is Dead
            // TODO: Check remote files as well
            rowsToRemove <<  trackRows;
        }
        else if( trackRows.size() > 1 )
        {
            // Track is Duplicated
            // Remove all rows except the first
            for( QList<int>::const_iterator it = ++trackRows.constBegin(); it != trackRows.constEnd(); ++it )
                rowsToRemove.push_back( *it );
        }
    }

    if( !rowsToRemove.empty() )
    {
        m_undoStack->beginMacro( "Remove dead and duplicate entries" );     // TODO: Internationalize?
        removeRows( rowsToRemove );
        m_undoStack->endMacro();
    }
}

void
Playlist::Controller::moveRow( int from, int to )
{
    DEBUG_BLOCK
    if ( from == to )
        return;

    QList<int> source;
    QList<int> target;
    source.append( from );
    source.append( to );

    // shift all the rows between
    if ( from < to )
    {
        for ( int i = from + 1; i <= to; i++ )
        {
            source.append( i );
            target.append( i - 1 );
        }
    }
    else
    {
        for ( int i = from - 1; i >= to; i-- )
        {
            source.append( i );
            target.append( i + 1 );
        }
    }

    moveRows( source, target );
}

/* This function returns the real starting location where the rows ended up.
 * For example, if you start with the following playlist:
 *
 *   1 Alpha
 *   2 Bravo
 *   2 Charlie
 *   4 Delta
 *   5 Echo
 *   6 Foxtrot
 *
 * and you call moveRows( [1,2,3], 5 ) then the playlist will end up with
 *
 *   1 Delta
 *   2 Echo
 *   3 Alpha
 *   4 Bravo
 *   5 Charlie
 *   6 Foxtrot
 *
 * and the function will return 3, because that's where the rows really
 * ended up. */

int
Playlist::Controller::moveRows( QList<int>& from, int to )
{
    DEBUG_BLOCK
    if ( from.size() <= 0 )
        return to;

    to = ( to == qBound( 0, to, m_topmostModel->rowCount() ) ) ? to : m_topmostModel->rowCount();

    qSort( from.begin(), from.end() );
    from.erase( std::unique( from.begin(), from.end() ), from.end() );
    int min = qMin( to, from.first() );
    int max = qMax( to, from.last() );

    QList<int> source;
    QList<int> target;
    for ( int i = min; i <= max; i++ )
    {
        if ( i >=  m_topmostModel->rowCount() )
            break; // we are likely moving below the last element, to an index that really does not exist, and thus should not be moved up.
        source.append( i );
        target.append( i );
    }

    int originalTo = to;

    foreach ( int f, from )
    {
        if ( f < originalTo )
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
Playlist::Controller::moveRows( QList<int>& from, QList<int>& to )
{
    DEBUG_BLOCK
    if ( from.size() != to.size() )
        return;

    // validity check
    foreach( int i, from )
    {
        if (( from.count( i ) != 1 ) || ( to.count( i ) != 1 ) )
        {
            error() << "row move lists malformed:";
            error() << from;
            error() << to;
            return;
        }
    }

    MoveCmdList cmds;
    for ( int i = 0; i < from.size(); i++ )
    {
        debug() << "moving rows:" << from.at( i ) << to.at( i );
        if ( ( from.at( i ) >= 0 ) && ( from.at( i ) < m_topmostModel->rowCount() ) )
            if ( from.at( i ) != to.at( i ) )
                cmds.append( MoveCmd( m_topmostModel->rowToBottomModel( from.at( i ) ), m_topmostModel->rowToBottomModel( to.at( i ) ) ) );
    }

    if ( cmds.size() > 0 )
        m_undoStack->push( new MoveTracksCmd( 0, cmds ) );
}

void
Playlist::Controller::undo()
{
    m_undoStack->undo();
}

void
Playlist::Controller::redo()
{
    m_undoStack->redo();
}

void
Playlist::Controller::clear()
{
    DEBUG_BLOCK
    removeRows( 0, Playlist::ModelStack::instance()->source()->rowCount() );
}

/**************************************************
 * Private Functions
 **************************************************/

void
Playlist::Controller::newResultReady( const QString&, const Meta::TrackList& tracks )
{
    QueryMaker *qm = dynamic_cast<QueryMaker*>( sender() );
    if ( qm )
    {
        m_queryMakerTrackResults[qm] += tracks;
    }
}

void
Playlist::Controller::queryDone()
{
    QueryMaker *qm = dynamic_cast<QueryMaker*>( sender() );
    if ( qm )
    {
        qStableSort( m_queryMakerTrackResults[qm].begin(), m_queryMakerTrackResults[qm].end(), Meta::Track::lessThan );
        if ( m_queryMap.contains( qm ) )
        {
            insertTracks( m_queryMap.value( qm ), m_queryMakerTrackResults.value( qm ) );
            m_queryMap.remove( qm );
        }
        else if ( m_optionedQueryMap.contains( qm ) )
        {
            insertOptioned( m_queryMakerTrackResults.value( qm ), m_optionedQueryMap.value( qm ) );
            m_optionedQueryMap.remove( qm );
        }
        m_queryMakerTrackResults.remove( qm );
        qm->deleteLater();
    }
}

void
Playlist::Controller::slotFinishDirectoryLoader( const Meta::TrackList& tracks )
{
    DEBUG_BLOCK
    if ( !tracks.isEmpty() )
    {
        insertOptioned( tracks, sender()->property( "options" ).toInt() );
    }
}

void
Playlist::Controller::insertionHelper( int row, Meta::TrackList& tl )
{
    // expand any tracks that are actually playlists
    QMutableListIterator<Meta::TrackPtr> i( tl );
    while ( i.hasNext() )
    {
        i.next();
        Meta::TrackPtr track = i.value();
        if ( track == Meta::TrackPtr() )
            i.remove();
        else if( Meta::canExpand( track ) )
        {
            Meta::PlaylistPtr playlist = Meta::expand( track ); //expand() can return 0 if the KIO job times out
            if ( playlist )
            {
                //since this is a playlist masqueurading as a single track, make a MultiTrack out of it:
                i.remove();
                if ( playlist->tracks().count() > 0 )
                    i.insert( Meta::TrackPtr( new Meta::MultiTrack( playlist ) ) );
            }
        }
    }

    InsertCmdList cmds;

    row = qBound( 0, m_topmostModel->rowToBottomModel( row ), Playlist::ModelStack::instance()->source()->rowCount() );

    foreach( Meta::TrackPtr t, tl )
        cmds.append( InsertCmd( t, row++ ) );

    if ( cmds.size() > 0 )
        m_undoStack->push( new InsertTracksCmd( 0, cmds ) );
}

namespace The
{
AMAROK_EXPORT Playlist::Controller* playlistController()
{
    return Playlist::Controller::instance();
}
}


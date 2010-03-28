/****************************************************************************************
 * Copyright (c) 2003 Frederik Holljen <fh@ez.no>                                       *
 * Copyright (c) 2009 Mark Kretschmann <kretschmann@kde.org>                            *
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

#include "core/engine/EngineObserver.h"

#include "core/support/Debug.h"

using namespace Engine;

//////////////////////////////////////////////////////////////////////////////////////////
/// CLASS EngineObserver
//////////////////////////////////////////////////////////////////////////////////////////

EngineObserver::EngineObserver( EngineSubject *s )
    : m_subject( s )
{
    Q_ASSERT( m_subject );
    m_subject->attach( this );
}

EngineObserver::~EngineObserver()
{
    if( m_subject )
        m_subject->detach( this );
}

void
EngineObserver::engineStateChanged( Phonon::State currentState, Phonon::State oldState )
{
    Q_UNUSED( oldState );
    Q_UNUSED( currentState );
}

void
EngineObserver::enginePlaybackEnded( qint64 finalPosition, qint64 trackLength, PlaybackEndedReason reason )
{
    Q_UNUSED( finalPosition );
    Q_UNUSED( trackLength );
    Q_UNUSED( reason );
}

void
EngineObserver::engineTrackChanged( Meta::TrackPtr track )
{
    Q_UNUSED( track );
}

void
EngineObserver::engineTrackFinished( Meta::TrackPtr track )
{
    Q_UNUSED( track );
}

void
EngineObserver::engineNewTrackPlaying()
{
}

void
EngineObserver::engineNewMetaData( const QHash<qint64, QString> &newMetaData, bool trackChanged )
{
    Q_UNUSED( newMetaData );
    Q_UNUSED( trackChanged );
}

void
EngineObserver::engineVolumeChanged( int percent )
{
    Q_UNUSED( percent );
}

void
EngineObserver::engineMuteStateChanged( bool mute )
{
    Q_UNUSED( mute );
}

void
EngineObserver::engineTrackPositionChanged( qint64 position , bool userSeek )
{
    Q_UNUSED( position );
    Q_UNUSED( userSeek );
}

void
EngineObserver::engineTrackLengthChanged( qint64 milliseconds )
{
    Q_UNUSED( milliseconds );
}

void
EngineObserver::engineDeleted()
{
    m_subject = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////
/// CLASS EngineSubject
//////////////////////////////////////////////////////////////////////////////////////////

EngineSubject::EngineSubject()
    : QObject()
    , m_realState( Phonon::StoppedState )
{}

EngineSubject::~EngineSubject()
{
    // tell any remaining observers that we are gone
    foreach( EngineObserver *observer, Observers )
        observer->engineDeleted();

    //do not delete the observers, we don't have ownership of them!
}

void
EngineSubject::stateChangedNotify( Phonon::State newState, Phonon::State oldState )
{
    DEBUG_BLOCK

    // We explicitly block notifications where newState == buffering in enginecontroller, so if the old state = buffering we can ignore the playing update.
    if( newState == m_realState && newState != Phonon::PlayingState )  // To prevent Playing->Buffering->Playing->buffering.
        return;

    debug() << "State changed, oldState:" << oldState << "-> newState:" << newState;

    foreach( EngineObserver *observer, Observers )
        observer->engineStateChanged( newState, oldState );

    m_realState = newState;
}

void EngineSubject::playbackEnded( qint64 finalPosition, qint64 trackLength, EngineObserver::PlaybackEndedReason reason )
{
    foreach( EngineObserver *observer, Observers )
        observer->enginePlaybackEnded( finalPosition, trackLength, reason );
}

void
EngineSubject::newMetaDataNotify( const QHash<qint64, QString> &newMetaData, bool trackChanged )
{
    DEBUG_BLOCK

    if( trackChanged )
      m_metaDataHistory.clear();

    if( isMetaDataSpam( newMetaData ) )
      return;

    foreach( EngineObserver *observer, Observers )
        observer->engineNewMetaData( newMetaData, trackChanged );
}

void EngineSubject::volumeChangedNotify( int percent )
{
    foreach( EngineObserver *observer, Observers )
        observer->engineVolumeChanged( percent );
}

void EngineSubject::muteStateChangedNotify( bool mute )
{
    foreach( EngineObserver *observer, Observers )
        observer->engineMuteStateChanged( mute );
}

void EngineSubject::trackPositionChangedNotify( qint64 position, bool userSeek )
{
    foreach( EngineObserver *observer, Observers )
        observer->engineTrackPositionChanged( position, userSeek );
}


void EngineSubject::trackLengthChangedNotify( qint64 milliseconds )
{
    foreach( EngineObserver *observer, Observers )
        observer->engineTrackLengthChanged( milliseconds );
}

void
EngineSubject::newTrackPlaying() const
{
    foreach( EngineObserver *observer, Observers )
        observer->engineNewTrackPlaying();
}

void
EngineSubject::trackChangedNotify( Meta::TrackPtr track )
{
    foreach( EngineObserver *observer, Observers )
        observer->engineTrackChanged( track );
}

void
EngineSubject::trackFinishedNotify( Meta::TrackPtr track )
{
    foreach( EngineObserver *observer, Observers )
        observer->engineTrackFinished( track );
}

void
EngineSubject::attach( EngineObserver *observer )
{
    if( !observer || Observers.contains( observer ) )
        return;

    QObject* object = dynamic_cast<QObject*>( observer );
    if( object )
        connect( object, SIGNAL( destroyed( QObject* ) ), this, SLOT( observerDestroyed( QObject* ) ) );

    Observers.insert( observer );
}

void
EngineSubject::detach( EngineObserver *observer )
{
    Observers.remove( observer );
}

void
EngineSubject::observerDestroyed( QObject* object ) //SLOT
{
    DEBUG_BLOCK

    EngineObserver* observer = reinterpret_cast<EngineObserver*>( object ); // cast is OK, it's guaranteed to be an EngineObserver
    Observers.remove( observer );

    debug() << "Removed EngineObserver: " << observer;
}

/* Try to detect MetaData spam in Streams. */
bool
EngineSubject::isMetaDataSpam( QHash<qint64, QString> newMetaData )
{
    // search for Metadata in history
    for( int i = 0; i < m_metaDataHistory.size(); i++)
    {
        if( m_metaDataHistory.at( i ) == newMetaData ) // we already had that one -> spam!
        {
            m_metaDataHistory.move( i, 0 ); // move spam to the beginning of the list
            return true;
        }
    }

    if( m_metaDataHistory.size() == 12 )
        m_metaDataHistory.removeLast();

    m_metaDataHistory.insert( 0, newMetaData );
    return false;
}

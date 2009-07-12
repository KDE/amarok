/****************************************************************************************
 * Copyright (c) 2003 Frederik Holljen <fh@ez.no>                                       *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_ENGINEOBSERVER_H
#define AMAROK_ENGINEOBSERVER_H

#include "amarok_export.h"
#include "meta/Meta.h"

#include <Phonon/Global>
#include <QHash>
#include <QPointer>
#include <QSet>

class EngineSubject;
class QString;

/**
 * If you want to observe the engine, inherit from this class.
 * Note that all positional information and times are in milliseconds
 */
class AMAROK_EXPORT EngineObserver
{
public:
    enum PlaybackEndedReason
    {
        EndedStopped = 0,
        EndedQuit    = 1
    };

    EngineObserver();
    EngineObserver( EngineSubject* );
    virtual ~EngineObserver();
    /**
     * Called when the engine state changes
     */
    virtual void engineStateChanged( Phonon::State currentState, Phonon::State oldState = Phonon::StoppedState );

    // is this when playback stops completely, or when a track stops?
    virtual void enginePlaybackEnded( int finalPosition, int trackLength, PlaybackEndedReason reason );

    // should be called when a tracks starts or stops playing
    // on stop, pass Meta::TrackPtr( 0 )
    virtual void engineTrackChanged( Meta::TrackPtr track );

    // called when a new track starts playing
    virtual void engineNewTrackPlaying();

    // called when metadata changes for any reason
    virtual void engineNewMetaData( const QHash<qint64, QString> &newMetaData, bool trackChanged );

    /**
     * Called when the volume was changed
     */
    virtual void engineVolumeChanged( int percent );

    /**
     * Called when audio output was enabled or disabled
     *
     * NB: if setMute() was called on the engine controller, but it didn't change the
     * mute state, this will not be called
     */
    virtual void engineMuteStateChanged( bool mute );

    /**
     * Called when the track position changes
     *
     * (even when play just progresses?)
     */
    virtual void engineTrackPositionChanged( long position, bool userSeek );

    /**
     * Called when the track length changes, typically because the track has changed
     */
    virtual void engineTrackLengthChanged( long seconds );

    /**
     * Called when the EngineSubject is deleted.
     *
     * Warning: at this point, the destructor for the engine has already run!
     *
     * Currently not virtual on the assumption no-one (apart from the base
     * EngineObserver class itself) cares.
     */
    void engineDeleted();

private:
    EngineSubject *m_subject;
};

/**
 * Inherited by EngineController.
 * Notify observer functionality is captured in this class.
 */
class EngineSubject
{
    friend class EngineObserver;

protected:
    EngineSubject();
    virtual ~EngineSubject();
    void stateChangedNotify( Phonon::State newState, Phonon::State oldState );
    void playbackEnded( int finalPosition, int trackLength, EngineObserver::PlaybackEndedReason reason );
    void newMetaDataNotify( const QHash<qint64, QString> &newMetaData, bool trackChanged );
    void volumeChangedNotify( int percent );
    void muteStateChangedNotify( bool mute );
    /* userSeek means the position didn't change due to normal playback */
    void trackPositionChangedNotify( long position , bool userSeek = false );
    void trackLengthChangedNotify( long seconds );
    void newTrackPlaying() const;
    void trackChangedNotify( Meta::TrackPtr track );

private:
    void attach( EngineObserver *observer );
    void detach( EngineObserver *observer );
    bool isMetaDataSpam( QHash<qint64, QString> newMetaData );

    QList<QHash<qint64, QString> > m_metaDataHistory;
    QSet<EngineObserver*> Observers;
    Phonon::State m_realState; // To work around the buffering issue
};

#endif // AMAROK_ENGINEOBSERVER_H

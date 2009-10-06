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
    /**
     * Indicates why playback ended
     */
    enum PlaybackEndedReason
    {
        /**
         * Playback ended because the engine was stopped.
         *
         * This may be because the end of the playlist was reached or
         * because the user explicitly stopped playback.
         */
        EndedStopped = 0,
        /**
         * Playback ended because Amarok was asked to quit.
         *
         * In this case, playback may well resume from the same point
         * when Amarok is started again.
         */
        EndedQuit    = 1
    };

    /**
     * Initializes the EngineObserver to watch for notifications from
     * The::EngineController()
     */
    EngineObserver();
    /**
     * Initializes the EngineObserver to watch for notifications from
     * a specified EngineSubject.
     *
     * @param subject  the subject that notifications should be received from
     */
    EngineObserver( EngineSubject* subject );

    virtual ~EngineObserver();
    /**
     * Called when the engine state changes
     *
     * Note that you must not rely on this to tell you when a track changes.
     * Playing -> Playing is not a state change.  However, some Phonon
     * backends may potentially make the state changes
     * Playing -> Buffering -> Playing when a track changes.
     *
     * Use engineTrackChanged() or engineNewTrackPlaying() instead.
     */
    virtual void engineStateChanged( Phonon::State currentState, Phonon::State oldState = Phonon::StoppedState );

    // is this when playback stops completely, or when a track stops?
    virtual void enginePlaybackEnded( qint64 /*ms*/ finalPosition, qint64 /*ms*/ trackLength, PlaybackEndedReason reason );

    /**
     * Called when the current track changes
     *
     * Unlike engineNewTrackPlaying(), this is called when playback stops
     * with Meta::TrackPtr( 0 ) for @p track.
     *
     * @param track  the new track; may be null
     */
    virtual void engineTrackChanged( Meta::TrackPtr track );

    /**
     * Called when a new track starts playing
     *
     * Unlike engineTrackChanged(), this is not called when playback stops.
     */
    virtual void engineNewTrackPlaying();

    /**
     * Called when the track metadata changes
     *
     * NB: currently, this is triggered when the metadata <em>as seen by
     * the Phonon backend</em> changes, and only a limited subset of metadata
     * items will be passed in @p newMetaData.  Amarok internally knows
     * about much more metadata.
     *
     * See the Meta namespace for more metadata info.
     */
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
    virtual void engineTrackPositionChanged( qint64 position, bool userSeek );

    /**
     * Called when the track length changes, typically because the track has changed
     */
    virtual void engineTrackLengthChanged( qint64 milliseconds );

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
class EngineSubject : public QObject
{
    Q_OBJECT

    friend class EngineObserver;

protected:
    EngineSubject();
    virtual ~EngineSubject();
    void stateChangedNotify( Phonon::State newState, Phonon::State oldState );
    void playbackEnded( qint64 /*ms*/ finalPosition, qint64 /*ms*/ trackLength, EngineObserver::PlaybackEndedReason reason );
    void newMetaDataNotify( const QHash<qint64, QString> &newMetaData, bool trackChanged );
    void volumeChangedNotify( int percent );
    void muteStateChangedNotify( bool mute );
    /* userSeek means the position didn't change due to normal playback */
    void trackPositionChangedNotify( qint64 position, bool userSeek = false );
    void trackLengthChangedNotify( qint64 seconds );
    void newTrackPlaying() const;
    void trackChangedNotify( Meta::TrackPtr track );

private Q_SLOTS:
    void observerDestroyed( QObject* object );

private:
    void attach( EngineObserver *observer );
    void detach( EngineObserver *observer );
    bool isMetaDataSpam( QHash<qint64, QString> newMetaData );

    QList<QHash<qint64, QString> > m_metaDataHistory;
    QSet<EngineObserver*> Observers;
    Phonon::State m_realState; // To work around the buffering issue
};

#endif // AMAROK_ENGINEOBSERVER_H

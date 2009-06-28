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

#include <Phonon/MediaObject>
#include <QHash>
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

    EngineObserver( EngineSubject* );
    virtual ~EngineObserver();
    virtual void engineStateChanged( Phonon::State currentState, Phonon::State oldState = Phonon::StoppedState );
    virtual void enginePlaybackEnded( int finalPosition, int trackLength, PlaybackEndedReason reason );
    virtual void engineNewTrackPlaying();
    virtual void engineNewMetaData( const QHash<qint64, QString> &newMetaData, bool trackChanged );
    virtual void engineVolumeChanged( int percent );
    virtual void engineMuteStateChanged( bool mute );
    virtual void engineTrackPositionChanged( long position, bool userSeek );
    virtual void engineTrackLengthChanged( long seconds );

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
    void playbackEnded( int /*finalPosition*/, int /*trackLength*/, EngineObserver::PlaybackEndedReason reason );
    void newMetaDataNotify( const QHash<qint64, QString> &newMetaData, bool trackChanged );
    void volumeChangedNotify( int /*percent*/ );
    void muteStateChangedNotify( bool /*mute*/ );
    /* userSeek means the position didn't change due to normal playback */
    void trackPositionChangedNotify( long /*position*/ , bool userSeek=false );
    void trackLengthChangedNotify( long /*seconds*/ );
    void newTrackPlaying() const;

private:
    void attach( EngineObserver *observer );
    void detach( EngineObserver *observer );
    bool isMetaDataSpam( QHash<qint64, QString> newMetaData );

    QList<QHash<qint64, QString> > m_metaDataHistory;
    QSet<EngineObserver*> Observers;
    Phonon::State m_realState; // To work around the buffering issue
};

#endif // AMAROK_ENGINEOBSERVER_H

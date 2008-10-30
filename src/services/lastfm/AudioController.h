/***************************************************************************
 * copyright            : (C) 2007 Shane King <kde@dontletsstart.com>      *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef LASTFMAUDIOCONTROLLER_H
#define LASTFMAUDIOCONTROLLER_H

#include "EngineObserver.h"

#include <QStringList>

class AudioController : public QObject, public EngineObserver
{
    Q_OBJECT

public:
    AudioController( QObject *parent = 0 );
    virtual ~AudioController();

    void setVolume( int vol );
    void play();
    void play( RadioPlaylist &playlist );
    void play( const QUrl &trackUrl );
    void play( const TrackInfo &track );
    void stop();
    void loadNext();

    QString currentTrackUrl() const;

    // these are unused, dummy implementations should be OK
    QStringList soundSystems() { return QStringList(); }
    QStringList devices() { return QStringList(); }

    // EngineObserver
    virtual void engineStateChanged( Phonon::State currentState, Phonon::State oldState = Phonon::StoppedState );
    virtual void enginePlaybackEnded( int finalPosition, int trackLength, const QString &reason );
    virtual void engineNewTrackPlaying();

signals:
    void stateChanged( RadioState );
    void buffering( int, int );
    void error( RadioError, const QString & );
    void trackChanged( TrackInfo &, const TrackInfo & );
    void trackStarted( const TrackInfo & );
    void trackEnded( const TrackInfo &, int );

private slots:
    void slotStationName( const QString& name );
    void slotNewTracks( const QList< Track >& tracks );
        
private:
    void playTrack( const TrackInfo &track );

    RadioPlaylist *m_playlist;
    TrackInfo m_currentTrackInfo;
};

#endif // LASTFMAUDIOCONTROLLER_H

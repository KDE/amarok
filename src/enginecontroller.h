/***************************************************************************
 *   Copyright (C) 2004 Frederik Holljen <fh@ez.no>                        *
 *             (C) 2004,5 Max Howell <max.howell@methylblue.com>           *
 *             (C) 2004-2008 Mark Kretschmann <kretschmann@kde.org>        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROK_ENGINECONTROLLER_H
#define AMAROK_ENGINECONTROLLER_H

#include "engineobserver.h"
#include "meta/Meta.h"

#include <QMap>
#include <QObject>
#include <QPointer>

#include <Phonon/Global>
#include <Phonon/Path>
#include <Phonon/MediaSource> //Needed for the slot

class QTimer;

namespace KIO { class Job; }
namespace Meta { class MultiPlayableCapability; }
namespace Phonon { class AudioOutput; class MediaObject; class VolumeFaderEffect; }

/**
 * This class captures Amarok specific behaviour for some common features.
 * Accessing the engine directly is perfectly legal but on your own risk.
 * TODO: Hide proxy stuff!
 */

class AMAROK_EXPORT EngineController : public QObject, public EngineSubject
{
    Q_OBJECT

public:
    typedef QMap<QString, bool>  ExtensionCache;

    // plugins have their own static space, so calling instance
    // from a plugin won't do any good. you'll only get a new
    // instance with a voidEngine
    static EngineController* instance();
    static bool              canDecode( const KUrl& );
    static ExtensionCache&   extensionCache() { return s_extensionCache; }

    int trackPosition() const;

    Meta::TrackPtr currentTrack() const;
    int trackLength() const;

    void restoreSession();
    void endSession();

    //xx000, xx100, xx200, so at most will be 200ms delay before time displays are updated
    static const int MAIN_TIMER = 150;

    /*enum Filetype { MP3 };*/ //assuming MP3 for time being
    /*AMAROK_EXPORT*/ static bool installDistroCodec();

    const Phonon::MediaObject* phononMediaObject() const { return m_media; } //!const so that it's only used by DBus for info
    int volume() const;
    Engine::State state() const;
    bool loaded() { return instance()->state() != Engine::Empty; }
    bool getAudioCDContents(const QString &device, KUrl::List &urls);
    bool isStream();

public slots:
    void play();
    void play( const Meta::TrackPtr&, uint offset = 0 );
    void pause();
    void stop( bool forceInstant = false );
    void playPause(); //pauses if playing, plays if paused or stopped

    void seek( int ms );
    void seekRelative( int ms );
    void seekForward( int ms = 10000 );
    void seekBackward( int ms = 10000 );

    int increaseVolume( int ticks = 100/25 );
    int decreaseVolume( int ticks = 100/25 );
    int setVolume( int percent );

    void mute();

signals:
    void statusText( const QString& );
    void trackFinished();

protected:
    EngineController();
   ~EngineController();

    // undefined
    EngineController( const EngineController& );
    EngineController &operator=( const EngineController& );

    void playUrl( const KUrl &url, uint offset );
    void trackDone();

private slots:
    void slotAboutToFinish();
    void slotTrackEnded();
    void slotNewTrackPlaying( const Phonon::MediaSource &source);
    void slotStateChanged();
    void slotPlayableUrlFetched(const KUrl&);
    void slotTick( qint64 );
    void slotTrackLengthChanged( qint64 );
    void slotMetaDataChanged();
    void slotReallyStop(); //called after the fade-out has finished

private:
    static ExtensionCache s_extensionCache;

    Phonon::MediaObject *m_media;
    Phonon::AudioOutput *m_audio;
    Phonon::Path        m_path;
    QPointer<Phonon::VolumeFaderEffect> m_fader;

    bool m_isStream;

    Meta::TrackPtr  m_currentTrack;
    Meta::TrackPtr  m_lastTrack;
    QPointer<Meta::MultiPlayableCapability> m_multi;
};


#endif

/***************************************************************************
 *   Copyright (C) 2004 Frederik Holljen <fh@ez.no>                        *
 *             (C) 2004,5 Max Howell <max.howell@methylblue.com>           *
 *             (C) 2004,5 Mark Kretschmann                                 *
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

#include <Phonon/Global>
#include <Phonon/Path>

class QTimer;


namespace KIO { class Job; }
namespace Meta { class MultiPlayableCapability; }
namespace Phonon { class MediaObject; class AudioOutput; }

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

    qint64                  trackPosition() const;

    Meta::TrackPtr currentTrack() const;
    qint64 trackLength() const;

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
    void stop();
    void playPause(); //pauses if playing, plays if paused or stopped

    void seek( qint64 ms );
    void seekRelative( int ms );
    void seekForward( int ms = 10000 );
    void seekBackward( int ms = 10000 );

    int increaseVolume( int ticks = 100/25 );
    int decreaseVolume( int ticks = 100/25 );
    int setVolume( int percent );

    void mute();

signals:
    void orderCurrent();
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
    void slotTrackEnded();
    void slotStateChanged(Engine::State);
    void slotPlayableUrlFetched(const KUrl&);
private:
    static ExtensionCache s_extensionCache;
    Phonon::MediaObject *m_media;
    Phonon::AudioOutput *m_audio;
    Phonon::Path        m_path;
    bool                m_stream;
    Meta::TrackPtr  m_currentTrack;
    Meta::MultiPlayableCapability *m_multi;
};


#endif

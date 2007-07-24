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

#include "enginebase.h"
#include "engineobserver.h"
#include "meta/meta.h"

#include <QHash>
#include <QMap>
#include <QObject>


class QTimer;


namespace KIO { class Job; }

/**
 * This class captures Amarok specific behaviour for some common features.
 * Accessing the engine directly is perfectly legal but on your own risk.
 * TODO: Hide proxy stuff!
 */

class EngineController : public QObject, public EngineSubject
{
    Q_OBJECT

public:
    typedef QMap<QString, bool>  ExtensionCache;

    // plugins have their own static space, so calling instance
    // from a plugin won't do any good. you'll only get a new
    // instance with a voidEngine
    static EngineController* instance();
    static EngineBase*       engine() { return instance()->m_engine; }
    static bool              canDecode( const KUrl& );
    static ExtensionCache&   extensionCache() { return s_extensionCache; }
    static QString           engineProperty( const QString& key ) { return engine()->pluginProperty( key ); }
    static bool              hasEngineProperty( const QString& key ) { return engine()->hasPluginProperty( key ); }

    uint                     trackPosition() const;

    EngineBase* loadEngine();

    Meta::TrackPtr currentTrack() const;
    uint trackLength() const;
    KUrl previousURL() const { return m_previousUrl; }
    KUrl playingURL() const;

    void restoreSession();
    void endSession();

    //xx000, xx100, xx200, so at most will be 200ms delay before time displays are updated
    static const int MAIN_TIMER = 150;

    /*enum Filetype { MP3 };*/ //assuming MP3 for time being
    AMAROK_EXPORT static bool installDistroCodec(const QString& engine /*Filetype type*/);

public slots:
    void previous();
    // forceNext make we go to next track even if Repeat Track is on
    //NOTE If the track ended normally, call next(false) !
    void next( const bool forceNext = true );
    void play();
    void play( const Meta::TrackPtr&, uint offset = 0 );
    void pause();
    void stop();
    void playPause(); //pauses if playing, plays if paused or stopped

    void seek( int ms );
    void seekRelative( int ms );
    void seekForward( int ms = 10000 );
    void seekBackward( int ms = 10000 );

    int increaseVolume( int ticks = 100/25 );
    int decreaseVolume( int ticks = 100/25 );
    int setVolume( int percent );

    void mute();

    void playlistChanged() { m_engine->playlistChanged(); }

signals:
    void orderPrevious();
    void orderCurrent();
    void orderNext( const bool );
    void statusText( const QString& );
    void trackFinished();

private slots:
    void slotEngineMetaData( const QHash<qint64, QString> &newMetaData );
    void slotMainTimer();
    void slotTrackEnded();
    void slotStateChanged( Engine::State );
    void trackDone() { emit trackFinished(); next(false); }
protected:
    EngineController();
   ~EngineController();

    // undefined
    EngineController( const EngineController& );
    EngineController &operator=( const EngineController& );

private:
    static ExtensionCache s_extensionCache;

    EngineBase* loadEngine( const QString &engineName );

    EngineBase*     m_engine;
    EngineBase*     m_voidEngine;
    KUrl            m_previousUrl;
    long            m_delayTime;
    int             m_muteVolume;
    bool            m_xFadeThisTrack;
    bool            m_isTiming;
    QTimer*         m_timer;
    uint            m_playFailureCount;
    // try to correct start time for tracks from last.fm streams
    bool            m_lastFm;
    uint            m_positionOffset, m_lastPositionOffset;
    Meta::TrackPtr  m_currentTrack;
};


#endif

/***************************************************************************
                      enginecontroller.h  -  Wraps engine and adds some functionality
                         -------------------
begin                : Mar 15 2004
copyright            : (C) 2004 by Frederik Holljen
                       (C) 2004 by Max Howell
email                : fh@ez.no
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROK_ENGINECONTROLLER_H
#define AMAROK_ENGINECONTROLLER_H

#include "engineobserver.h" // move me // where to sir?
#include "metabundle.h"
#include <qmap.h>
#include <qobject.h>
#include <qtimer.h>

typedef QMap<QString, bool>  ExtensionCache;

namespace amaroK { class StreamProvider; }
namespace KIO { class Job; }

/**
 * This class captures amaroK specific behaviour for some common features.
 * Accessing the engine directly is perfectly legal but on your own risk.
 * TODO: Hide proxy stuff!
 */

class EngineController : public QObject, public EngineSubject
{
    Q_OBJECT
public:
    // plugins have their own static space, so calling instance from a plugin won't do any good.
    // you'll only get a new (empty) instance.
    static EngineController *instance();
    static EngineBase       *engine() { return instance()->m_pEngine; }
    static EngineBase       *loadEngine();
    static bool              canDecode( const KURL& );
    static ExtensionCache   &extensionCache() { return s_extensionCache; }

    uint trackLength() const { return m_bundle.length() * 1000; }
    const MetaBundle &bundle() const { return m_bundle; }
    const KURL &playingURL() const { return m_bundle.url(); }

public slots:
    void previous();
    void next();
    void play();
    void play( const MetaBundle& );
    void pause();
    void stop();
    void playPause(); //pauses if playing, plays if paused or stopped

    void seek( int seconds ) { engine()->seek( seconds * 1000 ); }

    int increaseVolume( int ticks = 100/25 );
    int decreaseVolume( int ticks = 100/25 );
    int setVolume( int percent );

    void mute();

signals:
    void orderNext();
    void orderPrevious();
    void orderCurrent();
    void statusText( const QString& );

private slots:
    void playRemote( KIO::Job* );
    void slotMainTimer();
    void slotTrackEnded();
    void slotStateChanged( Engine::State );
    void slotNewMetaData( const MetaBundle& );

private:
    EngineController();

private:
    static ExtensionCache s_extensionCache;

    //xx000, xx100, xx200, so at most will be 200ms delay before time displays are updated
    static const int MAIN_TIMER = 300;

    EngineBase*     m_pEngine;
    MetaBundle      m_bundle;
    QTimer*         m_pMainTimer;
    long            m_delayTime;
    int             m_muteVolume;
    bool            m_xFadeThisTrack;
    QTimer          m_timer;
    
    amaroK::StreamProvider* m_stream;
};


inline EngineController*
EngineController::instance()
{
    //will only be instantiated the first time this function is called
    //will work with the inline directive
    //TODO there may be issues on older GCC versions where an instance exists in every translation unit
    static EngineController Instance;

    return &Instance;
}

#endif

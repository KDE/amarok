/***************************************************************************
                      enginecontroller.h  -  Wraps engine and adds some functionality
                         -------------------
begin                : Mar 15 2004
copyright            : (C) 2004 by Frederik Holljen
                       (C) 2004 by Max Howell
                       (C) 2004 by Mark Kretschmann
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

#include <qguardedptr.h>
#include <qmap.h>
#include <qobject.h>

typedef QMap<QString, bool>  ExtensionCache;

class QTimer;

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
    // plugins have their own static space, so calling instance
    // from a plugin won't do any good. you'll only get a new
    // instance with a dummyEngine
    static EngineController *instance();
    static EngineBase       *engine() { return instance()->m_engine; }
    static EngineBase       *loadEngine();
    static bool              canDecode( const KURL& );
    static ExtensionCache   &extensionCache() { return s_extensionCache; }
    static QString           engineProperty( const QString& key ) { return engine()->pluginProperty( key ); }
    static bool              hasEngineProperty( const QString& key ) { return engine()->hasPluginProperty( key ); }

    uint trackLength() const { return m_bundle.length() * 1000; }
    const MetaBundle &bundle() const;
    const KURL &playingURL() const { return m_bundle.url(); }

    void restoreSession();
    void endSession();

public slots:
    void previous();
    void next();
    void play();
    void play( const MetaBundle& );
    void pause();
    void stop();
    void playPause(); //pauses if playing, plays if paused or stopped

    void seek( int ms ) { engine()->seek( ms ); }

    int increaseVolume( int ticks = 100/25 );
    int decreaseVolume( int ticks = 100/25 );
    int setVolume( int percent );

    void mute();

signals:
    void orderPrevious();
    void orderCurrent();
    void orderNext();
    void statusText( const QString& );

private slots:
    void playRemote( KIO::Job* );
    void slotStreamMetaData( const MetaBundle& );
    void slotMainTimer();
    void slotTrackEnded();
    void slotStateChanged( Engine::State );
    void streamError();

private:
    EngineController();

private:
    static bool s_initialised;
    static ExtensionCache s_extensionCache;

    //xx000, xx100, xx200, so at most will be 200ms delay before time displays are updated
    static const int MAIN_TIMER = 300;

    EngineBase*     m_engine;
    MetaBundle      m_bundle;
    long            m_delayTime;
    int             m_muteVolume;
    bool            m_xFadeThisTrack;
    QTimer*         m_timer;

    QGuardedPtr<amaroK::StreamProvider> m_stream;
};


#endif

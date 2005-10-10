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
#include "metabundle.h"

#include <qguardedptr.h>
#include <qmap.h>
#include <qobject.h>
#include <qvaluelist.h>

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
    typedef QMap<QString, bool>  ExtensionCache;

    // plugins have their own static space, so calling instance
    // from a plugin won't do any good. you'll only get a new
    // instance with a voidEngine
    static EngineController* instance();
    static EngineBase*       engine() { return instance()->m_engine; }
    static bool              canDecode( const KURL& );
    static ExtensionCache&   extensionCache() { return s_extensionCache; }
    static QString           engineProperty( const QString& key ) { return engine()->pluginProperty( key ); }
    static bool              hasEngineProperty( const QString& key ) { return engine()->hasPluginProperty( key ); }

    EngineBase* loadEngine();

    uint trackLength() const { return m_bundle.length() * 1000; }
    const MetaBundle &bundle() const;
    const KURL &playingURL() const { return bundle().url(); }

    void restoreSession();
    void endSession();

    void currentTrackMetaDataChanged( const MetaBundle& bundle ) { m_bundle = bundle; newMetaDataNotify( bundle, false /* no track change */ ); }

    //xx000, xx100, xx200, so at most will be 200ms delay before time displays are updated
    static const int MAIN_TIMER = 300;

public slots:
    void previous();
    // forceNext make we go to next track even if Repeat Track is on
    //NOTE If the track ended normally, call next(false) !
    void next( const bool forceNext = true );
    void trackFinished() { next(false); };
    void play();
    void play( const MetaBundle& );
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

signals:
    void orderPrevious();
    void orderCurrent();
    void orderNext( const bool );
    void statusText( const QString& );

private slots:
    void playRemote( KIO::Job* );
    void slotStreamMetaData( const MetaBundle& );
    void slotEngineMetaData( const Engine::SimpleMetaBundle& );
    void slotMainTimer();
    void slotTrackEnded();
    void slotStateChanged( Engine::State );
    void streamError();
    void slotSigError();
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
    MetaBundle      m_bundle;
    BundleList      m_lastMetadata;
    long            m_delayTime;
    int             m_muteVolume;
    bool            m_xFadeThisTrack;
    bool            m_isTiming;
    QTimer*         m_timer;

    QGuardedPtr<amaroK::StreamProvider> m_stream;
};


#endif

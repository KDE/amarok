/***************************************************************************
                      enginecontroller.h  -  Wraps engine and adds some functionality
                         -------------------
begin                : Mar 15 2004
copyright            : (C) 2004 by Frederik Holljen
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

#include <qobject.h>
#include "engineobserver.h" // move me
#include "metabundle.h"

class EngineBase;
class QTimer;

/**
 * This class captures amaroK specific behaviour for some common features.
 * Accessing the engine directly is perfectly legal but on your own risk.
 * TODO: Hide proxy stuff!
 */
class EngineController : public QObject, public EngineSubject
{
    Q_OBJECT
public:
    virtual ~EngineController();

    // plugins have their own static space, so calling instance from a plugin won't do any good.
    // you'll only get a new (empty) instance.
    static EngineController *instance();

    static EngineBase *engine() { return instance()->m_pEngine; }
    static void setEngine( EngineBase* );
    long trackLength() const { return m_bundle.length() * 1000; }
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

    int increaseVolume();
    int decreaseVolume();
    int setVolume( int percent );

signals:
    void orderNext();
    void orderPrevious();
    void orderCurrent();

    void deleteProxy();    // proxy error stuff. Move to private class?

private slots:
    void slotMainTimer();
    void newMetaData( const MetaBundle & );

private:
    EngineController();

private:
    static const int MAIN_TIMER  = 150;

    EngineBase *m_pEngine;
    MetaBundle m_bundle;
    bool m_proxyError;
    QTimer *m_pMainTimer;
    long m_delayTime;
};

#endif

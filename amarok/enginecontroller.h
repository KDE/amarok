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
#include <kurl.h>
#include "engine/engineobserver.h" // move me

class EngineBase;
class MetaBundle;
class MainTimer;

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

    EngineBase *engine() const { return m_pEngine; }
    void setEngine( EngineBase *engine ) { m_pEngine = engine; }
    long trackLength() const { return m_length; }  // how about : const MetaBundle &currentTrack() const;
    const KURL &playingURL() { return m_playingURL; } // see over.

public slots:
    void previous();
    void next();
    void play();
    void play( const MetaBundle& );
    void pause();
    void stop();

    int setVolume( int percent );

    void proxyError();     // proxy error stuff. Move to private class?

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
    static EngineController *Instance;
    // TODO:
    //start with a dummy engine that has no capabilities but ensures that amaroK always starts with
    //something even if configuration is corrupt or engine is not compiled into new amaroK etc.
    EngineBase *m_pEngine;
    KURL m_playingURL;
    bool m_proxyError;
    QTimer *m_pMainTimer;
    long m_length;
    long m_delayTime;
};

#endif

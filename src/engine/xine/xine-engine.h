/***************************************************************************
 *   Copyright (C) 2004,5 Max Howell <max.howell@methylblue.com>           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef XINE_ENGINE_H
#define XINE_ENGINE_H

#include "engine/enginebase.h"
#include <qthread.h>
#include <sys/types.h>
#include <xine.h>

class XineEngine : public Engine::Base
{
    Q_OBJECT

    friend class Fader;

public:
    XineEngine();
   ~XineEngine();

    virtual bool init();
    virtual bool canDecode( const KURL& ) const;
    virtual bool load( const KURL &url, bool stream );
    virtual bool play( uint = 0 );
    virtual void stop();
    virtual void pause();
    virtual uint position() const;
    virtual uint length() const;
    virtual void seek( uint );

    virtual Engine::State state() const;
    virtual const Engine::Scope &scope();

    virtual amaroK::PluginConfig *configure() const;

    virtual void setEqualizerEnabled( bool );
    virtual void setEqualizerParameters( int preamp, const QValueList<int>& );

protected:
    virtual void setVolumeSW( uint );

private:
    static  void XineEventListener( void*, const xine_event_t* );
    virtual void customEvent( QCustomEvent* );
    virtual void timerEvent( QTimerEvent* );

    bool makeNewStream();

    xine_t             *m_xine;
    xine_stream_t      *m_stream;
    xine_audio_port_t  *m_audioPort;
    xine_event_queue_t *m_eventQueue;
    xine_post_t        *m_post;

    int64_t             m_currentVpts;
};

class Fader : public QObject, public QThread
{
    xine_t             *m_xine;
    xine_stream_t      *m_decrease;
    xine_stream_t      *m_increase;
    xine_audio_port_t  *m_port;
    xine_post_t        *m_post;
public:
    Fader( XineEngine* );
   ~Fader();
    virtual void run();
};

#endif

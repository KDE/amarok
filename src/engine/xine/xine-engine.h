//Copyright: (C) 2004 Max Howell, <max.howell@methylblue.com>
//License:   See xine-engine.cpp

#ifndef XINE_ENGINE_H
#define XINE_ENGINE_H

#include "engine/enginebase.h"
#include <xine.h>

class XineEngine : public Engine::Base
{
Q_OBJECT

public:
    XineEngine();
    ~XineEngine();

    bool init();

    bool canDecode( const KURL& );

    const Engine::Scope &scope();

    Engine::State state() const;

    bool load( const KURL &url, bool stream );
    bool play( uint = 0 );
    void stop();
    void pause();

    uint position() const;
    void seek( uint );

    void setVolumeSW( uint );

private:
    static  void XineEventListener( void*, const xine_event_t* );
    virtual void customEvent( QCustomEvent* );
    virtual void timerEvent( QTimerEvent* );

    xine_t             *m_xine;
    xine_stream_t      *m_stream;
    xine_audio_port_t  *m_audioPort;
    xine_event_queue_t *m_eventQueue;
    xine_post_t        *m_post;
};

#endif

//Copyright: (C) 2004 Max Howell, <max.howell@methylblue.com>
//Copyright: (C) 2003-2004 J. Kofler, <kaffeine@gmx.net>
//License:   See xine-engine.cpp

#ifndef VIDEOWINDOW_H
#define VIDEOWINDOW_H

#include "engine/enginebase.h"
#include <kurl.h>
#include <xine.h>

class XineEngine : public EngineBase
{
Q_OBJECT

public:
    XineEngine();
    ~XineEngine();

    void init( bool&, int, bool );

    bool initMixer( bool );
    bool canDecode( const KURL&, mode_t, mode_t );
    long position() const;
    bool isStream() const { return false; }

    std::vector<float>* scope();

    EngineBase::EngineState state() const;

    void  play( const KURL &url ) { m_url = url; play(); }
    void  play();
    void  stop();
    void  pause();

    void  seek( long );
    void  setVolume( int );

private:
    static  void XineEventListener(void* p, const xine_event_t*);
    virtual void customEvent( QCustomEvent* );

    xine_t             *m_xine;
    xine_stream_t      *m_stream;
    xine_audio_port_t  *m_audioPort;
    xine_event_queue_t *m_eventQueue;
    xine_post_t        *m_post;

    KURL m_url;
};

typedef struct post_class_s post_class_t;

extern "C"
{
    post_class_t*
    scope_init_plugin( xine_t* );
}

#endif

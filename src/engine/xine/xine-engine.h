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

    bool initMixer( bool hardware ) { return false; }
    bool canDecode( const KURL&, mode_t, mode_t );
    long position() const;
    bool isStream() const { return false; }

    EngineBase::EngineState state() const;

    void  play( const KURL &url ) { m_url = url; play(); }
    void  play();
    void  stop();
    void  pause();

    void  seek( long );
    void  setVolume( int );

private:
    static void XineEventListener(void* p, const xine_event_t*);
    static void AudioDriverChangedCallback(void* p, xine_cfg_entry_t* entry);
    static void AudioMixerMethodChangedCallback(void* p, xine_cfg_entry_t* entry);

    virtual void customEvent( QCustomEvent* );

    xine_t             *xineEngine;
    xine_stream_t      *xineStream;
    xine_audio_port_t  *audioDriver;
    xine_event_queue_t *eventQueue;

    QString audioDriverName;

    KURL m_url;
};

#endif

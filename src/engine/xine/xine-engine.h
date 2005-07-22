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

extern "C"
{
    #include <sys/types.h>
    #include <xine.h>
}


class XineEngine : public Engine::Base
{
    Q_OBJECT

    friend class Fader;

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
    virtual void setVolumeSW( uint );

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
    float               m_preamp;

    bool                m_stopFader;

    QString             m_currentAudioPlugin; //to see if audio plugin has been changed
    XineConfigDialog*   m_configDialog;
    //need to save these for when the audio plugin is changed and xine reloaded
    bool                m_equalizerEnabled;
    int                 m_intPreamp;
    QValueList<int>     m_equalizerGains;

private slots:
    void configChanged();

public:
    XineEngine();
signals:
    void resetConfig(xine_t *xine);
};

class Fader : public QObject, public QThread
{
    XineEngine         *m_engine;
    xine_t             *m_xine;
    xine_stream_t      *m_decrease;
    xine_stream_t      *m_increase;
    xine_audio_port_t  *m_port;
    xine_post_t        *m_post;

    virtual void run();

public:
    Fader( XineEngine* );
   ~Fader();
};

#endif

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

#include "amarok_engines_export.h"
#include "enginebase.h"
#include <QThread>
//Added by qt3to4:
#include <QCustomEvent>
#include <Q3ValueList>
#include <QTimerEvent>

extern "C"
{
    #include <sys/types.h>
    #include <xine.h>
}

class XineConfigDialog;

class AMAROK_XINE_ENGINE_EXPORT XineEngine : public Engine::Base
{
    Q_OBJECT

    friend class Fader;
    friend class OutFader;

   ~XineEngine();

    virtual bool init();
    virtual bool canDecode( const KUrl& ) const;
    virtual bool load( const KUrl &url, bool stream );
    virtual bool play( uint = 0 );
    virtual void stop();
    virtual void pause();
    virtual void unpause();
    virtual uint position() const;
    virtual uint length() const;
    virtual void seek( uint );

    virtual bool metaDataForUrl(const KUrl &url, Engine::SimpleMetaBundle &b);
    virtual bool getAudioCDContents(const QString &device, KUrl::List &urls);
    virtual bool flushBuffer();

    virtual Engine::State state() const;
    virtual const Engine::Scope &scope();

    virtual Amarok::PluginConfig *configure() const;
    virtual void setEqualizerEnabled( bool );
    virtual void setEqualizerParameters( int preamp, const Q3ValueList<int>& );
    virtual void setVolumeSW( uint );
    virtual void fadeOut( uint fadeLength, bool* terminate, bool exiting = false );

    static  void XineEventListener( void*, const xine_event_t* );
    virtual void customEvent( QEvent* );
    virtual void timerEvent( QTimerEvent* );

    virtual void playlistChanged();

    Engine::SimpleMetaBundle fetchMetaData() const;

    bool makeNewStream();
    bool ensureStream();

    void determineAndShowErrorMessage(); //call after failure to load/play

    xine_t             *m_xine;
    xine_stream_t      *m_stream;
    xine_audio_port_t  *m_audioPort;
    xine_event_queue_t *m_eventQueue;
    xine_post_t        *m_post;

    int64_t             m_currentVpts;
    float               m_preamp;

    bool                m_stopFader;
    bool                m_fadeOutRunning;

    QString             m_currentAudioPlugin; //to see if audio plugin has been changed
    XineConfigDialog*   m_configDialog;
    //need to save these for when the audio plugin is changed and xine reloaded
    bool                m_equalizerEnabled;
    int                 m_intPreamp;
    Q3ValueList<int>     m_equalizerGains;

    mutable Engine::SimpleMetaBundle m_currentBundle;

private slots:
    void configChanged();

public:
    XineEngine();

signals:
    void resetConfig(xine_t *xine);
};

class Fader : public QThread
{
    XineEngine         *m_engine;
    xine_t             *m_xine;
    xine_stream_t      *m_decrease;
    xine_stream_t      *m_increase;
    xine_audio_port_t  *m_port;
    xine_post_t        *m_post;
    uint               m_fadeLength;
    bool               m_paused;
    bool               m_terminated;

    virtual void run();

public:
    Fader( XineEngine *, uint fadeLengthMs );
   ~Fader();
   void pause();
   void resume();
   void finish();
};

class OutFader : public QThread
{
    XineEngine *m_engine;
    bool        m_terminated;
    uint        m_fadeLength;

    virtual void run();

public:
    OutFader( XineEngine *, uint fadeLengthMs );
    ~OutFader();

   void finish();
};

#endif

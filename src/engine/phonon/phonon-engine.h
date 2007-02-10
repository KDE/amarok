/***************************************************************************
 *   Copyright (C) 2007 Dan Meltzer <hydrogen@notyetimplemented.com>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

// A lot of this was based on noatuns phonon engine.

#ifndef PHONON_ENGINE_H
#define PHONON_ENGINE_H

#include "enginebase.h"
#include <QThread>
//Added by qt3to4:
#include <QCustomEvent>
#include <Q3ValueList>
#include <QTimerEvent>
#include <phonon/phononnamespace.h>
#include "amarok_export.h"

extern "C"
{
    #include <sys/types.h>
}

namespace Phonon {
    class MediaObject;
    class AudioPath;
    class AudioOutput;
}

class AMAROK_PHONON_ENGINE_EXPORT PhononEngine : public Engine::Base
{
    Q_OBJECT

    friend class Fader;
    friend class OutFader;

   ~PhononEngine();

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
    virtual void setVolumeSW( uint );

//TODO: Add in audiocd stuff.
//     virtual bool metaDataForUrl(const KUrl &url, Engine::SimpleMetaBundle &b);
//     virtual bool getAudioCDContents(const QString &device, KUrl::List &urls);
    static Engine::State convertState(Phonon::State s);

    virtual Engine::State state() const;

    Phonon::MediaObject *m_mediaObject;
    Phonon::AudioPath *m_audioPath;
    Phonon::AudioOutput *m_audioOutput;

private slots:
    void configChanged();

public:
    PhononEngine();
};

// class Fader : public QThread
// {
//     XineEngine         *m_engine;
//     xine_t             *m_xine;
//     xine_stream_t      *m_decrease;
//     xine_stream_t      *m_increase;
//     xine_audio_port_t  *m_port;
//     xine_post_t        *m_post;
//     uint               m_fadeLength;
//     bool               m_paused;
//     bool               m_terminated;
// 
//     virtual void run();
// 
// public:
//     Fader( XineEngine *, uint fadeLengthMs );
//    ~Fader();
//    void pause();
//    void resume();
//    void finish();
// };

/*class OutFader : public QThread
{
    XineEngine *m_engine;
    bool        m_terminated;
    uint        m_fadeLength;

    virtual void run();

public:
    OutFader( XineEngine *, uint fadeLengthMs );
    ~OutFader();

   void finish();
};*/

#endif

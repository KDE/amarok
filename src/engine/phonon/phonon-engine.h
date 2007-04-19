/***************************************************************************
 *   Copyright (C) 2007 Dan Meltzer <hydrogen@notyetimplemented.com>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef PHONON_ENGINE_H
#define PHONON_ENGINE_H

#include "amarok_engines_export.h"
#include "enginebase.h"

#include <phonon/phononnamespace.h>

namespace Phonon {
    class MediaObject;
    class AudioPath;
    class AudioOutput;
}

class AMAROK_PHONON_ENGINE_EXPORT PhononEngine : public Engine::Base
{
    Q_OBJECT

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
    Phonon::AudioPath   *m_audioPath;
    Phonon::AudioOutput *m_audioOutput;

// private slots:
//     void configChanged();

public:
    PhononEngine();
};

#endif

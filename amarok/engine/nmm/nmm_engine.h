// Copyright (c) Max Howell 2004
// Licensed under GPL v2
// "lack of pussy makes you brave!"

#ifndef NMM_ENGINE_H
#define NMM_ENGINE_H

#include <config.h>
#ifdef HAVE_NMM

#include "engine/enginebase.h"
#include <nmm/NMMTypes.hpp>

namespace NMM { class MP3ReadNode; }

class NmmEngine : public EngineBase
{
Q_OBJECT

public:
    NmmEngine();
    ~NmmEngine();

    void init( bool&, int, bool ) {}

    bool initMixer( bool hardware );
    bool canDecode( const KURL&, mode_t, mode_t );
    long length() const;
    long position() const;
    bool isStream() const;

    EngineBase::EngineState state() const;

public slots: //FIXME make these slots in enginebase?
    const QObject* play( const KURL& );
    void  play();
    void  stop();
    void  pause();

    void  seek( long );
    void  setVolume( int );

private:
    double m_progress;
    bool   m_firstTime; //FIXME I HATE BOOLS LIKE THESE!
    long   m_lastKnownPosition;

    EngineBase::EngineState m_state;

    //these are receivers for progress events
    NMM::Result setProgress( u_int64_t&, u_int64_t& );
    NMM::Result endTrack();
};

#endif
#endif

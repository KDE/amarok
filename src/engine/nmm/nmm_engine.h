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

class NmmEngine : public Engine::Base
{
Q_OBJECT

public:
    NmmEngine();
    ~NmmEngine();

    bool init() { return true; }

    bool initMixer( bool hardware );
    bool canDecode( const KURL& ) const;
    uint position() const;

    Engine::State state() const;

public slots: //FIXME make these slots in enginebase?
    bool  load( const KURL&, bool stream );
    bool  play( uint offset = 0 );
    void  stop();
    void  pause();

    void  seek( uint );
//    void  setVolume( int );

protected:
    void  setVolumeSW( uint percent );

private:
    double m_progress;
    bool   m_firstTime; //FIXME I HATE BOOLS LIKE THESE!
    uint   m_lastKnownPosition;

    Engine::State m_state;

    //these are receivers for progress events
    NMM::Result setProgress( u_int64_t&, u_int64_t& );
    NMM::Result endTrack();
};

#endif
#endif

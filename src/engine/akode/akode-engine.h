// Copyright (C) 2005 Max Howell <max.howell@methylblue.com>
// Licensed as described in the COPYING file found in the root of this distribution
//

#include "engine/enginebase.h"

namespace aKode { class Player; }

class AkodeEngine : public Engine::Base
{
    virtual bool init();
    virtual bool canDecode( const KURL& ) const;
    virtual uint position() const;
    virtual bool load( const KURL&, bool );
    virtual bool play( uint );
    virtual void stop();
    virtual void pause();
    virtual void setVolumeSW( uint );
    virtual void seek( uint );

    virtual Engine::State state() const;

    virtual void customEvent( QCustomEvent* );

   ~AkodeEngine();

    aKode::Player *m_player;

public:
    AkodeEngine();
};

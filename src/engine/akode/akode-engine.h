/***************************************************************************
 *   Copyright (C) 2005 Max Howell <max.howell@methylblue.com>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

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

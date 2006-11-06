/***************************************************************************
                       void-engine.cpp - Dummy engine plugin

copyright            : (C) 2003 by Max Howell <max.howell@methylblue.com>
copyright            : (C) 2004 by Mark Kretschmann <markey@web.de>
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "enginebase.h"

class VoidEngine : public Engine::Base
{
    //Does nothing, just here to prevent crashes on startup
    //and in case no engines are found

    virtual bool init() { return true; }
    virtual bool canDecode( const KURL& ) const { return false; }
    virtual uint position() const { return 0; }
    virtual bool load( const KURL&, bool );
    virtual bool play( uint ) { return false; }
    virtual void stop() {}
    virtual void pause() {}
    virtual void unpause() {}
    virtual void setVolumeSW( uint ) {}
    virtual void seek( uint ) {}

    virtual Engine::State state() const { return Engine::Empty; }

public: VoidEngine() : EngineBase() {}
};


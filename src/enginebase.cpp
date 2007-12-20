//Copyright: (C) 2003 Mark Kretschmann
//           (C) 2004,2005 Max Howell, <max.howell@methylblue.com>
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
 
#include "enginebase.h"

#include <cmath>


Engine::Base::Base()
        : Amarok::Plugin()
        , m_xfadeLength( 0 )
        , m_xfadeNextTrack( false )
        , m_volume( 50 )
        , m_scope( SCOPESIZE )
        , m_isStream( false )
{}


Engine::Base::~Base()
{
}

//////////////////////////////////////////////////////////////////////


bool
Engine::Base::load( const KUrl &url, bool stream )
{
    m_url = url;
    m_isStream = stream;

    return true;
}

void
Engine::Base::beginFadeOut()
{
    stop();
}

void Engine::Base::setVolume( uint value )
{
    m_volume = value;

    setVolumeSW( makeVolumeLogarithmic( value ) );
}


uint
Engine::Base::makeVolumeLogarithmic( uint volume ) // static
{
    // We're using a logarithmic function to make the volume ramp more natural.
    return static_cast<uint>( 100 - 100.0 * std::log10( ( 100 - volume ) * 0.09 + 1.0 ) );
}


#include "enginebase.moc"

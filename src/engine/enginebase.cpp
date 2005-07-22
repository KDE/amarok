//Copyright: (C) 2003 Mark Kretschmann
//           (C) 2004,2005 Max Howell, <max.howell@methylblue.com>
//License:   See COPYING

#include <cmath>
#include "enginebase.h"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <sys/wait.h>
#include <unistd.h>


Engine::Base::Base( Effects *effects )
        : amaroK::Plugin()
        , m_effects( effects )
        , m_mixer( -1 )
        , m_volume( 50 )
        , m_scope( 512 )
        , m_isStream( false )
{}

Engine::Base::~Base()
{
    setHardwareMixer( false );

    delete m_effects;
}

//////////////////////////////////////////////////////////////////////


bool
Engine::Base::load( const KURL &url, bool stream )
{
    m_url = url;
    m_isStream = stream;

    return true;
}


bool
Engine::Base::setHardwareMixer( bool useHardware )
{
    //TODO optimise the applySettings section too

    if ( useHardware )
    {
        if ( isMixerHW() ) return true;

        m_mixer = ::open( "/dev/mixer", O_RDWR );

        if ( m_mixer >= 0 )
        {
            int devmask, recmask, i_recsrc, stereodevs;
            if ( ioctl( m_mixer, SOUND_MIXER_READ_DEVMASK, &devmask )       == -1 ) goto failure;
            if ( ioctl( m_mixer, SOUND_MIXER_READ_RECMASK, &recmask )       == -1 ) goto failure;
            if ( ioctl( m_mixer, SOUND_MIXER_READ_RECSRC, &i_recsrc )       == -1 ) goto failure;
            if ( ioctl( m_mixer, SOUND_MIXER_READ_STEREODEVS, &stereodevs ) == -1 ) goto failure;
            if ( !devmask )                                                         goto failure;

            setVolumeSW( 100 ); //seems sensible

            return true;
        }
    }

    //otherwise lets close the mixer

    if ( isMixerHW() )
    {
        ::close( m_mixer );   //close /dev/mixer device

    failure:
        m_mixer = -1;
    }

    return false;
}


void Engine::Base::setVolume( uint value )
{
    m_volume = value;

    if( isMixerHW() ) {
        setVolumeHW( value );
        setVolumeSW( 100 );
    }
    else
        setVolumeSW( makeVolumeLogarithmic( value ) );
}


void
Engine::Base::setVolumeHW( uint percent )
{
    if ( isMixerHW() )
    {
        percent = percent + ( percent << 8 );
        ioctl( m_mixer, MIXER_WRITE( 4 ), &percent );
    }
}


uint
Engine::Base::makeVolumeLogarithmic( uint volume ) // static
{
    // We're using a logarithmic function to make the volume ramp more natural.
    return static_cast<uint>( 100 - 100.0 * std::log10( ( 100 - volume ) * 0.09 + 1.0 ) );
}


#include "enginebase.moc"

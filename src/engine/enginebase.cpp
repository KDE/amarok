//Copyright: (C) 2003 Mark Kretschmann
//           (C) 2004 Max Howell, <max.howell@methylblue.com>
//License:   See COPYING

#include "enginebase.h"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <sys/wait.h>
#include <unistd.h>


Engine::Base::Base( StreamingMode mode, bool hasConfigure, bool hasXFade, Effects *effects )
    : amaroK::Plugin( hasConfigure )
    , m_xfadeLength( 1500 ) //FIXME
    , m_streamingMode( mode )
    , m_hasXFade( hasXFade )
    , m_effects( effects )
    , m_mixer( -1 )
    , m_volume( 50 )
    , m_scope( 512 )
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

void
Engine::Base::setVolumeHW( uint percent )
{
    if ( isMixerHW() )
    {
        percent = percent + ( percent << 8 );
        ioctl( m_mixer, MIXER_WRITE( 4 ), &percent );
    }
}

#include "enginebase.moc"

/***************************************************************************
                      enginebase.cpp  -  audio engine base class
                         -------------------
begin                : Dec 31 2003
copyright            : (C) 2003 by Mark Kretschmann
email                : markey@web.de
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

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <sys/wait.h>
#include <unistd.h>

#include <qstringlist.h>
#include <kdebug.h>


EngineBase::EngineBase()
    : amaroK::Plugin()
    , m_mixerHW( -1 )
    , m_volume( 50 )
    , m_xfadeLength( 0 )
    , m_restoreEffects( false )
    , m_defaultSoundDevice( true )
{}


EngineBase::~EngineBase()
{
    kdDebug() << k_funcinfo << endl;

    closeMixerHW();
}

//////////////////////////////////////////////////////////////////////

bool EngineBase::initMixerHW()
{
    if ( ( m_mixerHW = ::open( "/dev/mixer", O_RDWR ) ) < 0 )
        return false;  //failed
    else
    {
        int devmask, recmask, i_recsrc, stereodevs;
        if ( ioctl( m_mixerHW, SOUND_MIXER_READ_DEVMASK, &devmask )       == -1 ) return false;
        if ( ioctl( m_mixerHW, SOUND_MIXER_READ_RECMASK, &recmask )       == -1 ) return false;
        if ( ioctl( m_mixerHW, SOUND_MIXER_READ_RECSRC, &i_recsrc )       == -1 ) return false;
        if ( ioctl( m_mixerHW, SOUND_MIXER_READ_STEREODEVS, &stereodevs ) == -1 ) return false;
        if ( !devmask )                                                           return false;
    }

    return true;
}


void EngineBase::closeMixerHW()
{
    if ( m_mixerHW != -1 )
    {
        ::close( m_mixerHW );   //close /dev/mixer device
        m_mixerHW = -1;
    }
}


void EngineBase::setVolumeHW( int percent )
{
    if ( m_mixerHW != -1 )
    {
        percent = percent + ( percent << 8 );
        ioctl( m_mixerHW, MIXER_WRITE( 4 ), &percent );
    }
}


void EngineBase::setXfadeLength( int ms )
{
    m_xfadeLength = ms;
}


void EngineBase::setSoundOutput( const QString& output )
{
    kdDebug() << "Setting sound output to: " << output << endl;

    m_soundOutput = output;
}


#include "enginebase.moc"

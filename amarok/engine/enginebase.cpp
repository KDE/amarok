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

#include "artsengine.h"
#include "enginebase.h"
#include "gstengine.h"
#include "nmm_engine.h"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <sys/wait.h>

#include "qstringlist.h"


bool EngineBase::m_restoreEffects;


EngineBase::~EngineBase()
{
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

//////////////////////////////////////////////////////////////////////////////
// STATIC METHODS
//////////////////////////////////////////////////////////////////////////////

EngineBase* EngineBase::createEngine( QString system, bool& restart, int scopeSize, bool restoreEffects = true )
{
    m_restoreEffects = restoreEffects;

    //TODO capitalise the engine names in the right places, this causes an issue with the configdialog engine
    //selector however

#ifdef HAVE_ARTS
    if ( system == "arts" )
        return new ArtsEngine( restart, scopeSize );
#endif

#ifdef HAVE_GSTREAMER
    if ( system == "gstreamer" )
        return new GstEngine( scopeSize );
#endif

#ifdef HAVE_NMM
    if ( system == "NMM" )
        return new NmmEngine();
#endif

    return 0;
}


QStringList EngineBase::listEngines()
{
    QStringList list;

#ifdef HAVE_ARTS
    list.append( "arts" );
#endif

#ifdef HAVE_GSTREAMER
    list.append( "gstreamer" );
#endif

#ifdef HAVE_NMM
    list.append( "NMM" );
#endif

    return list;
}


#include "enginebase.moc"

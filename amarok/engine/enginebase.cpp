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

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <sys/wait.h>

#include "qstringlist.h"

EngineBase::~EngineBase()
{}


//////////////////////////////////////////////////////////////////////

bool EngineBase::initMixerHW()
{
    if ( ( m_mixerHW = ::open( "/dev/mixer", O_RDWR ) ) < 0 )
        return false;

    int devmask, recmask, i_recsrc, stereodevs;
    if ( ioctl( m_mixerHW, SOUND_MIXER_READ_DEVMASK, &devmask )       == -1 ) return false;
    if ( ioctl( m_mixerHW, SOUND_MIXER_READ_RECMASK, &recmask )       == -1 ) return false;
    if ( ioctl( m_mixerHW, SOUND_MIXER_READ_RECSRC, &i_recsrc )       == -1 ) return false;
    if ( ioctl( m_mixerHW, SOUND_MIXER_READ_STEREODEVS, &stereodevs ) == -1 ) return false;
    if ( !devmask )                                                           return false;

    return true;
}


void EngineBase::setVolumeHW( int percent )
{
    percent = percent + ( percent << 8 );
    ioctl( m_mixerHW, MIXER_WRITE( 4 ), &percent );
}


//////////////////////////////////////////////////////////////////////

EngineBase* EngineBase::createEngine( QString system, bool& restart )
{
    EngineBase *p = 0;
    
    if ( system == "arts" )
        p = new ArtsEngine( restart );
    
#ifdef HAVE_GSTREAMER    
    if ( system == "gstreamer" )
        p = new GstEngine();
#endif           
            
    return p;
}


QStringList EngineBase::listEngines()
{
    QStringList list;
    
    list.append( "arts" );
    
#ifdef HAVE_GSTREAMER
    list.append( "gstreamer" );
#endif
    
    return list;
}


#include "enginebase.moc"
